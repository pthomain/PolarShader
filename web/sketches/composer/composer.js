// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// Top-level composer panel. Builds the entire UI after FastLED's WASM
// module is ready, owns the reactive scene model, and pushes scene changes
// to WASM on the next animation frame.
//
// Mounting strategy: FastLED's generated index.html provides a Three.js
// canvas + an empty body. We append a fixed sidebar to the right side
// of the viewport.

import {
    PALETTES, PATTERNS, TRANSFORMS, PF_PRESETS, DEFAULT_SCENE, DEFAULT_SIGNAL,
} from './schema.js';
import { encodeScene, decodeScene } from './codec.js';
import { renderSignalSlot } from './signal-editor.js';

// ─────────────────────────────────────────────────────────────────────
// Reactive scene model — the source of truth on the JS side.
// ─────────────────────────────────────────────────────────────────────

function createSceneStore(initialScene) {
    let scene = initialScene;
    let revision = 0;
    const listeners = new Set();

    function emit(change) {
        revision += 1;
        const event = {
            revision,
            change,
            scene,
            time: performance.now(),
        };
        for (const listener of listeners) {
            listener(event);
        }
    }

    return {
        get scene() {
            return scene;
        },
        subscribe(listener) {
            listeners.add(listener);
            return () => listeners.delete(listener);
        },
        mutate(change, mutator) {
            mutator(scene);
            emit(change);
        },
        setScene(nextScene, change = { path: 'scene', value: 'loaded' }) {
            scene = nextScene;
            emit(change);
        },
        notify(change = { path: 'scene', value: 'initial' }) {
            emit(change);
        },
    };
}

const BOOT_SCENE_STORAGE_KEY = 'polarComposerBootScene:v1';

function readBootSceneState() {
    try {
        const raw = sessionStorage.getItem(BOOT_SCENE_STORAGE_KEY);
        const saved = raw ? JSON.parse(raw) : null;
        if (saved?.verified !== true || !Array.isArray(saved.bytes)) return null;
        const bytes = new Uint8Array(saved.bytes);
        return { bytes, scene: decodeScene(bytes) };
    } catch {
        return null;
    }
}

const bootSceneState = readBootSceneState();
const sceneStore = createSceneStore(bootSceneState?.scene ?? DEFAULT_SCENE());

function displayIdFromParam(raw) {
    if (raw === '1' || raw === 'round' || raw === 'polar') return 1;
    return 0;
}

function initialDisplayId() {
    const params = new URLSearchParams(window.location.search);
    return displayIdFromParam(params.get('display') ?? params.get('ps_display'));
}

const state = {
    pushTimer: null,             // debounce timer handle for scene pushes
    latestEvent: null,           // most-recent scene-store emission
    statusEl: null,              // toast target
    debugLogEl: null,            // FastLED-side debug console
    displayId: initialDisplayId(), // active display geometry (0 = fabric, 1 = round)
    renderSeq: 0,                // monotonic command generation for scene pushes / reloads
    latestSceneSeq: 0,           // sequence assigned to the newest scene-store emission
    lastGoodBootBytes: bootSceneState?.bytes ?? null,
    workerRendererSeen: false,
    workerRendererFatal: false,
    workerRendererReloading: false,
    resetRendererBtn: null,
    composerTab: 'new',
    presetSelected: PF_PRESETS[0]?.id ?? '',
    sceneSourceName: '',
    playlistApiChecked: false,
    playlistApiAvailable: false,
    playlistItems: [],
    playlistSelected: '',
    playlistBusy: false,
    displayConfigApiAvailable: false,
    displayBrightness: 255,
    displayBrightnessSaved: false,
    displayBrightnessBusy: false,
    displayBrightnessSaveTimer: null,
    deployApiAvailable: false,
    deployTargets: [],
    deployDevices: [],
    deployDeviceError: '',
    deployTarget: '',
    deployBusy: false,
    deployDetectBusy: false,
    deployJobId: '',
    deployStatus: '',
    deployError: '',
    deployLogOffset: 0,
    deployPollTimer: null,
};

// Display geometry tags — must match the display enum in composer.ino.
const DISPLAYS = [
    { id: 0, name: 'Fabric (20×20 matrix)' },
    { id: 1, name: 'Round (241-pixel radial)' },
];

const DEPLOY_POLL_MS = 1000;
const DEPLOY_DONE_STATUSES = new Set(['succeeded', 'failed', 'timed_out']);
const DEBUG_LOG_LIMIT = 500;
const BRIGHTNESS_SAVE_DEBOUNCE_MS = 250;
const BLOOM_BASE_BRIGHTNESS = 30;
const BLOOM_MAX_BRIGHTNESS = 255;
const BLOOM_BASE_MULTIPLIER = 1;
const BLOOM_MAX_MULTIPLIER = 3;

function displayUrlValue(which) {
    return which === 1 ? 'round' : 'fabric';
}

function reloadForDisplay(which) {
    const url = new URL(window.location.href);
    url.searchParams.set('display', displayUrlValue(which));
    url.searchParams.delete('ps_display');
    window.location.replace(url.toString());
}

function mutateScene(path, value, mutator) {
    sceneStore.mutate({ path, value }, mutator);
}

function formatValue(value) {
    return typeof value === 'string' ? value : (JSON.stringify(value) ?? String(value));
}

// Hex dump of the encoded scene bytes, so a failed apply can be replayed /
// inspected against the codec offline.
function bytesToHex(bytes) {
    return Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join(' ');
}

function signalSummary(signal) {
    return {
        id: signal?.id,
        params: signal?.params ?? {},
    };
}

// The render view of the scene: transforms toggled off in the UI (enabled ===
// false) are dropped so the renderer never applies them, while they stay
// intact in the UI model (params unchanged) for quick re-enabling. `enabled`
// is UI-only state — it has no PSC wire representation, so it lives purely on
// the JS scene object. A shallow copy is fine: encodeScene only reads.
function sceneForRender(scene) {
    return {
        ...scene,
        transforms: (scene.transforms ?? []).filter((t) => t.enabled !== false),
    };
}

function sceneSummary(scene) {
    return {
        paletteId: scene.paletteId,
        pattern: scene.pattern?.id,
        patternConfig: scene.pattern?.config ?? {},
        patternSignals: Object.fromEntries(
            Object.entries(scene.pattern?.signals ?? {}).map(([name, signal]) => [
                name, signalSummary(signal),
            ])
        ),
        transforms: (scene.transforms ?? []).map((t) => ({
            id: t.id,
            config: t.config ?? {},
            signals: Object.fromEntries(
                Object.entries(t.signals ?? {}).map(([name, signal]) => [
                    name, signalSummary(signal),
                ])
            ),
        })),
    };
}

function slugForFilename(value) {
    return String(value ?? '')
        .replace(/([a-z0-9])([A-Z])/g, '$1 $2')
        .trim()
        .replace(/^pf\W+/i, '')
        .toLowerCase()
        .replace(/[^a-z0-9]+/g, '-')
        .replace(/^-+|-+$/g, '');
}

function timestampForFilename(date = new Date()) {
    const parts = [
        date.getFullYear().toString(),
        (date.getMonth() + 1).toString().padStart(2, '0'),
        date.getDate().toString().padStart(2, '0'),
        '-',
        date.getHours().toString().padStart(2, '0'),
        date.getMinutes().toString().padStart(2, '0'),
        date.getSeconds().toString().padStart(2, '0'),
    ];
    return parts.join('');
}

function pscDownloadFilename(scene) {
    const patternId = scene.pattern?.id ?? '';
    const patternName = PATTERNS[patternId]?.label ?? patternId;
    const prefix = slugForFilename(patternName) || slugForFilename(patternId) || 'scene';
    return `${prefix}-${timestampForFilename()}.psc`;
}

const PLAYLIST_FILENAME_PART_RE = /^[A-Za-z0-9][A-Za-z0-9._ -]*$/;

function isSafePlaylistFilename(name) {
    const trimmed = String(name ?? '').trim();
    if (!trimmed || !/\.psc$/i.test(trimmed)) return false;
    const parts = trimmed.split('/');
    return parts.every((part) => part
        && part !== '.'
        && part !== '..'
        && PLAYLIST_FILENAME_PART_RE.test(part));
}

function preferredPlaylistFilename() {
    return isSafePlaylistFilename(state.sceneSourceName)
        ? state.sceneSourceName.trim()
        : pscDownloadFilename(sceneStore.scene);
}

function bytesEqual(left, right) {
    if (!left || !right || left.length !== right.length) return false;
    for (let i = 0; i < left.length; i += 1) {
        if (left[i] !== right[i]) return false;
    }
    return true;
}

function readFileBytes(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = (ev) => resolve(new Uint8Array(ev.target.result));
        reader.onerror = () => reject(reader.error ?? new Error('file read failed'));
        reader.readAsArrayBuffer(file);
    });
}

function playlistApiUrl(path, params = {}) {
    const url = new URL(path, window.location.origin);
    for (const [key, value] of Object.entries(params)) {
        if (value !== undefined && value !== null && value !== '') {
            url.searchParams.set(key, value);
        }
    }
    return url.toString();
}

async function fetchPlaylistJson(path, options = {}) {
    const response = await fetch(playlistApiUrl(path, options.params), {
        ...options,
        params: undefined,
        headers: {
            Accept: 'application/json',
            ...(options.headers ?? {}),
        },
    });
    const body = await response.json().catch(() => ({}));
    if (!response.ok || body?.ok === false) {
        throw new Error(body?.error || `${response.status} ${response.statusText}`);
    }
    return body;
}

function clampDisplayBrightness(value) {
    const numeric = Number.parseInt(value, 10);
    if (!Number.isFinite(numeric)) return 255;
    return Math.max(0, Math.min(255, numeric));
}

function smap(value, inMin, inMax, outMin, outMax) {
    if (inMax === inMin) return outMin;
    const t = Math.max(0, Math.min(1, (value - inMin) / (inMax - inMin)));
    return outMin + (outMax - outMin) * t;
}

function bloomStrengthMultiplierForBrightness(value) {
    return smap(
        clampDisplayBrightness(value),
        BLOOM_BASE_BRIGHTNESS,
        BLOOM_MAX_BRIGHTNESS,
        BLOOM_BASE_MULTIPLIER,
        BLOOM_MAX_MULTIPLIER
    );
}

async function refreshDisplayConfig({ rebuild = false } = {}) {
    if (!state.displayConfigApiAvailable) return;
    try {
        const body = await fetchPlaylistJson('/api/display/config');
        state.displayBrightness = clampDisplayBrightness(body.brightness);
        state.displayBrightnessSaved = body.saved === true;
        if (body.warning) {
            logDebug('display', body.warning);
        }
        void setWorkerBloom(bloomPreference);
    } catch (e) {
        state.displayConfigApiAvailable = false;
        logDebug('display', `display config unavailable: ${e.message}`);
    } finally {
        if (rebuild) rebuildPanel();
    }
}

async function saveDisplayBrightness(value, { rebuild = true, notify = true } = {}) {
    if (!state.displayConfigApiAvailable) return true;
    const brightness = clampDisplayBrightness(value);
    state.displayBrightness = brightness;
    state.displayBrightnessBusy = true;
    if (rebuild) rebuildPanel();
    try {
        const body = await fetchPlaylistJson('/api/display/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ brightness }),
        });
        state.displayBrightness = clampDisplayBrightness(body.brightness);
        state.displayBrightnessSaved = true;
        void setWorkerBloom(bloomPreference);
        if (notify) {
            showStatus(`brightness ${state.displayBrightness}`, false);
            logDebug('display', `MCU brightness saved: ${state.displayBrightness}`);
        }
        return true;
    } catch (e) {
        if (notify) {
            showStatus(`brightness save error: ${e.message}`, true);
        }
        logDebug('error', `brightness save failed: ${e.message}`);
        return false;
    } finally {
        state.displayBrightnessBusy = false;
        if (rebuild) rebuildPanel();
    }
}

function scheduleDisplayBrightnessSave(value) {
    state.displayBrightness = clampDisplayBrightness(value);
    if (state.displayBrightnessSaveTimer !== null) {
        clearTimeout(state.displayBrightnessSaveTimer);
    }
    state.displayBrightnessSaveTimer = setTimeout(() => {
        state.displayBrightnessSaveTimer = null;
        void saveDisplayBrightness(state.displayBrightness);
    }, BRIGHTNESS_SAVE_DEBOUNCE_MS);
}

async function flushDisplayBrightnessSave() {
    if (state.displayBrightnessSaveTimer === null) return true;
    clearTimeout(state.displayBrightnessSaveTimer);
    state.displayBrightnessSaveTimer = null;
    return saveDisplayBrightness(state.displayBrightness, { rebuild: true, notify: false });
}

function deployTargetById(id) {
    return state.deployTargets.find((target) => target.id === id) ?? null;
}

function deployTargetLabel(id) {
    return deployTargetById(id)?.label ?? id;
}

function deployDeviceSearchText(device) {
    if (!device || typeof device !== 'object') return '';
    return [
        device.port,
        device.description,
        device.hwid,
        device.manufacturer,
        device.product,
    ].filter(Boolean).join(' ').toLowerCase();
}

function autoSelectDeployTarget() {
    if (state.deployTarget || state.deployTargets.length === 0 || state.deployDevices.length === 0) return;
    const deviceText = state.deployDevices.map(deployDeviceSearchText).join(' ');
    let targetId = '';
    if (deviceText.includes('teensy')) {
        targetId = 'teensy41_matrix';
    } else if (deviceText.includes('rp2040')) {
        targetId = state.displayId === 1 ? 'seeed_xiao_rp2040_round' : 'seeed_xiao_rp2040_fabric';
    } else if (deviceText.includes('samd') || deviceText.includes('atsamd')) {
        targetId = 'seeed_xiao';
    } else {
        const matches = state.deployTargets.filter((target) => {
            const hints = Array.isArray(target.matches) ? target.matches : [];
            return hints.some((hint) => deviceText.includes(String(hint).toLowerCase()));
        });
        if (matches.length === 1) targetId = matches[0].id;
    }
    if (targetId && state.deployTargets.some((target) => target.id === targetId)) {
        state.deployTarget = targetId;
    }
}

async function refreshDeployTargets({ rebuild = false } = {}) {
    if (!state.deployApiAvailable) return;
    const body = await fetchPlaylistJson('/api/deploy/targets');
    state.deployTargets = Array.isArray(body.targets) ? body.targets : [];
    if (state.deployTarget && !state.deployTargets.some((target) => target.id === state.deployTarget)) {
        state.deployTarget = '';
    }
    autoSelectDeployTarget();
    if (rebuild) rebuildPanel();
}

async function refreshDeployDevices({ rebuild = false } = {}) {
    if (!state.deployApiAvailable || state.deployBusy || state.deployDetectBusy) return;
    state.deployDetectBusy = true;
    state.deployDeviceError = '';
    if (rebuild) rebuildPanel();
    try {
        const body = await fetchPlaylistJson('/api/deploy/devices', { method: 'POST' });
        state.deployDevices = Array.isArray(body.devices) ? body.devices : [];
        state.deployDeviceError = body.error ?? '';
        autoSelectDeployTarget();
        if (state.deployDeviceError) {
            logDebug('deploy', `device detection: ${state.deployDeviceError}`);
        } else {
            logDebug('deploy', `device detection found ${state.deployDevices.length} device(s)`);
        }
    } catch (e) {
        state.deployDevices = [];
        state.deployDeviceError = e.message;
        logDebug('deploy', `device detection failed: ${e.message}`);
    } finally {
        state.deployDetectBusy = false;
        if (rebuild) rebuildPanel();
    }
}

async function refreshDeployData({ rebuild = false } = {}) {
    if (!state.deployApiAvailable) return;
    try {
        await refreshDeployTargets();
        await refreshDeployDevices();
    } catch (e) {
        state.deployApiAvailable = false;
        state.deployDeviceError = e.message;
        logDebug('deploy', `deploy API unavailable: ${e.message}`);
    } finally {
        if (rebuild) rebuildPanel();
    }
}

async function refreshPlaylistItems({ rebuild = false } = {}) {
    if (!state.playlistApiAvailable) return;
    const body = await fetchPlaylistJson('/api/playlist');
    state.playlistItems = Array.isArray(body.files) ? body.files : [];
    if (state.playlistSelected && !state.playlistItems.some((file) => file.name === state.playlistSelected)) {
        state.playlistSelected = '';
    }
    if (rebuild) rebuildPanel();
}

async function refreshPlaylistCapability({ rebuild = false } = {}) {
    try {
        const body = await fetchPlaylistJson('/api/capabilities');
        state.playlistApiAvailable = body.savePsc === true && body.playlist === true;
        state.deployApiAvailable = state.playlistApiAvailable && body.deploy === true;
        state.displayConfigApiAvailable = body.displayConfig === true;
        state.playlistApiChecked = true;
        if (state.playlistApiAvailable) {
            await refreshPlaylistItems();
        }
        if (state.displayConfigApiAvailable) {
            await refreshDisplayConfig();
        }
        if (state.deployApiAvailable && state.composerTab === 'playlist') {
            await refreshDeployData();
        } else if (!state.deployApiAvailable) {
            state.deployTargets = [];
            state.deployDevices = [];
            state.deployTarget = '';
        }
    } catch {
        state.playlistApiAvailable = false;
        state.deployApiAvailable = false;
        state.displayConfigApiAvailable = false;
        state.playlistApiChecked = true;
        state.playlistItems = [];
        state.playlistSelected = '';
        state.deployTargets = [];
        state.deployDevices = [];
        state.deployTarget = '';
    }
    if (rebuild) rebuildPanel();
}

async function loadPlaylistScene(name) {
    if (!name || state.playlistBusy) return;
    state.playlistBusy = true;
    state.playlistSelected = name;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const response = await fetch(playlistApiUrl('/api/playlist/file', { name }), {
            headers: { Accept: 'application/octet-stream' },
        });
        if (!response.ok) {
            const body = await response.json().catch(() => ({}));
            throw new Error(body?.error || `${response.status} ${response.statusText}`);
        }
        const bytes = new Uint8Array(await response.arrayBuffer());
        const decoded = decodeScene(bytes);
        sceneStore.setScene(decoded, {
            path: 'scene.playlist',
            value: name,
        });
        state.sceneSourceName = name;
        statusText = `loaded ${name}`;
        logDebug('playlist', `loaded ${name} (${bytes.length} bytes)`);
    } catch (e) {
        statusText = `playlist load error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist load failed for ${name}: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

async function fetchPlaylistFileBytes(name) {
    const response = await fetch(playlistApiUrl('/api/playlist/file', { name }), {
        headers: { Accept: 'application/octet-stream' },
    });
    if (!response.ok) {
        const body = await response.json().catch(() => ({}));
        throw new Error(body?.error || `${response.status} ${response.statusText}`);
    }
    return new Uint8Array(await response.arrayBuffer());
}

async function fetchPlaylistByteEntries() {
    const entries = [];
    for (const file of state.playlistItems) {
        entries.push({
            name: file.name,
            bytes: await fetchPlaylistFileBytes(file.name),
        });
    }
    return entries;
}

async function saveCurrentSceneToPlaylist() {
    if (!state.playlistApiAvailable || state.playlistBusy || state.deployBusy) return;
    const sourceName = String(state.sceneSourceName ?? '').trim();
    const filename = preferredPlaylistFilename();
    const renamedFromSource = sourceName && filename !== sourceName;
    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const bytes = encodeScene(sceneForRender(sceneStore.scene));
        const body = await fetchPlaylistJson('/api/playlist/save', {
            method: 'POST',
            params: { name: filename },
            headers: { 'Content-Type': 'application/octet-stream' },
            body: bytes,
        });
        const savedName = body.file?.name ?? filename;
        state.playlistSelected = savedName;
        state.sceneSourceName = filename;
        await refreshPlaylistItems();
        statusText = renamedFromSource
            ? `saved ${savedName} (renamed from ${sourceName})`
            : `saved ${savedName}`;
        logDebug('playlist', `${statusText} (${bytes.length} bytes)`);
    } catch (e) {
        statusText = `playlist save error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist save failed: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

async function deletePlaylistScene(name) {
    if (!name || state.playlistBusy || state.deployBusy) return;
    if (!window.confirm(`Delete ${name} from the playlist?`)) return;

    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const body = await fetchPlaylistJson('/api/playlist/delete', {
            method: 'POST',
            params: { name },
        });
        const deletedName = body.file?.name ?? name;
        if (state.playlistSelected === deletedName) {
            state.playlistSelected = '';
        }
        await refreshPlaylistItems();
        statusText = `deleted ${deletedName}`;
        logDebug('playlist', statusText);
    } catch (e) {
        statusText = `playlist delete error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist delete failed for ${name}: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

async function importFilesToPlaylist(fileList) {
    if (!state.playlistApiAvailable || state.playlistBusy || state.deployBusy) return;
    const files = Array.from(fileList ?? []);
    if (files.length === 0) return;

    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    let imported = 0;
    let skipped = 0;
    let failed = 0;
    try {
        await refreshPlaylistItems();
        const knownEntries = await fetchPlaylistByteEntries();

        for (const file of files) {
            let bytes;
            let decoded;
            try {
                bytes = await readFileBytes(file);
                decoded = decodeScene(bytes);
            } catch (e) {
                failed += 1;
                logDebug('error', `playlist import skipped ${file.name}: ${e.message}`);
                continue;
            }

            const duplicate = knownEntries.find((entry) => bytesEqual(entry.bytes, bytes));
            if (duplicate) {
                skipped += 1;
                logDebug('playlist', `import skipped ${file.name}: already present as ${duplicate.name}`);
                continue;
            }

            const sourceName = String(file.name ?? '').trim();
            const filename = isSafePlaylistFilename(sourceName)
                ? sourceName
                : pscDownloadFilename(decoded);
            const body = await fetchPlaylistJson('/api/playlist/save', {
                method: 'POST',
                params: { name: filename },
                headers: { 'Content-Type': 'application/octet-stream' },
                body: bytes,
            });
            const savedName = body.file?.name ?? filename;
            knownEntries.push({ name: savedName, bytes });
            state.playlistSelected = savedName;
            imported += 1;
            if (filename !== sourceName) {
                logDebug('playlist', `imported ${sourceName || 'unnamed file'} as ${savedName}`);
            } else {
                logDebug('playlist', `imported ${savedName}`);
            }
        }

        await refreshPlaylistItems();
        statusText = `imported ${imported}, skipped ${skipped}${failed ? `, failed ${failed}` : ''}`;
        statusError = failed > 0 && imported === 0;
    } catch (e) {
        statusText = `playlist import error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist import failed: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

function appendDeployLog(logs) {
    const text = String(logs ?? '');
    if (text.length <= state.deployLogOffset) return;
    const chunk = text.slice(state.deployLogOffset);
    state.deployLogOffset = text.length;
    for (const line of chunk.split(/\r?\n|\r/)) {
        const cleaned = line.replace(/\x1b\[[0-9;?]*[ -/]*[@-~]/g, '');
        if (cleaned.trim()) logDebug('deploy', cleaned);
    }
}

function clearDeployPollTimer() {
    if (state.deployPollTimer === null) return;
    clearTimeout(state.deployPollTimer);
    state.deployPollTimer = null;
}

function handleDeployJobSnapshot(job) {
    if (!job) return false;
    state.deployStatus = job.status ?? '';
    state.deployError = job.error ?? '';
    appendDeployLog(job.logs ?? '');
    const done = DEPLOY_DONE_STATUSES.has(state.deployStatus);
    if (!done) return false;

    state.deployBusy = false;
    clearDeployPollTimer();
    if (state.deployStatus === 'succeeded') {
        showStatus(`deployed ${deployTargetLabel(job.env)}`, false);
        logDebug('deploy', `deploy succeeded for ${deployTargetLabel(job.env)}`);
    } else {
        const detail = state.deployError || state.deployStatus || 'failed';
        showStatus(`deploy ${detail}`, true);
        logDebug('deploy', `deploy failed for ${deployTargetLabel(job.env)}: ${detail}`);
    }
    return true;
}

async function pollDeployStatus() {
    if (!state.deployJobId) return;
    state.deployPollTimer = null;
    try {
        const body = await fetchPlaylistJson('/api/deploy/status', {
            params: { id: state.deployJobId },
        });
        if (!handleDeployJobSnapshot(body.job)) {
            state.deployPollTimer = setTimeout(pollDeployStatus, DEPLOY_POLL_MS);
        }
    } catch (e) {
        state.deployBusy = false;
        clearDeployPollTimer();
        state.deployError = e.message;
        showStatus(`deploy status error: ${e.message}`, true);
        logDebug('deploy', `status polling failed: ${e.message}`);
    } finally {
        rebuildPanel();
    }
}

async function startDeployToMcu() {
    if (!state.deployApiAvailable
        || state.deployBusy
        || state.playlistBusy
        || state.deployDetectBusy
        || state.displayBrightnessBusy) return;
    if (!(await flushDisplayBrightnessSave())) {
        showStatus('save brightness before deploying', true);
        return;
    }
    if (state.playlistItems.length === 0) {
        showStatus('save at least one composition to the playlist first', true);
        return;
    }
    if (!state.deployTarget) {
        showStatus('choose an MCU target first', true);
        return;
    }

    state.deployBusy = true;
    state.deployJobId = '';
    state.deployStatus = 'starting';
    state.deployError = '';
    state.deployLogOffset = 0;
    clearDeployPollTimer();
    rebuildPanel();
    try {
        const body = await fetchPlaylistJson('/api/deploy', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ env: state.deployTarget }),
        });
        state.deployJobId = body.job?.id ?? '';
        if (!state.deployJobId) throw new Error('deploy response missing job id');
        const done = handleDeployJobSnapshot(body.job);
        showStatus(`deploying ${deployTargetLabel(state.deployTarget)}`, false);
        logDebug('deploy', `started deploy ${state.deployJobId || '?'} for ${deployTargetLabel(state.deployTarget)}`);
        if (!done) {
            state.deployPollTimer = setTimeout(pollDeployStatus, DEPLOY_POLL_MS);
        }
    } catch (e) {
        state.deployBusy = false;
        state.deployError = e.message;
        showStatus(`deploy error: ${e.message}`, true);
        logDebug('deploy', `deploy start failed: ${e.message}`);
    } finally {
        rebuildPanel();
    }
}

function persistBootSceneBytes(bytes) {
    try {
        state.lastGoodBootBytes = new Uint8Array(bytes);
        sessionStorage.setItem(BOOT_SCENE_STORAGE_KEY, JSON.stringify({
            verified: true,
            displayId: state.displayId,
            bytes: Array.from(bytes),
        }));
    } catch (e) {
        logDebug('error', `failed to save boot scene: ${e?.message ?? e}`);
    }
}

function saveLastGoodBootSceneState() {
    if (!state.lastGoodBootBytes) {
        sessionStorage.removeItem(BOOT_SCENE_STORAGE_KEY);
        return;
    }
    persistBootSceneBytes(state.lastGoodBootBytes);
}

function logDebug(kind, detail) {
    if (!state.debugLogEl) return;
    const row = document.createElement('div');
    row.className = `composer-debug-line ${kind}`;
    const now = new Date();
    const stamp = [
        now.getHours().toString().padStart(2, '0'),
        now.getMinutes().toString().padStart(2, '0'),
        now.getSeconds().toString().padStart(2, '0'),
    ].join(':');
    row.textContent = `[${stamp}] ${detail}`;
    state.debugLogEl.appendChild(row);
    while (state.debugLogEl.children.length > DEBUG_LOG_LIMIT) {
        state.debugLogEl.removeChild(state.debugLogEl.firstChild);
    }
    state.debugLogEl.scrollTop = state.debugLogEl.scrollHeight;
}

function mountDebugConsole() {
    if (document.getElementById('composer-debug-console')) return;

    // FastLED injects a default "Async Controls" box (Start/Stop/Toggle/FPS)
    // that the composer does not use — remove it.
    document.getElementById('fastled-async-controls')?.remove();

    const panel = document.createElement('div');
    panel.id = 'composer-debug-console';

    const title = document.createElement('div');
    title.className = 'composer-debug-title';
    title.textContent = 'Composer event console';
    panel.appendChild(title);

    state.debugLogEl = document.createElement('div');
    state.debugLogEl.className = 'composer-debug-log';
    panel.appendChild(state.debugLogEl);

    const anchor = document.getElementById('canvas-container');
    if (anchor) {
        anchor.insertAdjacentElement('afterend', panel);
    } else {
        document.body.appendChild(panel);
    }

    logDebug('system', 'console mounted');
}

// ─────────────────────────────────────────────────────────────────────
// WASM bridge
// ─────────────────────────────────────────────────────────────────────

// FastLED runs the Emscripten module on the main thread but does not
// expose it as `window.Module`. The controller (built by FastLED's
// runtime) holds the module reference under `.module`.
function getWasmModule() {
    const ctrl = window.getFastLEDController?.() ?? window.fastLEDController;
    return ctrl?.module ?? null;
}

function getWorkerManager() {
    const manager = window.__polarComposerWorkerManager ?? window.fastLEDWorkerManager;
    if (!manager?.isWorkerActive || !manager.worker
            || typeof manager.sendMessageWithResponse !== 'function') {
        return null;
    }
    return manager;
}

function delay(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

function nextRenderSeq() {
    state.renderSeq = (state.renderSeq + 1) >>> 0;
    if (state.renderSeq === 0) state.renderSeq = 1;
    return state.renderSeq;
}

function isCurrentRenderSeq(seq) {
    return seq === 0 || seq === state.renderSeq;
}

function isStaleCommandResult(seq, result) {
    return !!result?.stale || !isCurrentRenderSeq(seq);
}

function staleApplyResult(seq, target, detail = 'stale composer apply skipped') {
    return { status: -1, target, detail, stale: true, seq };
}

function workerFatalResult(seq, detail) {
    return { status: -1, target: 'worker', detail, fatal: true, seq };
}

function isWorkerFatalResult(result) {
    if (result?.fatal) return true;
    if (result?.target !== 'worker' || result?.status !== -1) return false;
    return /apply threw in worker|worker call rejected|memory access out of bounds|function signature mismatch|call_indirect|unreachable/i
        .test(result.detail ?? '');
}

function resetRenderer() {
    if (state.workerRendererReloading) return;
    state.workerRendererReloading = true;
    saveLastGoodBootSceneState();
    showStatus('resetting renderer', false);
    logDebug('system', 'resetting renderer from last good scene');
    const url = new URL(window.location.href);
    url.searchParams.set('recover', Date.now().toString());
    window.location.replace(url.toString());
}

function handleWorkerFatal(detail) {
    if (state.workerRendererFatal) return;
    state.workerRendererFatal = true;
    saveLastGoodBootSceneState();
    showStatus('renderer crashed; inspect log, then reset renderer', true);
    logDebug('error', `worker renderer trapped; reset renderer when ready: ${detail ?? 'unknown'}`);
    if (state.resetRendererBtn) {
        state.resetRendererBtn.disabled = false;
        state.resetRendererBtn.classList.add('attention');
    }
}

function safelyFreeWasm(M, ptr, context) {
    if (!ptr || typeof M?._free !== 'function') return null;
    try {
        M._free(ptr);
        return null;
    } catch (e) {
        return `${context}: ${e?.message ?? e}`;
    }
}

function installWorkerPhaseTap() {
    const manager = getWorkerManager();
    const worker = manager?.worker;
    if (!worker || worker.__composerPhaseTapInstalled) return;
    const originalOnMessage = worker.onmessage;
    if (typeof originalOnMessage !== 'function') return;

    worker.onmessage = function onComposerWorkerMessage(event) {
        const data = event?.data;
        const payload = data?.payload;
        if (data?.type === 'debug_log'
                && payload?.module === 'COMPOSER'
                && payload?.data?.phase) {
            logDebug('phase', `seq ${payload.data.seq ?? '?'} ${payload.data.phase}`);
        } else if (data?.type === 'composer_phase' && payload?.phase) {
            logDebug('phase', `seq ${payload.seq ?? '?'} ${payload.phase}`);
        }
        return originalOnMessage.call(this, event);
    };
    worker.__composerPhaseTapInstalled = true;
}

async function waitForWorkerManager(timeoutMs) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
        const manager = getWorkerManager();
        if (manager) {
            installWorkerPhaseTap();
            return manager;
        }
        await delay(50);
    }
    const manager = getWorkerManager();
    if (manager) installWorkerPhaseTap();
    return manager;
}

async function callApplyScene(bytes, seq) {
    // Route to the render worker first when it is active. This FastLED build
    // renders in a background worker that owns the OffscreenCanvas and runs
    // its OWN wasm module — the main-thread module's frames are never shown.
    // A scene is only visible if it reaches the worker's module. The worker's
    // composer_apply_scene message handler is injected by build_site.py
    // (patch_render_worker). Fall back to the main module only before the
    // worker activates (early boot); that scene is re-pushed on worker init.
    if (state.workerRendererFatal) {
        return workerFatalResult(seq, 'worker renderer is recovering from a prior trap');
    }

    const workerManager = getWorkerManager() ?? await waitForWorkerManager(2000);
    if (!isCurrentRenderSeq(seq)) return staleApplyResult(seq, 'none');
    if (workerManager) {
        state.workerRendererSeen = true;
        installWorkerPhaseTap();
        const buffer = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
        let response;
        try {
            response = await workerManager.sendMessageWithResponse(
                { type: 'composer_apply_scene', payload: { bytes: buffer, seq } },
                [buffer]
            );
        } catch (e) {
            // Rejected/timed-out message channel — the scene never reached the
            // worker's module (distinct from the module rejecting the bytes).
            return { status: -1, target: 'worker', detail: `worker call rejected: ${e?.message ?? e}`, seq };
        }
        if (response == null) {
            return { status: -1, target: 'worker', detail: 'worker returned no response', seq };
        }
        if (response.stale) {
            return staleApplyResult(response.seq ?? seq, 'worker', response.reason ?? 'stale composer apply skipped in worker');
        }
        if (typeof response.status !== 'number') {
            return { status: -1, target: 'worker', detail: `worker response missing status: ${formatValue(response)}`, seq };
        }
        // status may be a composer::DecodeStatus (>=0) or -1 from the worker
        // handler's guard/catch; response.reason (if any) explains a -1.
        return { status: response.status, target: 'worker', detail: response.reason ?? null, seq: response.seq ?? seq };
    }

    if (state.workerRendererSeen) {
        return workerFatalResult(seq, 'worker renderer disappeared after owning the visible canvas');
    }

    const M = getWasmModule();
    if (M && M._composer_apply_scene && M._malloc) {
        let ptr = 0;
        let enteredApply = false;
        let applyReturned = false;
        let result;
        try {
            if (!isCurrentRenderSeq(seq)) return staleApplyResult(seq, 'main');
            ptr = M._malloc(bytes.length);
            if (bytes.length > 0 && !ptr) {
                return { status: -1, target: 'main', detail: 'malloc returned null', seq };
            }
            M.HEAPU8.set(bytes, ptr);
            enteredApply = true;
            const status = typeof M._composer_apply_scene_seq === 'function'
                ? M._composer_apply_scene_seq(ptr, bytes.length, seq)
                : M._composer_apply_scene(ptr, bytes.length);
            applyReturned = true;
            result = { status, target: 'main', seq };
        } catch (e) {
            result = { status: -1, target: 'main', detail: `apply threw: ${e?.message ?? e}`, seq };
        }

        // If the wasm call trapped, the module may already be invalid. Freeing
        // through that module can mask the original trap, so only clean up
        // after calls that did not die inside wasm.
        if (!enteredApply || applyReturned) {
            const freeError = safelyFreeWasm(M, ptr, 'free after main apply failed');
            if (freeError && result.status === 0) {
                return { status: -1, target: 'main', detail: freeError, seq };
            }
            if (freeError && result.detail) {
                result.detail += `; ${freeError}`;
            }
        }
        return result;
    }

    return { status: -1, target: 'none', detail: 'no worker and no main-thread module', seq };
}

// Bloom is a ThreeJS post-processing pass on the graphics manager that lives in
// the render worker (it owns the OffscreenCanvas). The composer_set_bloom message
// handler + graphics-manager setBloomEnabled() are injected by build_site.py
// (patch_render_worker / patch_threejs_bloom_toggle). The enabled flag and
// brightness-derived strength multiplier are replayed when the worker activates.
let bloomPreference = true;

async function setWorkerBloom(enabled) {
    bloomPreference = !!enabled;
    const strengthMultiplier = bloomStrengthMultiplierForBrightness(state.displayBrightness);
    const workerManager = getWorkerManager() ?? await waitForWorkerManager(2000);
    if (!workerManager) return { ok: false, reason: 'worker not active' };
    try {
        return await workerManager.sendMessageWithResponse(
            { type: 'composer_set_bloom', payload: { enabled: bloomPreference, strengthMultiplier } }
        );
    } catch (e) {
        return { ok: false, reason: e && e.message ? e.message : String(e) };
    }
}

const STATUS_LABELS = [
    'OK', 'BAD_MAGIC', 'BAD_VERSION', 'TRUNCATED', 'UNKNOWN_TAG', 'BAD_ENUM',
];

function showStatus(text, isError) {
    if (!state.statusEl) return;
    state.statusEl.textContent = text;
    state.statusEl.className = isError ? 'status err' : 'status ok';
}

// Encode + push on the next animation frame. This keeps drag interactions
// responsive while still coalescing bursts of input events.
async function pushSceneNow(pushEvent, pushSeq) {
    if (!isCurrentRenderSeq(pushSeq)) {
        logDebug('stale', `skipped stale scene rev ${pushEvent?.revision ?? '?'} seq ${pushSeq}`);
        return;
    }
    try {
        const bytes = encodeScene(sceneForRender(sceneStore.scene));
        const result = await callApplyScene(bytes, pushSeq);
        if (isStaleCommandResult(pushSeq, result)) {
            logDebug('stale', `ignored stale scene response rev ${pushEvent?.revision ?? '?'} seq ${pushSeq} via ${result.target}`);
            return;
        }
        const status = result.status;
        if (status === 0) {
            persistBootSceneBytes(bytes);
            showStatus(`applied (${bytes.length} bytes)`, false);
            logDebug(
                'update',
                `display updated rev ${pushEvent?.revision ?? '?'} seq ${pushSeq} via ${result.target} (${bytes.length} bytes): ${formatValue(sceneSummary(sceneStore.scene))}`
            );
        } else {
            // status >= 1 is a real composer::DecodeStatus (the module
            // rejected the bytes); status -1 is a transport/guard failure
            // with no DecodeStatus — result.detail says which.
            const label = STATUS_LABELS[status] ?? `code ${status}`;
            const kind = status === -1 ? 'transport error' : 'decode error';
            showStatus(`${kind}: ${label}`, true);
            logDebug(
                'error',
                `display update failed rev ${pushEvent?.revision ?? '?'} seq ${pushSeq} via ${result.target}: ${label}`
                + (result.detail ? ` — ${result.detail}` : '')
                + ` | ${bytes.length}B [${bytesToHex(bytes)}]`
                + ` | ${formatValue(sceneSummary(sceneStore.scene))}`
            );
            if (isWorkerFatalResult(result)) {
                handleWorkerFatal(result.detail);
            }
        }
    } catch (e) {
        showStatus(`encode error: ${e.message}`, true);
        logDebug('error', `encode failed rev ${pushEvent?.revision ?? '?'}: ${e.message}`);
    }
}

// Debounce scene pushes so a continuous interaction (slider drag, held stepper)
// only applies once the user settles. Each new emission resets the timer; the
// trailing edge encodes + pushes the latest scene. Discrete edits (dropdowns,
// checkboxes, a single stepper click) apply after one quiet interval.
const PUSH_DEBOUNCE_MS = 160;

function schedulePush(seq = nextRenderSeq()) {
    state.latestSceneSeq = seq;
    const event = state.latestEvent;
    if (state.pushTimer !== null) {
        clearTimeout(state.pushTimer);
        logDebug('coalesce', `debounced latest rev ${event?.revision ?? '?'} seq ${seq}`);
    }
    state.pushTimer = setTimeout(() => {
        state.pushTimer = null;
        const pushEvent = state.latestEvent;
        const pushSeq = state.latestSceneSeq;
        void pushSceneNow(pushEvent, pushSeq);
    }, PUSH_DEBOUNCE_MS);
}

sceneStore.subscribe((event) => {
    const seq = nextRenderSeq();
    state.latestEvent = event;
    state.latestSceneSeq = seq;
    logDebug(
        'emit',
        `emit rev ${event.revision} seq ${seq}: ${event.change.path} = ${formatValue(event.change.value)}`
    );
    schedulePush(seq);
});

// ─────────────────────────────────────────────────────────────────────
// Static-config controls (re-used between pattern + transform forms)
// ─────────────────────────────────────────────────────────────────────

function renderConfigControl(c, configObj, setValue) {
    const wrap = document.createElement('label');
    wrap.className = 'param';
    const labelText = document.createElement('span');
    labelText.className = 'param-label';
    labelText.textContent = c.label ?? c.name;
    wrap.appendChild(labelText);

    if (c.kind === 'enum') {
        const sel = document.createElement('select');
        for (let i = 0; i < c.options.length; ++i) {
            const opt = document.createElement('option');
            opt.value = i; opt.textContent = c.options[i];
            sel.appendChild(opt);
        }
        sel.value = configObj[c.name] ?? c.default ?? 0;
        sel.addEventListener('change', () => {
            setValue(c.name, parseInt(sel.value, 10));
        });
        wrap.appendChild(sel);
        return wrap;
    }

    if (c.kind === 'bool') {
        const cb = document.createElement('input');
        cb.type = 'checkbox';
        cb.checked = !!(configObj[c.name] ?? c.default ?? 0);
        cb.addEventListener('change', () => {
            setValue(c.name, cb.checked ? 1 : 0);
        });
        wrap.appendChild(cb);
        return wrap;
    }

    // Palette tint mode: one wire byte (0 hue-remap, 1 colour-mask, 2 native)
    // surfaced as two linked checkboxes.
    if (c.kind === 'tintMode') {
        const mode = configObj[c.name] ?? c.default ?? 0;

        const group = document.createElement('div');
        group.className = 'tint-mode';

        const tintLabel = document.createElement('label');
        tintLabel.className = 'tint-mode-check';
        const tintCb = document.createElement('input');
        tintCb.type = 'checkbox';
        tintCb.checked = mode !== 2;   // native (2) means "don't tint with palette"
        tintLabel.appendChild(tintCb);
        tintLabel.appendChild(document.createTextNode('Tint with palette'));

        const maskLabel = document.createElement('label');
        maskLabel.className = 'tint-mode-check';
        const maskCb = document.createElement('input');
        maskCb.type = 'checkbox';
        maskCb.checked = mode === 1;   // colour-mask
        maskCb.disabled = !tintCb.checked;
        maskLabel.appendChild(maskCb);
        maskLabel.appendChild(document.createTextNode('Use offset as colour mask'));

        const commit = () => {
            maskCb.disabled = !tintCb.checked;
            let next;
            if (!tintCb.checked) next = 2;            // native
            else next = maskCb.checked ? 1 : 0;       // colour-mask : hue-remap
            setValue(c.name, next);
        };
        tintCb.addEventListener('change', commit);
        maskCb.addEventListener('change', commit);

        group.appendChild(tintLabel);
        group.appendChild(maskLabel);
        wrap.appendChild(group);
        return wrap;
    }

    const input = document.createElement('input');
    input.type = 'number';
    input.value = configObj[c.name] ?? c.default ?? 0;
    input.addEventListener('input', () => {
        const v = parseInt(input.value, 10);
        if (!Number.isNaN(v)) setValue(c.name, v);
    });
    wrap.appendChild(input);
    return wrap;
}

function renderConfigForm(configSchema, configObj, setValue) {
    const form = document.createElement('div');
    form.className = 'config-form';
    for (const c of configSchema) {
        form.appendChild(renderConfigControl(c, configObj, setValue));
    }
    return form;
}

function renderSignalsForm(signalsSchema, signalsObj, setSignal) {
    const form = document.createElement('div');
    form.className = 'signals-form';
    for (const s of signalsSchema) {
        const row = document.createElement('div');
        row.className = 'signal-row';
        const label = document.createElement('div');
        label.className = 'signal-row-label';
        label.textContent = s.name;
        row.appendChild(label);
        const slot = renderSignalSlot(
            signalsObj[s.name] ?? DEFAULT_SIGNAL(),
            (next) => setSignal(s.name, next)
        );
        row.appendChild(slot);
        form.appendChild(row);
    }
    return form;
}

// Ensure pattern.config / .signals contain entries for every schema slot
// (defaults applied) so encoding never silently uses fall-back values.
function hydrateConfig(schemaConfig, configObj) {
    for (const c of schemaConfig) {
        if (configObj[c.name] === undefined) configObj[c.name] = c.default ?? 0;
    }
}

function hydrateSignals(schemaSignals, signalsObj) {
    for (const s of schemaSignals) {
        if (!signalsObj[s.name]) signalsObj[s.name] = DEFAULT_SIGNAL();
    }
}

// ─────────────────────────────────────────────────────────────────────
// Pattern picker
// ─────────────────────────────────────────────────────────────────────

function renderPatternSection() {
    const section = document.createElement('section');
    section.className = 'panel-section';
    const h = document.createElement('h2'); h.textContent = 'Pattern'; section.appendChild(h);

    const row = document.createElement('div');
    row.className = 'pattern-picker-row';
    const sel = document.createElement('select');
    for (const [id, def] of Object.entries(PATTERNS)) {
        const opt = document.createElement('option');
        opt.value = id; opt.textContent = def.label;
        sel.appendChild(opt);
    }
    sel.value = sceneStore.scene.pattern.id;
    row.appendChild(sel);
    section.appendChild(row);

    const body = document.createElement('div'); body.className = 'pattern-body';
    section.appendChild(body);

    function rebuildBody() {
        body.innerHTML = '';
        const pattern = sceneStore.scene.pattern;
        const def = PATTERNS[pattern.id];
        hydrateConfig(def.config, pattern.config);
        hydrateSignals(def.signals, pattern.signals);
        body.appendChild(renderConfigForm(def.config, pattern.config, (name, value) => {
            mutateScene(`pattern.${pattern.id}.config.${name}`, value, () => {
                pattern.config[name] = value;
            });
        }));
        body.appendChild(renderSignalsForm(def.signals, pattern.signals, (name, next) => {
            mutateScene(`pattern.${pattern.id}.signals.${name}`, signalSummary(next), () => {
                pattern.signals[name] = next;
            });
        }));
    }

    sel.addEventListener('change', () => {
        mutateScene('pattern.id', sel.value, (scene) => {
            scene.pattern = { id: sel.value, config: {}, signals: {} };
        });
        rebuildBody();
    });

    rebuildBody();
    return section;
}

// ─────────────────────────────────────────────────────────────────────
// Transform list (add / remove / up / down)
// ─────────────────────────────────────────────────────────────────────

function renderTransformsSection() {
    const section = document.createElement('section');
    section.className = 'panel-section';
    const h = document.createElement('h2'); h.textContent = 'Transforms'; section.appendChild(h);

    const list = document.createElement('div'); list.className = 'transform-list';
    section.appendChild(list);

    const addRow = document.createElement('div'); addRow.className = 'add-row';
    const addSel = document.createElement('select');
    for (const [id, def] of Object.entries(TRANSFORMS)) {
        if (def.hidden) continue;
        const opt = document.createElement('option');
        opt.value = id; opt.textContent = def.label;
        addSel.appendChild(opt);
    }
    const addBtn = document.createElement('button'); addBtn.textContent = 'Add';
    addRow.appendChild(addSel); addRow.appendChild(addBtn);
    section.appendChild(addRow);

    function rebuildList() {
        list.innerHTML = '';
        sceneStore.scene.transforms.forEach((t, i) => {
            const def = TRANSFORMS[t.id];
            hydrateConfig(def.config, t.config);
            hydrateSignals(def.signals, t.signals);

            const card = document.createElement('div'); card.className = 'transform-card';
            if (t.enabled === false) card.classList.add('transform-disabled');
            const header = document.createElement('div'); header.className = 'transform-header';
            const title = document.createElement('span'); title.className = 'transform-title';
            title.textContent = `${i + 1}. ${def.label}`;
            header.appendChild(title);

            // Enabled toggle: unchecking drops this transform from the render
            // push (sceneForRender) without touching its params, so its effect
            // can be compared on/off instantly.
            const enableWrap = document.createElement('label');
            enableWrap.className = 'transform-enable';
            const enableCb = document.createElement('input');
            enableCb.type = 'checkbox';
            enableCb.checked = t.enabled !== false;
            enableCb.addEventListener('change', () => {
                mutateScene(`transforms.${i}.${t.id}.enabled`, enableCb.checked, () => {
                    t.enabled = enableCb.checked;
                });
                card.classList.toggle('transform-disabled', !enableCb.checked);
            });
            enableWrap.appendChild(enableCb);
            enableWrap.appendChild(Object.assign(
                document.createElement('span'), { textContent: 'Enabled' }));
            header.appendChild(enableWrap);

            const btnUp   = document.createElement('button'); btnUp.textContent   = '↑';
            const btnDown = document.createElement('button'); btnDown.textContent = '↓';
            const btnRm   = document.createElement('button'); btnRm.textContent   = '✕';
            btnUp.disabled   = (i === 0);
            btnDown.disabled = (i === sceneStore.scene.transforms.length - 1);
            btnUp.addEventListener('click', () => {
                mutateScene('transforms.reorder', `up:${i}`, (scene) => {
                    [scene.transforms[i - 1], scene.transforms[i]] =
                        [scene.transforms[i], scene.transforms[i - 1]];
                });
                rebuildList();
            });
            btnDown.addEventListener('click', () => {
                mutateScene('transforms.reorder', `down:${i}`, (scene) => {
                    [scene.transforms[i + 1], scene.transforms[i]] =
                        [scene.transforms[i], scene.transforms[i + 1]];
                });
                rebuildList();
            });
            btnRm.addEventListener('click', () => {
                mutateScene('transforms.remove', `${i}:${t.id}`, (scene) => {
                    scene.transforms.splice(i, 1);
                });
                rebuildList();
            });
            header.appendChild(btnUp); header.appendChild(btnDown); header.appendChild(btnRm);
            card.appendChild(header);

            card.appendChild(renderConfigForm(def.config, t.config, (name, value) => {
                mutateScene(`transforms.${i}.${t.id}.config.${name}`, value, () => {
                    t.config[name] = value;
                });
            }));
            card.appendChild(renderSignalsForm(def.signals, t.signals, (name, next) => {
                mutateScene(`transforms.${i}.${t.id}.signals.${name}`, signalSummary(next), () => {
                    t.signals[name] = next;
                });
            }));
            list.appendChild(card);
        });
    }

    addBtn.addEventListener('click', () => {
        mutateScene('transforms.add', addSel.value, (scene) => {
            scene.transforms.push({ id: addSel.value, config: {}, signals: {} });
        });
        rebuildList();
    });

    rebuildList();
    return section;
}

// ─────────────────────────────────────────────────────────────────────
// Palette + save/load + status
// ─────────────────────────────────────────────────────────────────────

function renderTopSection() {
    const section = document.createElement('section'); section.className = 'panel-section';

    const palRow = document.createElement('div'); palRow.className = 'top-row';
    palRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Palette' }));
    const palSel = document.createElement('select');
    for (const p of PALETTES) {
        const o = document.createElement('option'); o.value = p.id; o.textContent = p.name;
        palSel.appendChild(o);
    }
    palSel.value = sceneStore.scene.paletteId;
    palSel.addEventListener('change', () => {
        const paletteId = parseInt(palSel.value, 10);
        mutateScene('paletteId', paletteId, (scene) => {
            scene.paletteId = paletteId;
        });
    });
    palRow.appendChild(palSel);
    section.appendChild(palRow);

    // Display geometry is selected at WASM startup. Runtime display rebuilds
    // can leave the generated FastLED worker with stale display/screenmap
    // pointers, so changing geometry stores the current scene and reloads.
    const dispRow = document.createElement('div'); dispRow.className = 'top-row';
    dispRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Display' }));
    const dispSel = document.createElement('select');
    for (const d of DISPLAYS) {
        const o = document.createElement('option'); o.value = d.id; o.textContent = d.name;
        dispSel.appendChild(o);
    }
    dispSel.value = state.displayId;
    dispSel.addEventListener('change', () => {
        const which = parseInt(dispSel.value, 10);
        if (which === state.displayId) return;
        state.displayId = which;
        saveLastGoodBootSceneState();
        const seq = nextRenderSeq();
        const label = DISPLAYS.find((d) => d.id === which)?.name ?? which;
        showStatus(`switching display: ${label}`, false);
        logDebug('display', `reloading for ${label} seq ${seq}`);
        reloadForDisplay(which);
    });
    dispRow.appendChild(dispSel);
    section.appendChild(dispRow);

    const brightnessRow = document.createElement('div'); brightnessRow.className = 'top-row';
    brightnessRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Brightness' }));
    const brightnessSlider = document.createElement('input');
    brightnessSlider.type = 'range';
    brightnessSlider.min = '0';
    brightnessSlider.max = '255';
    brightnessSlider.step = '1';
    brightnessSlider.value = String(state.displayBrightness);
    brightnessSlider.disabled = !state.displayConfigApiAvailable || state.displayBrightnessBusy || state.deployBusy;
    brightnessSlider.title = state.deployBusy
        ? 'Wait for deploy to finish before changing MCU brightness'
        : state.displayConfigApiAvailable
        ? 'Saved into build/display_config.json and applied on the next MCU deploy'
        : 'Run web/serve.sh locally to save MCU brightness';
    const brightnessValue = document.createElement('span');
    brightnessValue.className = 'brightness-value';
    brightnessValue.textContent = String(state.displayBrightness);
    brightnessSlider.addEventListener('input', () => {
        const brightness = clampDisplayBrightness(brightnessSlider.value);
        state.displayBrightness = brightness;
        brightnessValue.textContent = String(brightness);
        void setWorkerBloom(bloomPreference);
        scheduleDisplayBrightnessSave(brightness);
    });
    brightnessRow.appendChild(brightnessSlider);
    brightnessRow.appendChild(brightnessValue);
    section.appendChild(brightnessRow);

    const ioRow = document.createElement('div'); ioRow.className = 'top-row action-row';
    const saveBtn = document.createElement('button'); saveBtn.textContent = 'Download .psc';
    const savePlaylistBtn = document.createElement('button'); savePlaylistBtn.textContent = 'Add to playlist';
    const importPlaylistBtn = document.createElement('button'); importPlaylistBtn.textContent = 'Import to playlist';
    const loadBtn = document.createElement('button'); loadBtn.textContent = 'Load .psc';
    const resetBtn = document.createElement('button'); resetBtn.textContent = 'Reset renderer';
    resetBtn.title = 'Reload the renderer from the last successfully applied scene';
    state.resetRendererBtn = resetBtn;
    if (state.workerRendererFatal) {
        resetBtn.classList.add('attention');
    }
    const fileIn  = document.createElement('input');
    fileIn.type = 'file'; fileIn.accept = '.psc,application/octet-stream';
    fileIn.style.display = 'none';
    const importIn = document.createElement('input');
    importIn.type = 'file'; importIn.accept = '.psc,application/octet-stream';
    importIn.multiple = true;
    importIn.style.display = 'none';
    saveBtn.addEventListener('click', () => {
        try {
            const renderScene = sceneForRender(sceneStore.scene);
            const bytes = encodeScene(renderScene);
            const blob = new Blob([bytes], { type: 'application/octet-stream' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url; a.download = pscDownloadFilename(renderScene);
            document.body.appendChild(a); a.click(); document.body.removeChild(a);
            URL.revokeObjectURL(url);
        } catch (e) {
            showStatus(`save error: ${e.message}`, true);
        }
    });
    savePlaylistBtn.disabled = !state.playlistApiAvailable || state.playlistBusy || state.deployBusy;
    if (!state.playlistApiAvailable) {
        savePlaylistBtn.title = state.playlistApiChecked
            ? 'Run web/serve.sh locally to save directly into build/psc'
            : 'Checking local playlist API';
    } else if (state.deployBusy) {
        savePlaylistBtn.title = 'Wait for deploy to finish before changing build/psc';
    } else if (state.sceneSourceName && isSafePlaylistFilename(state.sceneSourceName)) {
        savePlaylistBtn.title = `Add build/psc/${state.sceneSourceName}`;
    } else if (state.sceneSourceName) {
        savePlaylistBtn.title = 'Save with a generated build/psc filename';
    } else {
        savePlaylistBtn.title = 'Save a new .psc file into build/psc';
    }
    savePlaylistBtn.addEventListener('click', () => {
        void saveCurrentSceneToPlaylist();
    });
    importPlaylistBtn.disabled = !state.playlistApiAvailable || state.playlistBusy || state.deployBusy;
    if (!state.playlistApiAvailable) {
        importPlaylistBtn.title = state.playlistApiChecked
            ? 'Run web/serve.sh locally to import directly into build/psc'
            : 'Checking local playlist API';
    } else if (state.deployBusy) {
        importPlaylistBtn.title = 'Wait for deploy to finish before changing build/psc';
    } else {
        importPlaylistBtn.title = 'Import one or more .psc files into build/psc';
    }
    importPlaylistBtn.addEventListener('click', () => importIn.click());
    importIn.addEventListener('change', () => {
        const files = Array.from(importIn.files ?? []);
        importIn.value = '';
        void importFilesToPlaylist(files);
    });
    loadBtn.addEventListener('click', () => fileIn.click());
    fileIn.addEventListener('change', () => {
        const f = fileIn.files?.[0]; if (!f) return;
        const reader = new FileReader();
        reader.onload = (ev) => {
            let bytes = null;
            try {
                bytes = new Uint8Array(ev.target.result);
                const decoded = decodeScene(bytes);
                sceneStore.setScene(decoded, {
                    path: 'scene.load',
                    value: sceneSummary(decoded),
                });
                state.sceneSourceName = f.name;
                rebuildPanel();           // rerender the whole panel
            } catch (e) {
                showStatus(`load error: ${e.message}`, true);
                const size = bytes ? bytes.length : (f.size ?? '?');
                const hexHead = bytes
                    ? Array.from(bytes.slice(0, 16), (b) => b.toString(16).padStart(2, '0')).join(' ')
                    : '(unread)';
                console.error(
                    `[composer] failed to load .psc "${f.name}" (${size} bytes): ${e.message}\n` +
                    `first bytes: ${hexHead}`,
                    e
                );
            }
        };
        reader.onerror = () => {
            const err = reader.error;
            showStatus(`load error: ${err?.message ?? 'file read failed'}`, true);
            console.error(`[composer] FileReader failed to read "${f.name}":`, err);
        };
        reader.readAsArrayBuffer(f);
    });
    resetBtn.addEventListener('click', resetRenderer);
    // Bloom on/off: toggles the ThreeJS bloom post-processing pass in the render
    // worker's graphics manager (see setWorkerBloom).
    const bloomLabel = document.createElement('label');
    bloomLabel.style.display = 'flex';
    bloomLabel.style.alignItems = 'center';
    bloomLabel.style.gap = '6px';
    const bloomChk = document.createElement('input');
    bloomChk.type = 'checkbox';
    bloomChk.checked = bloomPreference;
    bloomChk.addEventListener('change', () => {
        setWorkerBloom(bloomChk.checked).then((res) => {
            if (res && res.ok === false) {
                showStatus(`bloom: ${res.reason || 'failed'}`, true);
            } else {
                showStatus(`bloom ${bloomChk.checked ? 'on' : 'off'}`, false);
            }
        });
    });
    bloomLabel.appendChild(bloomChk);
    bloomLabel.appendChild(Object.assign(document.createElement('span'), { textContent: 'Bloom' }));
    ioRow.appendChild(loadBtn);
    ioRow.appendChild(saveBtn);
    ioRow.appendChild(savePlaylistBtn);
    ioRow.appendChild(importPlaylistBtn);
    ioRow.appendChild(resetBtn);
    ioRow.appendChild(bloomLabel);
    ioRow.appendChild(fileIn);
    ioRow.appendChild(importIn);
    section.appendChild(ioRow);

    state.statusEl = document.createElement('div');
    state.statusEl.className = 'status';
    state.statusEl.textContent = 'idle';
    section.appendChild(state.statusEl);

    const tabs = document.createElement('div');
    tabs.className = 'composer-tabs';
    for (const tab of [
        { id: 'new', label: 'New composition' },
        { id: 'playlist', label: 'Playlist' },
    ]) {
        const btn = document.createElement('button');
        btn.type = 'button';
        btn.textContent = tab.label;
        btn.className = state.composerTab === tab.id ? 'active' : '';
        btn.addEventListener('click', () => {
            if (state.composerTab === tab.id) return;
            state.composerTab = tab.id;
            rebuildPanel();
            if (tab.id === 'playlist') {
                if (!state.playlistApiChecked) {
                    void refreshPlaylistCapability({ rebuild: true });
                } else if (state.playlistApiAvailable) {
                    void refreshPlaylistItems({ rebuild: true });
                }
                if (state.deployApiAvailable) {
                    void refreshDeployData({ rebuild: true });
                }
            }
        });
        tabs.appendChild(btn);
    }
    section.appendChild(tabs);

    if (state.composerTab === 'new') {
        // PatternFlow ready-made presets: load a bare pf pattern + its transform
        // stack in one click (mirrors the native pf*Preset recipes).
        const presetRow = document.createElement('div'); presetRow.className = 'top-row';
        presetRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'PF preset' }));
        const presetSel = document.createElement('select');
        for (const p of PF_PRESETS) {
            const o = document.createElement('option'); o.value = p.id; o.textContent = p.label;
            presetSel.appendChild(o);
        }
        if (!PF_PRESETS.some((p) => p.id === state.presetSelected)) {
            state.presetSelected = PF_PRESETS[0]?.id ?? '';
        }
        presetSel.value = state.presetSelected;
        const presetBtn = document.createElement('button'); presetBtn.textContent = 'Load';
        const loadSelectedPreset = () => {
            const preset = PF_PRESETS.find((p) => p.id === presetSel.value);
            if (!preset) return;
            state.presetSelected = preset.id;
            const nextScene = preset.scene();
            sceneStore.setScene(nextScene, {
                path: 'scene.preset',
                value: preset.label,
            });
            state.sceneSourceName = '';
            rebuildPanel();
        };
        presetSel.addEventListener('change', loadSelectedPreset);
        presetBtn.addEventListener('click', loadSelectedPreset);
        presetRow.appendChild(presetSel); presetRow.appendChild(presetBtn);
        section.appendChild(presetRow);
    }

    if (state.composerTab === 'playlist') {
        const playlistRow = document.createElement('div'); playlistRow.className = 'top-row';
        playlistRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Files' }));
        if (!state.playlistApiChecked) {
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: 'checking local playlist API',
            }));
        } else if (!state.playlistApiAvailable) {
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: 'available only from web/serve.sh',
            }));
        } else if (state.playlistItems.length === 0) {
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: 'no saved compositions in build/psc',
            }));
        } else {
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: `${state.playlistItems.length} composition${state.playlistItems.length === 1 ? '' : 's'} in build/psc`,
            }));
        }
        const refreshBtn = document.createElement('button');
        refreshBtn.textContent = 'Refresh';
        refreshBtn.disabled = !state.playlistApiAvailable || state.playlistBusy || state.deployBusy;
        refreshBtn.addEventListener('click', () => {
            void refreshPlaylistItems({ rebuild: true });
        });
        playlistRow.appendChild(refreshBtn);
        section.appendChild(playlistRow);

        if (state.playlistApiAvailable && state.playlistItems.length > 0) {
            const playlistList = document.createElement('div');
            playlistList.className = 'playlist-list';
            for (const file of state.playlistItems) {
                const item = document.createElement('div');
                item.className = file.name === state.playlistSelected
                    ? 'playlist-item active'
                    : 'playlist-item';
                const loadSavedBtn = document.createElement('button');
                loadSavedBtn.type = 'button';
                loadSavedBtn.textContent = file.name;
                loadSavedBtn.title = `Load ${file.name}`;
                loadSavedBtn.className = 'playlist-load-btn';
                loadSavedBtn.disabled = state.playlistBusy || state.deployBusy;
                loadSavedBtn.addEventListener('click', () => {
                    void loadPlaylistScene(file.name);
                });
                const deleteSavedBtn = document.createElement('button');
                deleteSavedBtn.type = 'button';
                deleteSavedBtn.textContent = '×';
                deleteSavedBtn.className = 'playlist-delete-btn';
                deleteSavedBtn.title = `Delete ${file.name}`;
                deleteSavedBtn.setAttribute('aria-label', `Delete ${file.name}`);
                deleteSavedBtn.disabled = state.playlistBusy || state.deployBusy;
                deleteSavedBtn.addEventListener('click', () => {
                    void deletePlaylistScene(file.name);
                });
                item.appendChild(loadSavedBtn);
                item.appendChild(deleteSavedBtn);
                playlistList.appendChild(item);
            }
            section.appendChild(playlistList);
        }

        const targetRow = document.createElement('div'); targetRow.className = 'top-row';
        targetRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'MCU' }));
        if (!state.deployApiAvailable) {
            targetRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: state.playlistApiAvailable ? 'deploy unavailable' : 'available only from web/serve.sh',
            }));
        } else {
            const targetSel = document.createElement('select');
            const placeholder = document.createElement('option');
            placeholder.value = '';
            placeholder.textContent = 'Choose MCU target';
            targetSel.appendChild(placeholder);
            for (const target of state.deployTargets) {
                const o = document.createElement('option');
                o.value = target.id;
                o.textContent = target.label ?? target.id;
                targetSel.appendChild(o);
            }
            targetSel.value = state.deployTarget;
            targetSel.disabled = state.deployBusy || state.playlistBusy;
            targetSel.addEventListener('change', () => {
                state.deployTarget = targetSel.value;
                rebuildPanel();
            });
            targetRow.appendChild(targetSel);
        }
        const detectBtn = document.createElement('button');
        detectBtn.textContent = state.deployDetectBusy ? 'Detecting' : 'Detect';
        detectBtn.disabled = !state.deployApiAvailable || state.deployBusy || state.playlistBusy || state.deployDetectBusy;
        detectBtn.title = state.deployBusy
            ? 'Device detection is disabled while deploying'
            : 'Detect connected serial devices';
        detectBtn.addEventListener('click', () => {
            void refreshDeployDevices({ rebuild: true });
        });
        targetRow.appendChild(detectBtn);
        section.appendChild(targetRow);

        const deployRow = document.createElement('div'); deployRow.className = 'top-row';
        deployRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Deploy' }));
        const deployBtn = document.createElement('button');
        deployBtn.textContent = state.deployBusy ? 'Deploying' : 'Deploy to MCU';
        deployBtn.disabled = !state.deployApiAvailable
            || state.deployBusy
            || state.playlistBusy
            || state.deployDetectBusy
            || state.displayBrightnessBusy
            || state.playlistItems.length === 0
            || !state.deployTarget;
        if (state.playlistItems.length === 0) {
            deployBtn.title = 'Save at least one composition to build/psc first';
        } else if (state.displayBrightnessBusy) {
            deployBtn.title = 'Wait for brightness to save';
        } else if (state.deployDetectBusy) {
            deployBtn.title = 'Wait for device detection to finish';
        } else if (!state.deployTarget) {
            deployBtn.title = 'Choose an MCU target first';
        } else {
            deployBtn.title = `Build and upload ${state.playlistItems.length} playlist composition${state.playlistItems.length === 1 ? '' : 's'} to ${deployTargetLabel(state.deployTarget)}`;
        }
        deployBtn.addEventListener('click', () => {
            void startDeployToMcu();
        });
        deployRow.appendChild(deployBtn);

        const deployInfo = document.createElement('span');
        deployInfo.className = 'muted-note deploy-status';
        if (state.deployBusy) {
            deployInfo.textContent = state.deployStatus || 'deploying';
        } else if (state.deployError) {
            deployInfo.textContent = state.deployError;
        } else if (state.deployDeviceError) {
            deployInfo.textContent = state.deployDeviceError;
        } else if (state.deployDevices.length > 0) {
            deployInfo.textContent = `${state.deployDevices.length} device(s) detected`;
        } else if (state.deployApiAvailable) {
            deployInfo.textContent = 'manual target selection allowed';
        }
        if (deployInfo.textContent) deployRow.appendChild(deployInfo);
        section.appendChild(deployRow);
    }

    return section;
}

// ─────────────────────────────────────────────────────────────────────
// Panel mount
// ─────────────────────────────────────────────────────────────────────

let panelEl = null;
let panelContentEl = null;
let rendererResizeFrame = null;

const MIN_PANEL_WIDTH = 320;
const MIN_RENDERER_WIDTH = 280;

function clampPanelWidth(width) {
    const max = Math.max(MIN_PANEL_WIDTH, window.innerWidth - MIN_RENDERER_WIDTH);
    return Math.min(Math.max(width, MIN_PANEL_WIDTH), max);
}

function requestRendererResize() {
    if (rendererResizeFrame !== null) return;
    rendererResizeFrame = requestAnimationFrame(() => {
        rendererResizeFrame = null;
        window.dispatchEvent(new Event('resize'));
    });
}

function setPanelWidth(width, options = {}) {
    if (!panelEl) return;
    const clamped = clampPanelWidth(width);
    panelEl.style.width = `${clamped}px`;
    document.documentElement.style.setProperty('--composer-panel-width', `${clamped}px`);
    if (options.notifyRenderer) requestRendererResize();
}

function mountResizeHandle() {
    const handle = document.createElement('div');
    handle.className = 'composer-resize-handle';
    handle.setAttribute('role', 'separator');
    handle.setAttribute('aria-orientation', 'vertical');
    handle.setAttribute('aria-label', 'Resize composer panel');

    handle.addEventListener('pointerdown', (event) => {
        if (!panelEl) return;
        event.preventDefault();
        handle.setPointerCapture(event.pointerId);
        handle.classList.add('dragging');
        document.body.classList.add('composer-resizing');

        const startX = event.clientX;
        const startWidth = panelEl.getBoundingClientRect().width;

        const onPointerMove = (moveEvent) => {
            setPanelWidth(startWidth + startX - moveEvent.clientX, { notifyRenderer: true });
        };
        const onPointerUp = (upEvent) => {
            handle.releasePointerCapture(upEvent.pointerId);
            handle.classList.remove('dragging');
            document.body.classList.remove('composer-resizing');
            requestRendererResize();
            handle.removeEventListener('pointermove', onPointerMove);
            handle.removeEventListener('pointerup', onPointerUp);
            handle.removeEventListener('pointercancel', onPointerUp);
        };

        handle.addEventListener('pointermove', onPointerMove);
        handle.addEventListener('pointerup', onPointerUp);
        handle.addEventListener('pointercancel', onPointerUp);
    });

    panelEl.appendChild(handle);
}

function rebuildPanel() {
    if (!panelContentEl) return;
    panelContentEl.innerHTML = '';
    const title = document.createElement('h1');
    title.textContent = 'Composer';
    panelContentEl.appendChild(title);
    panelContentEl.appendChild(renderTopSection());
    panelContentEl.appendChild(renderPatternSection());
    panelContentEl.appendChild(renderTransformsSection());
}

function mountPanel() {
    panelEl = document.createElement('aside');
    panelEl.id = 'composer-panel';
    mountResizeHandle();
    panelContentEl = document.createElement('div');
    panelContentEl.id = 'composer-panel-content';
    panelEl.appendChild(panelContentEl);
    document.body.appendChild(panelEl);
    setPanelWidth(window.innerWidth / 2, { notifyRenderer: true });
    window.addEventListener('resize', () => {
        setPanelWidth(panelEl.getBoundingClientRect().width);
    });
    mountDebugConsole();
    rebuildPanel();
    void refreshPlaylistCapability({ rebuild: true });
    rePushOnWorkerInit();
    // Push the initial scene once WASM is ready.
    sceneStore.notify({ path: 'scene.initial', value: sceneSummary(sceneStore.scene) });
}

// If the render worker activates AFTER the first scene was applied to the
// main-thread module, that scene never reached the worker's module (which
// owns the canvas), so the display keeps showing the C++ default. Re-push the
// current scene once the worker comes up so it takes over rendering it.
function rePushOnWorkerInit() {
    const events = window.fastLEDEvents;
    if (!events || typeof events.on !== 'function') {
        setTimeout(rePushOnWorkerInit, 100);
        return;
    }

    let synced = false;
    let pollTimer = null;

    const syncWorker = async (reason) => {
        if (synced) return;
        synced = true;
        if (pollTimer !== null) {
            clearInterval(pollTimer);
            pollTimer = null;
        }
        logDebug('worker', `${reason} - syncing scene`);
        const sceneSeq = nextRenderSeq();
        state.latestSceneSeq = sceneSeq;
        await pushSceneNow(state.latestEvent, sceneSeq);
        await setWorkerBloom(bloomPreference);
    };

    if (getWorkerManager()) {
        void syncWorker('render worker already active');
        return;
    }

    events.on('worker:initialized', () => {
        void syncWorker('render worker active');
    });
    window.addEventListener('polar:worker-ready', () => {
        void syncWorker('render worker ready hook');
    }, { once: true });

    // The FastLED worker can finish booting before the composer panel has
    // registered the event listener during full-page renderer reloads. Poll for
    // the same condition briefly so a missed one-shot event still replays the
    // current scene into the worker-owned canvas.
    const started = performance.now();
    pollTimer = setInterval(() => {
        if (synced) return;
        if (getWorkerManager()) {
            void syncWorker('render worker detected after composer boot');
            return;
        }
        if (performance.now() - started > 10000) {
            clearInterval(pollTimer);
            pollTimer = null;
            logDebug('worker', 'render worker not active; staying on main renderer');
        }
    }, 100);
}

// Wait for FastLED's runtime to expose getFastLEDController() with our
// composer exports attached to the Emscripten module.
function composerBridgeReady() {
    if (getWorkerManager()) return true;

    const M = getWasmModule();
    return !!(M && typeof M._composer_apply_scene === 'function'
          && typeof M._composer_set_display === 'function'
          && typeof M._malloc === 'function'
          && M.HEAPU8);
}

function waitForModule(cb) {
    const check = () => {
        if (composerBridgeReady()) {
            cb();
        } else {
            setTimeout(check, 100);
        }
    };
    check();
}

function bootComposer() {
    waitForModule(mountPanel);
}

window.__polarComposerBeforeGraphicsModeSwitch = saveLastGoodBootSceneState;
window.addEventListener('beforeunload', saveLastGoodBootSceneState);

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', bootComposer);
} else {
    bootComposer();
}
