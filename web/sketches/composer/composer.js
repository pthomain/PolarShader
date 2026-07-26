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
    PALETTES, PATTERNS, TRANSFORMS, DEFAULT_SCENE, DEFAULT_SIGNAL,
    DEFAULT_PALETTE_TRANSFORM,
} from './schema.js';
import { encodeScene, decodeScene } from './codec.js';
import { decodePds } from './pds-codec.js';
import { renderSignalSlot } from './signal-editor.js';
import {
    PALETTE_ENTRY_COUNT, cosinePaletteEntries, defaultCosineCoeffs,
    paletteGradientCss, paletteSlug,
} from './palette.js';

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
// Static-build fallback for the palette library when the local server API
// (/api/palettes) is unreachable — mirrors the playlist localStorage pattern.
const PALETTE_LIBRARY_STORAGE_KEY = 'polarComposerPaletteLibrary:v1';

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

// Display id for a runtime-loaded .PDS display. Must match DISPLAY_LOADED in
// composer.ino. Built-ins are 0..4; a custom display gets this sentinel so the
// selector and raster-capability checks can tell it apart.
const DISPLAY_LOADED = 5;
const LOADED_DISPLAY_STORAGE_KEY = 'polarComposerLoadedDisplay:v1';

// Restore a previously-loaded .PDS display across reloads. Decodes the stored
// bytes so raster capability (hasRaster) is known before the WASM module boots.
function readLoadedDisplayState() {
    try {
        const raw = sessionStorage.getItem(LOADED_DISPLAY_STORAGE_KEY);
        const saved = raw ? JSON.parse(raw) : null;
        if (!Array.isArray(saved?.bytes)) return null;
        const bytes = new Uint8Array(saved.bytes);
        const spec = decodePds(bytes);
        return {
            bytes,
            name: spec.name || saved.name || 'custom',
            sourceKind: spec.sourceKind ?? 0,
            hasRaster: Boolean(spec.raster),
        };
    } catch {
        return null;
    }
}

const loadedDisplayState = readLoadedDisplayState();

function displayIdFromParam(raw) {
    if (raw === '1' || raw === 'round' || raw === 'polar') return 1;
    if (raw === '2' || raw === 'fabric32x8') return 2;
    if (raw === '3' || raw === 'smartmatrix' || raw === 'matrix128') return 3;
    if (raw === '4' || raw === 'fibonacci') return 4;
    return 0;
}

function initialDisplayId() {
    const params = new URLSearchParams(window.location.search);
    return displayIdFromParam(params.get('display') ?? params.get('ps_display'));
}

const state = {
    pushTimer: null,             // debounce timer handle for scene pushes
    latestEvent: null,           // most-recent scene-store emission
    debugLogEl: null,            // FastLED-side debug console
    displayId: loadedDisplayState ? DISPLAY_LOADED : initialDisplayId(), // active display geometry (0 = fabric, 1 = round, 2 = fabric 32x8, 3 = SmartMatrix, 5 = loaded .pds)
    loadedDisplay: loadedDisplayState, // { bytes, name, sourceKind, hasRaster } when a custom .pds is active
    renderSeq: 0,                // monotonic command generation for scene pushes / reloads
    latestSceneSeq: 0,           // sequence assigned to the newest scene-store emission
    lastGoodBootBytes: bootSceneState?.bytes ?? null,
    workerRendererSeen: false,
    workerRendererFatal: false,
    workerRendererReloading: false,
    resetRendererBtn: null,
    composerTab: 'new',
    paletteApiAvailable: false,   // /api/palettes reachable (local server)
    paletteLibrary: [],           // [{ name: slug, data: paletteJson }]
    paletteGenerator: null,       // lazily { coeffs, name, startFrom, liveApply, link, savedSnapshot }
    sceneSourceName: '',
    playlistApiChecked: false,
    playlistApiAvailable: false,
    playlistLocalMode: false,
    localPlaylistBytes: null,
    playlistItems: [],
    playlistSelected: '',
    playlistActiveName: '',
    playlistBaselineName: '',
    playlistBaselineBytes: null,
    playlistDirty: false,
    playlistBusy: false,
    displayConfigApiAvailable: false,
    displayBrightness: 255,
    displayBrightnessSaved: false,
    displayBrightnessBusy: false,
    displayBrightnessSaveTimer: null,
    diffusionPercent: 100,
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
    { id: 0, name: 'Fabric (20×20 matrix)', matrix: true },
    { id: 1, name: 'Round (241-pixel radial)', matrix: false },
    { id: 2, name: 'Matrix (32×8 matrix)', matrix: true },
    { id: 3, name: 'SmartMatrix (128×128 HUB75)', matrix: true },
    { id: 4, name: 'Fibonacci (324-pixel phyllotaxis)', matrix: false },
];

// Golden-angle ("Fibonacci") vortex twist, as a plain constant-signal permille.
// The vortex maps a 0..1000 permille constant onto a bipolar [-1,1] strength,
// so 500 is neutral and a ±0.381966-turn (137.5°) swirl at the rim lands at
// 691 (clockwise) and 309 (counter-clockwise). Web-only convenience — it emits
// an ordinary constant signal, so the .psc wire format is unchanged.
const VORTEX_FIBONACCI_CW_PERMILLE = 691;
const VORTEX_FIBONACCI_CCW_PERMILLE = 309;

const DEPLOY_POLL_MS = 1000;
const DEPLOY_DONE_STATUSES = new Set(['succeeded', 'failed', 'timed_out']);
const DEBUG_LOG_LIMIT = 500;
const BRIGHTNESS_SAVE_DEBOUNCE_MS = 250;
const BLOOM_BASE_BRIGHTNESS = 30;
const BLOOM_MAX_BRIGHTNESS = 255;
const BLOOM_BASE_MULTIPLIER = 1;
const BLOOM_MAX_MULTIPLIER = 3;
const DIFFUSION_MIN_PERCENT = 0;
const DIFFUSION_MAX_PERCENT = 100;
const DIFFUSION_BASE_SCALE = 2.25;
const DIFFUSION_MIN_SCALE_FACTOR = 0.25;

function displayUrlValue(which) {
    if (which === 1) return 'round';
    if (which === 2) return 'fabric32x8';
    if (which === 3) return 'smartmatrix';
    if (which === 4) return 'fibonacci';
    return 'fabric';
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

function patternDomain(patternId) {
    return PATTERNS[patternId]?.domain ?? 'uv';
}

const PATTERN_DOMAIN_LABELS = {
    uv: 'UV',
    raster: 'Raster Grid',
};

function displaySupportsGrid(displayId) {
    if (displayId === DISPLAY_LOADED) return state.loadedDisplay?.hasRaster === true;
    return DISPLAYS.find((display) => display.id === displayId)?.matrix === true;
}

function availablePatternDomainsForDisplay(displayId) {
    const domains = ['uv'];
    if (displaySupportsGrid(displayId)) domains.push('raster');
    return domains;
}

function patternEntriesForDomain(domain) {
    return Object.entries(PATTERNS).filter(([, def]) => !def.hidden && (def.domain ?? 'uv') === domain);
}

function patternDropdownLabel(id, def, { includeOutput = true } = {}) {
    let label = def.label ?? id;
    if (id.startsWith('pf')) {
        label = label
            .replace(/^PF\s*[—-]\s*/i, '')
            .replace(/\s*\([^)]*\)\s*$/u, '')
            .trim();
    }
    if (!includeOutput) return label;
    const output = def.output === 'rgb' ? 'RGB' : 'Greyscale';
    return `${label} [${output}]`;
}

function comparePatternEntries([leftId, leftDef], [rightId, rightDef]) {
    return patternDropdownLabel(leftId, leftDef, { includeOutput: false })
        .localeCompare(
            patternDropdownLabel(rightId, rightDef, { includeOutput: false }),
            undefined,
            { sensitivity: 'base' }
        );
}

function sortedPatternEntriesForOutput(entries, output) {
    return entries
        .filter(([id]) => patternOutput(id) === output)
        .sort(comparePatternEntries);
}

function appendPatternDropdownOptions(select, domain) {
    const entries = patternEntriesForDomain(domain);
    for (const group of [
        { output: 'greyscale', label: 'Greyscale' },
        { output: 'rgb', label: 'RGB' },
    ]) {
        const groupEntries = sortedPatternEntriesForOutput(entries, group.output);
        if (groupEntries.length === 0) continue;
        const optgroup = document.createElement('optgroup');
        optgroup.label = group.label;
        for (const [id, def] of groupEntries) {
            const opt = document.createElement('option');
            opt.value = id;
            opt.textContent = patternDropdownLabel(id, def);
            optgroup.appendChild(opt);
        }
        select.appendChild(optgroup);
    }
}

function firstPatternIdForDomain(domain) {
    const entries = patternEntriesForDomain(domain);
    for (const output of ['greyscale', 'rgb']) {
        const first = sortedPatternEntriesForOutput(entries, output)[0];
        if (first) return first[0];
    }
    return Object.keys(PATTERNS)[0];
}

function setScenePattern(scene, patternId) {
    scene.pattern = { id: patternId, config: {}, signals: {} };
    removeIncompatibleTransforms(scene);
    ensureGreyscalePaletteTransform(scene);
}

function ensurePatternTypeAvailableForDisplay() {
    const currentPatternId = sceneStore.scene.pattern?.id;
    const currentDef = PATTERNS[currentPatternId];
    const availableDomains = availablePatternDomainsForDisplay(state.displayId);
    if (currentDef && availableDomains.includes(currentDef.domain ?? 'uv')) return;

    const fallbackDomain = availableDomains[0];
    const fallbackPatternId = firstPatternIdForDomain(fallbackDomain);
    mutateScene('pattern.domain', fallbackDomain, (scene) => {
        setScenePattern(scene, fallbackPatternId);
    });
}

function currentPatternDomainIsAvailable() {
    return availablePatternDomainsForDisplay(state.displayId).includes(
        patternDomain(sceneStore.scene.pattern?.id)
    );
}

function transformDomain(transformId) {
    return TRANSFORMS[transformId]?.transformDomain ?? 'uv';
}

function patternOutput(patternId) {
    return PATTERNS[patternId]?.output ?? 'greyscale';
}

function isGreyscalePattern(patternId) {
    return patternOutput(patternId) !== 'rgb';
}

function isTransformCompatible(patternId, transformId) {
    return patternDomain(patternId) !== 'raster' || transformDomain(transformId) === 'palette';
}

function hasPaletteTransform(scene) {
    return (scene.transforms ?? []).some((transform) => transformDomain(transform.id) === 'palette');
}

function isTransformAddable(scene, transformId) {
    if (!isTransformCompatible(scene.pattern?.id, transformId)) return false;
    return transformDomain(transformId) !== 'palette' || !hasPaletteTransform(scene);
}

function ensurePaletteTransformFirst(scene) {
    const transforms = scene.transforms ?? [];
    const paletteIndex = transforms.findIndex((transform) => (
        transformDomain(transform.id) === 'palette'
    ));

    if (paletteIndex < 0) {
        scene.transforms = [DEFAULT_PALETTE_TRANSFORM(), ...transforms];
        return;
    }

    const [paletteTransform] = transforms.splice(paletteIndex, 1);
    paletteTransform.enabled = true;
    scene.transforms = [paletteTransform, ...transforms];
}

function ensureGreyscalePaletteTransform(scene) {
    if (!isGreyscalePattern(scene.pattern?.id)) return;
    ensurePaletteTransformFirst(scene);
}

function removeIncompatibleTransforms(scene) {
    const patternId = scene.pattern?.id;
    const before = scene.transforms?.length ?? 0;
    scene.transforms = (scene.transforms ?? []).filter((t) => isTransformCompatible(patternId, t.id));
    return before - scene.transforms.length;
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

function playlistFileSupported(file) {
    return file?.supported !== false;
}

function playlistFileIssue(file) {
    if (!file) return 'playlist file is unavailable';
    if (file.error) return file.error;
    if (Number.isInteger(file.patternTag)) {
        return `unsupported pattern tag 0x${file.patternTag.toString(16).padStart(2, '0')}`;
    }
    return 'unsupported .psc file';
}

function playlistFileByName(name) {
    return state.playlistItems.find((file) => file.name === name) ?? null;
}

// True when a playlist can be browsed/loaded — either the local server API is
// running, or an in-browser playlist has been loaded on the static build.
function playlistBrowsingAvailable() {
    return state.playlistApiAvailable || state.playlistLocalMode;
}

function supportedPlaylistItems() {
    return state.playlistItems.filter(playlistFileSupported);
}

function setPlaylistBaseline(name, bytes) {
    state.playlistSelected = name;
    state.playlistBaselineName = name;
    state.playlistBaselineBytes = bytes ? new Uint8Array(bytes) : null;
    state.playlistDirty = false;
    state.sceneSourceName = name;
}

function hasSelectedPlaylistScene() {
    return Boolean(
        state.playlistSelected
        && state.playlistBaselineName === state.playlistSelected
        && state.playlistBaselineBytes
    );
}

function shouldShowSceneEditor() {
    if (state.composerTab === 'palette-generator') return false;
    return state.composerTab !== 'playlist' || hasSelectedPlaylistScene();
}

function clearPlaylistBaseline({ clearSelection = false } = {}) {
    if (clearSelection) {
        state.playlistSelected = '';
        state.playlistActiveName = '';
    }
    state.playlistBaselineName = '';
    state.playlistBaselineBytes = null;
    state.playlistDirty = false;
}

function deactivatePlaylistRow({ rebuild = true } = {}) {
    if (!state.playlistActiveName) return;
    state.playlistActiveName = '';
    if (rebuild && state.composerTab === 'playlist') {
        rebuildPanel();
    }
}

function playlistLoadButtonFromTarget(target) {
    return target instanceof Element ? target.closest('.playlist-load-btn') : null;
}

function handlePlaylistDocumentClick(event) {
    if (!state.playlistActiveName) return;
    if (playlistLoadButtonFromTarget(event.target)) return;
    deactivatePlaylistRow();
}

function handlePlaylistDocumentFocus(event) {
    if (!state.playlistActiveName) return;
    const button = playlistLoadButtonFromTarget(event.target);
    if (button?.dataset.playlistName === state.playlistActiveName) return;
    deactivatePlaylistRow();
}

function isTextEditingTarget(target) {
    if (!(target instanceof HTMLElement)) return false;
    const tag = target.tagName;
    return tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || target.isContentEditable;
}

function moveActivePlaylistSelection(delta) {
    if (
        state.composerTab !== 'playlist'
        || !state.playlistActiveName
        || state.playlistBusy
        || state.deployBusy
        || state.playlistItems.length === 0
    ) return;

    const currentIndex = state.playlistItems.findIndex((file) => file.name === state.playlistActiveName);
    if (currentIndex < 0) {
        state.playlistActiveName = '';
        rebuildPanel();
        return;
    }

    let nextIndex = currentIndex + delta;
    while (nextIndex >= 0 && nextIndex < state.playlistItems.length) {
        const nextFile = state.playlistItems[nextIndex];
        if (playlistFileSupported(nextFile)) {
            void loadPlaylistScene(nextFile.name, { activate: true });
            return;
        }
        nextIndex += delta;
    }
}

function handlePlaylistKeydown(event) {
    if (event.defaultPrevented || !state.playlistActiveName || isTextEditingTarget(event.target)) return;
    if (event.key === 'ArrowUp') {
        event.preventDefault();
        moveActivePlaylistSelection(-1);
    } else if (event.key === 'ArrowDown') {
        event.preventDefault();
        moveActivePlaylistSelection(1);
    }
}

function refreshPlaylistDirtyState({ rebuild = false } = {}) {
    let dirty = false;
    if (state.playlistBaselineName && state.playlistBaselineBytes) {
        try {
            dirty = !bytesEqual(encodeScene(sceneForRender(sceneStore.scene)), state.playlistBaselineBytes);
        } catch {
            dirty = true;
        }
    }

    if (dirty === state.playlistDirty) return;
    state.playlistDirty = dirty;
    if (rebuild && state.composerTab === 'playlist') {
        rebuildPanel();
    }
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

function clampDiffusionPercent(value) {
    const numeric = Number.parseInt(value, 10);
    if (!Number.isFinite(numeric)) return DIFFUSION_MAX_PERCENT;
    return Math.max(DIFFUSION_MIN_PERCENT, Math.min(DIFFUSION_MAX_PERCENT, numeric));
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

function diffusionScaleForPercent(value) {
    return smap(
        clampDiffusionPercent(value),
        DIFFUSION_MIN_PERCENT,
        DIFFUSION_MAX_PERCENT,
        DIFFUSION_BASE_SCALE * DIFFUSION_MIN_SCALE_FACTOR,
        DIFFUSION_BASE_SCALE
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
        if (state.displayId === 1) {
            targetId = 'seeed_xiao_rp2040_round';
        } else if (state.displayId === 2) {
            targetId = 'seeed_xiao_rp2040_matrix32x8';
        } else {
            targetId = 'seeed_xiao_rp2040_fabric';
        }
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
    const selectedFile = state.playlistSelected ? playlistFileByName(state.playlistSelected) : null;
    if (state.playlistSelected && (!selectedFile || !playlistFileSupported(selectedFile))) {
        clearPlaylistBaseline({ clearSelection: true });
    }
    const activeFile = state.playlistActiveName ? playlistFileByName(state.playlistActiveName) : null;
    if (state.playlistActiveName && (!activeFile || !playlistFileSupported(activeFile))) {
        state.playlistActiveName = '';
    }
    if (rebuild) rebuildPanel();
}

async function refreshPlaylistCapability({ rebuild = false } = {}) {
    try {
        const body = await fetchPlaylistJson('/api/capabilities');
        state.playlistApiAvailable = body.savePsc === true && body.playlist === true;
        state.deployApiAvailable = state.playlistApiAvailable && body.deploy === true;
        state.displayConfigApiAvailable = body.displayConfig === true;
        state.paletteApiAvailable = body.palettes === true;
        state.playlistApiChecked = true;
        if (state.playlistApiAvailable) {
            await refreshPlaylistItems();
        }
        await refreshPaletteLibrary();
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
        state.paletteApiAvailable = false;
        state.playlistApiChecked = true;
        state.playlistItems = [];
        clearPlaylistBaseline({ clearSelection: true });
        state.deployTargets = [];
        state.deployDevices = [];
        state.deployTarget = '';
        await refreshPaletteLibrary();
    }
    if (rebuild) rebuildPanel();
}

async function loadPlaylistScene(name, { activate = false } = {}) {
    if (!name || state.playlistBusy) return;
    const file = playlistFileByName(name);
    if (file && !playlistFileSupported(file)) {
        const issue = playlistFileIssue(file);
        logDebug('playlist', `playlist load skipped for ${name}: ${issue}`);
        showStatus(`playlist load error: ${issue}`, true);
        return;
    }
    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const bytes = await fetchPlaylistFileBytes(name);
        const decoded = decodeScene(bytes);
        setPlaylistBaseline(name, bytes);
        state.playlistActiveName = activate ? name : '';
        sceneStore.setScene(decoded, {
            path: 'scene.playlist',
            value: name,
        });
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
    if (state.playlistLocalMode) {
        const bytes = state.localPlaylistBytes?.get(name);
        if (!bytes) throw new Error(`${name} is not in the loaded playlist`);
        return bytes;
    }
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
        if (!playlistFileSupported(file)) continue;
        entries.push({
            name: file.name,
            bytes: await fetchPlaylistFileBytes(file.name),
        });
    }
    return entries;
}

function bytesToBase64(bytes) {
    let binary = '';
    const chunk = 0x8000;
    for (let i = 0; i < bytes.length; i += chunk) {
        binary += String.fromCharCode.apply(null, bytes.subarray(i, i + chunk));
    }
    return btoa(binary);
}

function base64ToBytes(b64) {
    const binary = atob(String(b64 ?? ''));
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i += 1) {
        bytes[i] = binary.charCodeAt(i);
    }
    return bytes;
}

// Playlist bundle wire format — a single JSON file wrapping every .psc as
// base64. Built by buildPlaylistBundle, parsed by parsePlaylistBundle, and
// mirrored by web/local_server.py (PLAYLIST_BUNDLE_VERSION there must match).
const PLAYLIST_BUNDLE_VERSION = 1;

function buildPlaylistBundle(entries) {
    return {
        polarshaderPlaylist: PLAYLIST_BUNDLE_VERSION,
        files: entries.map((entry) => ({
            name: entry.name,
            data: bytesToBase64(entry.bytes),
        })),
    };
}

function parsePlaylistBundle(text) {
    let bundle;
    try {
        bundle = JSON.parse(text);
    } catch {
        throw new Error('not a valid playlist file');
    }
    if (!bundle
        || bundle.polarshaderPlaylist !== PLAYLIST_BUNDLE_VERSION
        || !Array.isArray(bundle.files)
        || bundle.files.length === 0) {
        throw new Error('unrecognised or empty playlist file');
    }
    return bundle;
}

function playlistBundleFilename() {
    const now = new Date();
    const pad = (n) => String(n).padStart(2, '0');
    const stamp = `${now.getFullYear()}${pad(now.getMonth() + 1)}${pad(now.getDate())}`
        + `-${pad(now.getHours())}${pad(now.getMinutes())}${pad(now.getSeconds())}`;
    return `polarshader-playlist-${stamp}.json`;
}

async function downloadPlaylistBundle() {
    if (!playlistBrowsingAvailable() || state.playlistBusy || state.deployBusy) return;
    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const entries = await fetchPlaylistByteEntries();
        if (entries.length === 0) {
            statusText = 'playlist is empty — nothing to download';
            statusError = true;
        } else {
            const bundle = buildPlaylistBundle(entries);
            const blob = new Blob([JSON.stringify(bundle)], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = playlistBundleFilename();
            document.body.appendChild(a); a.click(); document.body.removeChild(a);
            URL.revokeObjectURL(url);
            statusText = `downloaded playlist (${entries.length} composition${entries.length === 1 ? '' : 's'})`;
            logDebug('playlist', statusText);
        }
    } catch (e) {
        statusText = `playlist download error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist download failed: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

async function readPlaylistBundleFile(file) {
    const text = await file.text();
    return { text, bundle: parsePlaylistBundle(text) };
}

function confirmPlaylistReplace(incoming) {
    const existing = state.playlistItems.length;
    const message = existing > 0
        ? `Replace all ${existing} saved composition${existing === 1 ? '' : 's'} `
            + `with ${incoming} from this playlist?\n\nThis cannot be undone.`
        : `Load ${incoming} composition${incoming === 1 ? '' : 's'} into the playlist?`;
    return window.confirm(message);
}

// Route a chosen bundle file: when the local playlist API is running, replace
// build/psc on disk via the server; otherwise (static GitHub Pages build) load
// the compositions into an in-browser playlist that can still be browsed.
async function loadPlaylistBundleFile(file) {
    if (!file || state.playlistBusy || state.deployBusy) return;
    if (state.playlistApiAvailable) {
        await replacePlaylistViaApi(file);
    } else {
        await replacePlaylistLocally(file);
    }
}

async function replacePlaylistViaApi(file) {
    let text;
    let bundle;
    try {
        ({ text, bundle } = await readPlaylistBundleFile(file));
    } catch (e) {
        showStatus(`playlist load error: ${e.message}`, true);
        return;
    }
    if (!confirmPlaylistReplace(bundle.files.length)) return;

    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const body = await fetchPlaylistJson('/api/playlist/replace', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: text,
        });
        clearPlaylistBaseline({ clearSelection: true });
        await refreshPlaylistItems();
        const replaced = body.replaced ?? bundle.files.length;
        statusText = `playlist replaced (${replaced} composition${replaced === 1 ? '' : 's'})`;
        logDebug('playlist', statusText);
    } catch (e) {
        statusText = `playlist replace error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist replace failed: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

async function replacePlaylistLocally(file) {
    let bundle;
    try {
        ({ bundle } = await readPlaylistBundleFile(file));
    } catch (e) {
        showStatus(`playlist load error: ${e.message}`, true);
        return;
    }

    // Decode + validate every entry before mutating state, mirroring the
    // server's validate-before-replace behaviour.
    const items = [];
    const bytesByName = new Map();
    for (const entry of bundle.files) {
        const name = String(entry?.name ?? '').trim();
        if (!isSafePlaylistFilename(name)) {
            showStatus(`playlist load error: invalid filename "${entry?.name ?? ''}"`, true);
            return;
        }
        if (bytesByName.has(name)) {
            showStatus(`playlist load error: duplicate composition "${name}"`, true);
            return;
        }
        let bytes;
        try {
            bytes = base64ToBytes(entry.data);
        } catch {
            showStatus(`playlist load error: ${name} is not valid base64`, true);
            return;
        }
        let error;
        try {
            decodeScene(bytes);
        } catch (e) {
            error = e.message;
        }
        bytesByName.set(name, bytes);
        items.push(error ? { name, supported: false, error } : { name, supported: true });
    }

    if (!confirmPlaylistReplace(items.length)) return;

    state.localPlaylistBytes = bytesByName;
    state.playlistLocalMode = true;
    state.playlistItems = items;
    clearPlaylistBaseline({ clearSelection: true });
    rebuildPanel();
    showStatus(`playlist loaded (${items.length} composition${items.length === 1 ? '' : 's'})`, false);
    logDebug('playlist', `loaded local playlist (${items.length} compositions)`);
}

// ─────────────────────────────────────────────────────────────────────
// Palette library — save / list / export / import. Mirrors the playlist
// bundle helpers: server-backed when /api/palettes is reachable, else a
// localStorage fallback for the static GitHub Pages build. Palettes are
// small structured data, so the library is JSON (not the PSC binary wire).
// ─────────────────────────────────────────────────────────────────────

// Bundle wire format — mirrored by web/local_server.py (PALETTE_BUNDLE_VERSION).
const PALETTE_BUNDLE_VERSION = 1;

function cloneCoeffs(coeffs) {
    return {
        a: coeffs.a.slice(),
        b: coeffs.b.slice(),
        c: coeffs.c.slice(),
        d: coeffs.d.slice(),
    };
}

// Client-side validator, mirroring _validate_palette_data in local_server.py.
function validatePaletteData(data) {
    if (!data || typeof data !== 'object') throw new Error('palette must be an object');
    if (data.type !== 'cosine' && data.type !== 'raw') {
        throw new Error('palette type must be "cosine" or "raw"');
    }
    if (typeof data.name !== 'string' || !data.name.trim()) {
        throw new Error('palette name is required');
    }
    const entries = data.entries;
    if (!Array.isArray(entries) || entries.length !== PALETTE_ENTRY_COUNT) {
        throw new Error(`palette entries must be ${PALETTE_ENTRY_COUNT} [r,g,b] triples`);
    }
    for (const entry of entries) {
        if (!Array.isArray(entry) || entry.length !== 3) {
            throw new Error('palette entry must be an [r,g,b] triple');
        }
        for (const ch of entry) {
            if (!Number.isInteger(ch) || ch < 0 || ch > 255) {
                throw new Error('palette channel must be an integer 0..255');
            }
        }
    }
    if (data.type === 'cosine') {
        const coeffs = data.coeffs;
        if (!coeffs || typeof coeffs !== 'object') {
            throw new Error('cosine palette requires coeffs');
        }
        for (const key of ['a', 'b', 'c', 'd']) {
            const arr = coeffs[key];
            if (!Array.isArray(arr) || arr.length !== 3 || !arr.every((n) => Number.isFinite(n))) {
                throw new Error(`cosine coeffs.${key} must be 3 finite numbers`);
            }
        }
    }
    return data;
}

// Append -1, -2, … until the slug is unique within `existingNames` (a Set).
// Mirrors the server's _deduplicated_palette_path scheme.
function dedupPaletteSlug(slug, existingNames) {
    if (!existingNames.has(slug)) return slug;
    const dot = slug.lastIndexOf('.');
    const base = dot >= 0 ? slug.slice(0, dot) : slug;
    const ext = dot >= 0 ? slug.slice(dot) : '';
    for (let i = 1; ; i += 1) {
        const candidate = `${base}-${i}${ext}`;
        if (!existingNames.has(candidate)) return candidate;
    }
}

function loadLocalPaletteLibrary() {
    try {
        const raw = localStorage.getItem(PALETTE_LIBRARY_STORAGE_KEY);
        const parsed = raw ? JSON.parse(raw) : [];
        return Array.isArray(parsed) ? parsed : [];
    } catch {
        return [];
    }
}

function saveLocalPaletteLibrary(library) {
    try {
        localStorage.setItem(PALETTE_LIBRARY_STORAGE_KEY, JSON.stringify(library));
    } catch (e) {
        logDebug('error', `palette library save failed: ${e.message}`);
    }
}

async function refreshPaletteLibrary() {
    if (state.paletteApiAvailable) {
        try {
            const body = await fetchPlaylistJson('/api/palettes');
            state.paletteLibrary = Array.isArray(body.palettes)
                ? body.palettes.map((p) => ({ name: p.name, data: p.data }))
                : [];
        } catch (e) {
            logDebug('error', `palette list failed: ${e.message}`);
            state.paletteLibrary = [];
        }
    } else {
        state.paletteLibrary = loadLocalPaletteLibrary();
    }
}

// Persist one palette. Server dedups on collision (never overwrites); the
// localStorage fallback mirrors that. Returns the stored slug.
async function savePaletteData(data) {
    validatePaletteData(data);
    if (state.paletteApiAvailable) {
        const body = await fetchPlaylistJson('/api/palettes/save', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data),
        });
        await refreshPaletteLibrary();
        return body.palette?.name ?? paletteSlug(data.name);
    }
    const library = loadLocalPaletteLibrary();
    const names = new Set(library.map((p) => p.name));
    const slug = dedupPaletteSlug(paletteSlug(data.name), names);
    library.push({ name: slug, data });
    saveLocalPaletteLibrary(library);
    state.paletteLibrary = library;
    return slug;
}

function buildPaletteBundle(library) {
    return {
        polarshaderPalettes: PALETTE_BUNDLE_VERSION,
        palettes: library.map((p) => ({ name: p.name, data: p.data })),
    };
}

function parsePaletteBundle(text) {
    let bundle;
    try {
        bundle = JSON.parse(text);
    } catch {
        throw new Error('not a valid palette file');
    }
    if (!bundle
        || bundle.polarshaderPalettes !== PALETTE_BUNDLE_VERSION
        || !Array.isArray(bundle.palettes)
        || bundle.palettes.length === 0) {
        throw new Error('unrecognised or empty palette file');
    }
    return bundle;
}

function paletteBundleFilename() {
    const now = new Date();
    const pad = (n) => String(n).padStart(2, '0');
    const stamp = `${now.getFullYear()}${pad(now.getMonth() + 1)}${pad(now.getDate())}`
        + `-${pad(now.getHours())}${pad(now.getMinutes())}${pad(now.getSeconds())}`;
    return `polarshader-palettes-${stamp}.json`;
}

function downloadPaletteBundle() {
    if (state.paletteLibrary.length === 0) {
        showStatus('palette library is empty — nothing to export', true);
        return;
    }
    const bundle = buildPaletteBundle(state.paletteLibrary);
    const blob = new Blob([JSON.stringify(bundle)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = paletteBundleFilename();
    document.body.appendChild(a); a.click(); document.body.removeChild(a);
    URL.revokeObjectURL(url);
    const n = state.paletteLibrary.length;
    showStatus(`exported palette library (${n} palette${n === 1 ? '' : 's'})`, false);
    logDebug('palette', `exported ${n} palettes`);
}

// Import replaces the whole library (matching the server's /replace + the
// playlist import flow). The slug is re-derived from data.name — the bundle
// `name` is advisory — then deduped, so import never trusts a bundle filename.
async function importPaletteBundleFile(file) {
    if (!file) return;
    let bundle;
    try {
        bundle = parsePaletteBundle(await file.text());
    } catch (e) {
        showStatus(`palette import error: ${e.message}`, true);
        return;
    }
    const existing = state.paletteLibrary.length;
    const message = existing > 0
        ? `Replace all ${existing} saved palette${existing === 1 ? '' : 's'} with `
            + `${bundle.palettes.length} from this file?\n\nThis cannot be undone.`
        : `Load ${bundle.palettes.length} palette${bundle.palettes.length === 1 ? '' : 's'}?`;
    if (!window.confirm(message)) return;

    if (state.paletteApiAvailable) {
        try {
            const body = await fetchPlaylistJson('/api/palettes/replace', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(bundle),
            });
            await refreshPaletteLibrary();
            const n = body.replaced ?? bundle.palettes.length;
            showStatus(`palette library replaced (${n} palette${n === 1 ? '' : 's'})`, false);
            logDebug('palette', `imported ${n} palettes`);
        } catch (e) {
            showStatus(`palette import error: ${e.message}`, true);
        }
        rebuildPanel();
        return;
    }

    // Static fallback: validate + re-derive slug from data.name + dedup.
    const library = [];
    const names = new Set();
    for (const entry of bundle.palettes) {
        let data;
        try {
            data = validatePaletteData(entry?.data);
        } catch (e) {
            showStatus(`palette import error: ${e.message}`, true);
            return;
        }
        const slug = dedupPaletteSlug(paletteSlug(data.name), names);
        names.add(slug);
        library.push({ name: slug, data });
    }
    saveLocalPaletteLibrary(library);
    state.paletteLibrary = library;
    rebuildPanel();
    showStatus(`palette library loaded (${library.length} palette${library.length === 1 ? '' : 's'})`, false);
    logDebug('palette', `loaded ${library.length} palettes locally`);
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
        setPlaylistBaseline(savedName, bytes);
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

async function updateSelectedPlaylistScene() {
    const name = state.playlistBaselineName || state.playlistSelected;
    if (!name || !state.playlistApiAvailable || state.playlistBusy || state.deployBusy) return;

    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const bytes = encodeScene(sceneForRender(sceneStore.scene));
        const body = await fetchPlaylistJson('/api/playlist/save', {
            method: 'POST',
            params: { name, overwrite: '1' },
            headers: { 'Content-Type': 'application/octet-stream' },
            body: bytes,
        });
        const savedName = body.file?.name ?? name;
        setPlaylistBaseline(savedName, bytes);
        await refreshPlaylistItems();
        statusText = `updated ${savedName}`;
        logDebug('playlist', `${statusText} (${bytes.length} bytes)`);
    } catch (e) {
        statusText = `playlist update error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist update failed for ${name}: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

async function saveSelectedPlaylistSceneAsNew() {
    const sourceName = state.playlistBaselineName || state.playlistSelected || preferredPlaylistFilename();
    if (!sourceName || !state.playlistApiAvailable || state.playlistBusy || state.deployBusy) return;

    const keepRowActive = Boolean(state.playlistActiveName);
    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const bytes = encodeScene(sceneForRender(sceneStore.scene));
        const body = await fetchPlaylistJson('/api/playlist/save', {
            method: 'POST',
            params: { name: sourceName },
            headers: { 'Content-Type': 'application/octet-stream' },
            body: bytes,
        });
        const savedName = body.file?.name ?? sourceName;
        setPlaylistBaseline(savedName, bytes);
        state.playlistActiveName = keepRowActive ? savedName : '';
        await refreshPlaylistItems();
        statusText = `saved ${savedName}`;
        logDebug('playlist', `${statusText} (${bytes.length} bytes)`);
    } catch (e) {
        statusText = `playlist save-as-new error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist save-as-new failed for ${sourceName}: ${e.message}`);
    } finally {
        state.playlistBusy = false;
        rebuildPanel();
        if (statusText) showStatus(statusText, statusError);
    }
}

function discardSelectedPlaylistChanges() {
    const name = state.playlistBaselineName || state.playlistSelected;
    if (!name || state.playlistBusy || state.deployBusy) return;
    void loadPlaylistScene(name);
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
            clearPlaylistBaseline({ clearSelection: true });
        } else if (state.playlistActiveName === deletedName) {
            state.playlistActiveName = '';
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

async function clearPlaylist() {
    if (!playlistBrowsingAvailable() || state.playlistBusy || state.deployBusy) return;
    const count = state.playlistItems.length;
    if (count === 0) return;
    const message = state.playlistLocalMode
        ? `Clear the loaded playlist (${count} composition${count === 1 ? '' : 's'})?`
        : `Delete all ${count} composition${count === 1 ? '' : 's'} from build/psc?\n\nThis cannot be undone.`;
    if (!window.confirm(message)) return;

    if (state.playlistLocalMode) {
        state.localPlaylistBytes = null;
        state.playlistLocalMode = false;
        state.playlistItems = [];
        clearPlaylistBaseline({ clearSelection: true });
        rebuildPanel();
        showStatus(`playlist cleared (${count} composition${count === 1 ? '' : 's'})`, false);
        logDebug('playlist', `cleared local playlist (${count} compositions)`);
        return;
    }

    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const body = await fetchPlaylistJson('/api/playlist/clear', { method: 'POST' });
        clearPlaylistBaseline({ clearSelection: true });
        await refreshPlaylistItems();
        const removed = body.removed ?? count;
        statusText = `playlist cleared (${removed} composition${removed === 1 ? '' : 's'})`;
        logDebug('playlist', statusText);
    } catch (e) {
        statusText = `playlist clear error: ${e.message}`;
        statusError = true;
        logDebug('error', `playlist clear failed: ${e.message}`);
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
    const deployableCount = supportedPlaylistItems().length;
    if (deployableCount === 0) {
        showStatus('save at least one compatible composition to the playlist first', true);
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

// Persist the active custom .PDS so a reload restores it (the built-in display
// selector uses a full page reload + ?display=, which can't carry a custom id).
function persistLoadedDisplay(loaded) {
    try {
        sessionStorage.setItem(LOADED_DISPLAY_STORAGE_KEY, JSON.stringify({
            name: loaded.name,
            bytes: Array.from(loaded.bytes),
        }));
    } catch (e) {
        logDebug('error', `failed to save loaded display: ${e?.message ?? e}`);
    }
}

function clearLoadedDisplay() {
    state.loadedDisplay = null;
    try {
        sessionStorage.removeItem(LOADED_DISPLAY_STORAGE_KEY);
    } catch { /* ignore */ }
}

// Push a runtime .PDS display to the active renderer (worker-first, main-module
// fallback) — mirrors callApplyScene's routing. Returns { status, target, detail }.
async function callLoadDisplay(bytes, seq) {
    if (state.workerRendererFatal) {
        return { status: -1, target: 'worker', detail: 'worker renderer is recovering from a prior trap', seq };
    }
    const workerManager = getWorkerManager() ?? await waitForWorkerManager(2000);
    if (!isCurrentRenderSeq(seq)) return { status: -1, target: 'none', detail: 'stale load skipped', seq };
    if (workerManager) {
        state.workerRendererSeen = true;
        installWorkerPhaseTap();
        const buffer = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
        let response;
        try {
            response = await workerManager.sendMessageWithResponse(
                { type: 'composer_load_display', payload: { bytes: buffer, seq } },
                [buffer]
            );
        } catch (e) {
            return { status: -1, target: 'worker', detail: `worker load rejected: ${e?.message ?? e}`, seq };
        }
        if (response == null) return { status: -1, target: 'worker', detail: 'worker returned no response', seq };
        if (response.stale) return { status: -1, target: 'worker', detail: response.reason ?? 'stale load', seq: response.seq ?? seq };
        const status = typeof response.status === 'number' ? response.status : (response.ok ? 0 : -1);
        return { status, target: 'worker', detail: response.reason ?? null, seq: response.seq ?? seq };
    }

    if (state.workerRendererSeen) {
        return { status: -1, target: 'worker', detail: 'worker renderer disappeared after owning the visible canvas', seq };
    }

    const M = getWasmModule();
    if (M && M._composer_load_display_pds && M._malloc) {
        let ptr = 0;
        try {
            if (!isCurrentRenderSeq(seq)) return { status: -1, target: 'main', detail: 'stale load skipped', seq };
            ptr = M._malloc(bytes.length);
            if (bytes.length > 0 && !ptr) return { status: -1, target: 'main', detail: 'malloc returned null', seq };
            M.HEAPU8.set(bytes, ptr);
            const status = M._composer_load_display_pds(ptr, bytes.length, seq);
            return { status, target: 'main', seq };
        } catch (e) {
            return { status: -1, target: 'main', detail: `load threw: ${e?.message ?? e}`, seq };
        } finally {
            if (ptr) safelyFreeWasm(M, ptr, 'free after main load failed');
        }
    }

    return { status: -1, target: 'none', detail: 'no worker and no main-thread module', seq };
}

// Decode + validate a .PDS blob, then push it as the active display. On success:
// switch state to the custom display, persist it, re-gate raster patterns, and
// replay the current scene onto the new geometry. On failure the current display
// is left untouched.
async function loadDisplayFromBytes(bytes, sourceName) {
    let spec;
    try {
        spec = decodePds(bytes);
    } catch (e) {
        showStatus(`display load error: ${e.message}`, true);
        logDebug('error', `failed to decode .pds "${sourceName}": ${e.message}`);
        return;
    }
    const seq = nextRenderSeq();
    const result = await callLoadDisplay(bytes, seq);
    if (result.status !== 0) {
        const detail = result.detail ? ` — ${result.detail}` : '';
        showStatus(`display load failed: code ${result.status}`, true);
        logDebug('error', `.pds load rejected via ${result.target}: code ${result.status}${detail}`);
        return;
    }
    state.loadedDisplay = {
        bytes: new Uint8Array(bytes),
        name: spec.name || sourceName || 'custom',
        sourceKind: spec.sourceKind ?? 0,
        hasRaster: Boolean(spec.raster),
    };
    state.displayId = DISPLAY_LOADED;
    persistLoadedDisplay(state.loadedDisplay);
    // A raster scene on a non-raster display renders black; swap to a UV-safe
    // default if the loaded display can't show the current pattern's domain.
    ensurePatternTypeAvailableForDisplay();
    const label = `Custom: ${state.loadedDisplay.name}`;
    showStatus(`loaded display ${label} (${state.loadedDisplay.hasRaster ? 'raster' : 'radial'})`, false);
    logDebug('display', `loaded custom display "${state.loadedDisplay.name}" via ${result.target} seq ${seq}`);
    rebuildPanel();
    const sceneSeq = nextRenderSeq();
    state.latestSceneSeq = sceneSeq;
    await pushSceneNow(state.latestEvent, sceneSeq);
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

async function setWorkerDiffusion(percent) {
    const diffusionPercent = clampDiffusionPercent(percent);
    state.diffusionPercent = diffusionPercent;
    const scale = diffusionScaleForPercent(diffusionPercent);
    const workerManager = getWorkerManager() ?? await waitForWorkerManager(500);
    if (!workerManager) return { ok: false, reason: 'worker not active', scale };
    try {
        return await workerManager.sendMessageWithResponse(
            { type: 'composer_set_diffusion', payload: { scale, percent: diffusionPercent } }
        );
    } catch (e) {
        return { ok: false, reason: e && e.message ? e.message : String(e), scale };
    }
}

const STATUS_LABELS = [
    'OK', 'BAD_MAGIC', 'BAD_VERSION', 'TRUNCATED', 'UNKNOWN_TAG', 'BAD_ENUM', 'BAD_VALUE',
];

function showStatus(text, isError) {
    logDebug(isError ? 'error' : 'status', text);
}

// Encode + push on the next animation frame. This keeps drag interactions
// responsive while still coalescing bursts of input events.
async function pushSceneNow(pushEvent, pushSeq) {
    if (!isCurrentRenderSeq(pushSeq)) {
        logDebug('stale', `skipped stale scene rev ${pushEvent?.revision ?? '?'} seq ${pushSeq}`);
        return;
    }
    try {
        const renderScene = sceneForRender(sceneStore.scene);
        const bytes = encodeScene(renderScene);
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
    refreshPlaylistDirtyState({ rebuild: true });
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
    if (c.min != null) input.min = c.min;
    if (c.max != null) input.max = c.max;
    input.value = configObj[c.name] ?? c.default ?? 0;
    input.addEventListener('input', () => {
        let v = parseInt(input.value, 10);
        if (Number.isNaN(v)) return;
        v = Math.min(c.max ?? v, Math.max(c.min ?? v, v));
        setValue(c.name, v);
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

function defaultSignalForSlot(slot) {
    return slot.default ? JSON.parse(JSON.stringify(slot.default)) : DEFAULT_SIGNAL();
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
            signalsObj[s.name] ?? defaultSignalForSlot(s),
            (next) => setSignal(s.name, next)
        );
        row.appendChild(slot);
        form.appendChild(row);
    }
    return form;
}

// ─────────────────────────────────────────────────────────────────────
// Palette generator (cosine palette) UI
// ─────────────────────────────────────────────────────────────────────

// Slider ranges per coefficient. a/b/d are normalized ~0..1; c is a small
// frequency multiplier (colour cycles across the strip), so it gets a wider
// range. These bound the UI only — the cosine math clamps to [0,1] anyway.
const PALETTE_COEFF_RANGES = { a: [0, 1], b: [0, 1], c: [0, 4], d: [0, 1] };
const PALETTE_CHANNEL_LABELS = ['R', 'G', 'B'];

// Human-friendly meaning of each cosine coefficient (Inigo Quilez formula).
const PALETTE_COEFF_META = {
    a: { title: 'Base', desc: 'base brightness / DC offset per channel' },
    b: { title: 'Amplitude', desc: 'contrast — how far each channel swings' },
    c: { title: 'Cycles', desc: 'number of colour cycles across the strip' },
    d: { title: 'Phase', desc: 'hue phase offset per channel' },
};

// Generator state survives panel rebuilds by living on `state`, not the DOM.
function ensurePaletteGenerator() {
    if (!state.paletteGenerator) {
        state.paletteGenerator = {
            coeffs: defaultCosineCoeffs(),
            name: '',
            startFrom: '',
            liveApply: true,
            link: { a: false, b: false, c: false, d: false },
            savedSnapshot: null,
        };
    }
    return state.paletteGenerator;
}

// True when two coefficient sets are identical channel-for-channel.
function coeffsEqual(a, b) {
    if (!a || !b) return false;
    for (const key of ['a', 'b', 'c', 'd']) {
        for (let ch = 0; ch < 3; ch += 1) {
            if (a[key][ch] !== b[key][ch]) return false;
        }
    }
    return true;
}

// The generator has unsaved edits when its coeffs differ from the last save.
function paletteGeneratorDirty(gen) {
    return !gen.savedSnapshot || !coeffsEqual(gen.coeffs, gen.savedSnapshot);
}

// Random coefficients in IQ-friendly ranges (a,b,d ~0..1, c a small frequency).
function randomCosineCoeffs() {
    const rand = (min, max) => min + Math.random() * (max - min);
    const triple = (min, max) => [rand(min, max), rand(min, max), rand(min, max)];
    return {
        a: triple(0.3, 0.7),
        b: triple(0.2, 0.6),
        c: triple(0.5, 2.0),
        d: triple(0.0, 1.0),
    };
}

// Shadertoy-style GLSL palette function for these coeffs (for Copy GLSL).
function coeffsToGlsl(coeffs) {
    const vec = (t) => `vec3(${t.map((n) => n.toFixed(3)).join(', ')})`;
    return 'vec3 palette(float t) {\n'
        + `    vec3 a = ${vec(coeffs.a)};\n`
        + `    vec3 b = ${vec(coeffs.b)};\n`
        + `    vec3 c = ${vec(coeffs.c)};\n`
        + `    vec3 d = ${vec(coeffs.d)};\n`
        + '    return a + b * cos(6.28318 * (c * t + d));\n'
        + '}';
}

async function copyGeneratorGlsl(gen) {
    try {
        await navigator.clipboard.writeText(coeffsToGlsl(gen.coeffs));
        showStatus('copied GLSL palette to clipboard', false);
    } catch (e) {
        showStatus(`clipboard error: ${e.message}`, true);
    }
}

// Apply the generator's current colours to the scene (embeds on encode).
function applyGeneratorToScene(gen) {
    const entries = cosinePaletteEntries(gen.coeffs);
    const match = matchingLibraryPalette(entries);
    gen.startFrom = match ? `lib:${match.name}` : 'embedded';
    mutateScene('paletteEntries', 'cosine', (scene) => {
        scene.paletteId = null;
        scene.paletteEntries = entries.map((e) => e.slice());
        scene.paletteCoeffs = cloneCoeffs(gen.coeffs);
    });
}

// Delete a saved palette (server or localStorage), then rebuild the panel.
async function deletePaletteBySlug(slug) {
    const entry = state.paletteLibrary.find((p) => p.name === slug);
    const label = entry?.data?.name ?? slug;
    if (!window.confirm(`Delete palette "${label}"? This cannot be undone.`)) return;
    try {
        if (state.paletteApiAvailable) {
            await fetchPlaylistJson(`/api/palettes/delete?name=${encodeURIComponent(slug)}`, { method: 'POST' });
            await refreshPaletteLibrary();
        } else {
            const library = loadLocalPaletteLibrary().filter((p) => p.name !== slug);
            saveLocalPaletteLibrary(library);
            state.paletteLibrary = library;
        }
        showStatus(`deleted palette ${label}`, false);
    } catch (e) {
        showStatus(`palette delete error: ${e.message}`, true);
    }
    rebuildPanel();
}

// Suggest "Cosine N" where N is one past the highest existing "Cosine N".
function defaultPaletteName() {
    let max = 0;
    for (const entry of state.paletteLibrary) {
        const m = /^Cosine (\d+)$/.exec(entry.data?.name ?? '');
        if (m) max = Math.max(max, parseInt(m[1], 10));
    }
    return `Cosine ${max + 1}`;
}

async function savePaletteFromGenerator() {
    const gen = ensurePaletteGenerator();
    const name = (gen.name || '').trim() || defaultPaletteName();
    const data = {
        type: 'cosine',
        name,
        coeffs: cloneCoeffs(gen.coeffs),
        entries: cosinePaletteEntries(gen.coeffs),
    };
    try {
        const slug = await savePaletteData(data);
        gen.savedSnapshot = cloneCoeffs(gen.coeffs);
        gen.startFrom = `lib:${slug}`;
        gen.name = '';
        rebuildPanel();
        showStatus(`saved palette ${name}`, false);
        logDebug('palette', `saved ${slug}`);
    } catch (e) {
        showStatus(`palette save error: ${e.message}`, true);
        logDebug('error', `palette save failed: ${e.message}`);
    }
}

function renderPaletteGeneratorSection() {
    const gen = ensurePaletteGenerator();
    const section = document.createElement('section');
    section.className = 'panel-section palette-generator';

    section.appendChild(Object.assign(document.createElement('h2'), { textContent: 'Palette Generator' }));
    section.appendChild(Object.assign(document.createElement('p'), {
        className: 'muted-note',
        textContent: 'Cosine palette: channel(t) = a + b·cos(2π·(c·t + d)).',
    }));

    // ── Live preview: gradient strip + the 16 discrete entries the engine renders.
    const preview = document.createElement('div');
    preview.className = 'palette-preview';
    section.appendChild(preview);

    const swatches = document.createElement('div');
    swatches.className = 'palette-swatches';
    const swatchCells = [];
    for (let i = 0; i < PALETTE_ENTRY_COUNT; i += 1) {
        const cell = document.createElement('div');
        cell.className = 'palette-swatch';
        swatches.appendChild(cell);
        swatchCells.push(cell);
    }
    section.appendChild(swatches);

    // Outputs updated in place (no panel rebuild) so sliders survive dragging.
    let saveBtn = null;
    let exportBtn = null;
    let dirtyNote = null;
    const updateOutputs = ({ applyLive = true } = {}) => {
        const entries = cosinePaletteEntries(gen.coeffs);
        preview.style.background = paletteGradientCss(entries);
        for (let i = 0; i < swatchCells.length; i += 1) {
            const [r, g, b] = entries[i];
            swatchCells[i].style.background = `rgb(${r},${g},${b})`;
        }
        const dirty = paletteGeneratorDirty(gen);
        if (saveBtn) saveBtn.disabled = !dirty;
        if (dirtyNote) dirtyNote.textContent = gen.savedSnapshot && !dirty ? 'Saved' : 'Unsaved changes';
        if (exportBtn) exportBtn.disabled = state.paletteLibrary.length === 0;
        if (applyLive && gen.liveApply) applyGeneratorToScene(gen);
    };

    // ── Palette: same choices as the Palette transform, plus Randomize / Reset.
    const startRow = document.createElement('div');
    startRow.className = 'top-row palette-start-row';
    const startSel = document.createElement('select');
    appendPaletteOptions(startSel, sceneStore.scene);
    const currentChoice = currentPaletteChoice(sceneStore.scene);
    const startChoice = gen.startFrom || currentChoice;
    startSel.value = startChoice;
    if (startSel.value !== startChoice) {
        gen.startFrom = currentChoice;
        startSel.value = currentChoice;
    }
    startSel.addEventListener('change', () => {
        syncPaletteGeneratorFromChoice(startSel.value);
        applyPaletteChoiceToScene(startSel.value, { syncGenerator: false });
        rebuildPanel();
    });
    startRow.appendChild(startSel);

    const randomBtn = document.createElement('button');
    randomBtn.type = 'button';
    randomBtn.textContent = 'Randomize';
    randomBtn.title = 'Roll random coefficients';
    randomBtn.addEventListener('click', () => {
        gen.coeffs = randomCosineCoeffs();
        gen.startFrom = '';
        if (gen.liveApply) applyGeneratorToScene(gen);
        rebuildPanel();
    });
    startRow.appendChild(randomBtn);

    const resetBtn = document.createElement('button');
    resetBtn.type = 'button';
    resetBtn.textContent = 'Reset';
    resetBtn.title = 'Reset to the default rainbow coefficients';
    resetBtn.addEventListener('click', () => {
        gen.coeffs = defaultCosineCoeffs();
        gen.startFrom = '';
        if (gen.liveApply) applyGeneratorToScene(gen);
        rebuildPanel();
    });
    startRow.appendChild(resetBtn);
    section.appendChild(startRow);

    // ── Coefficient groups: a/b/c/d, each with R/G/B slider + numeric input,
    //    a tooltip explaining its role, and an optional "link RGB" toggle.
    for (const key of ['a', 'b', 'c', 'd']) {
        section.appendChild(buildPaletteCoeffGroup(gen, key, updateOutputs));
    }

    // ── Name (auto-advances after a save) + saved/dirty indicator.
    const nameRow = document.createElement('label');
    nameRow.className = 'param';
    nameRow.appendChild(Object.assign(document.createElement('span'), { className: 'param-label', textContent: 'Name' }));
    const nameIn = document.createElement('input');
    nameIn.type = 'text';
    nameIn.placeholder = defaultPaletteName();
    nameIn.value = gen.name;
    nameIn.addEventListener('input', () => { gen.name = nameIn.value; });
    nameRow.appendChild(nameIn);
    section.appendChild(nameRow);

    dirtyNote = document.createElement('p');
    dirtyNote.className = 'muted-note palette-dirty-note';
    section.appendChild(dirtyNote);

    // ── Actions.
    const actions = document.createElement('div');
    actions.className = 'top-row palette-actions';

    const liveLabel = document.createElement('label');
    liveLabel.className = 'palette-live-toggle';
    liveLabel.title = 'Update the current scene as you edit';
    const liveBox = document.createElement('input');
    liveBox.type = 'checkbox';
    liveBox.checked = gen.liveApply;
    liveBox.addEventListener('change', () => {
        gen.liveApply = liveBox.checked;
        if (gen.liveApply) applyGeneratorToScene(gen);
    });
    liveLabel.appendChild(liveBox);
    liveLabel.appendChild(Object.assign(document.createElement('span'), { textContent: 'Live apply' }));
    actions.appendChild(liveLabel);

    const applyBtn = document.createElement('button');
    applyBtn.type = 'button';
    applyBtn.textContent = 'Apply to scene';
    applyBtn.title = 'Use this palette in the current composition';
    applyBtn.addEventListener('click', () => {
        applyGeneratorToScene(gen);
        showStatus('applied cosine palette to scene', false);
    });
    actions.appendChild(applyBtn);

    saveBtn = document.createElement('button');
    saveBtn.type = 'button';
    saveBtn.textContent = 'Save';
    saveBtn.title = 'Save this palette to your library';
    saveBtn.addEventListener('click', () => { void savePaletteFromGenerator(); });
    actions.appendChild(saveBtn);

    const glslBtn = document.createElement('button');
    glslBtn.type = 'button';
    glslBtn.textContent = 'Copy GLSL';
    glslBtn.title = 'Copy the Shadertoy-style palette() function';
    glslBtn.addEventListener('click', () => { void copyGeneratorGlsl(gen); });
    actions.appendChild(glslBtn);

    exportBtn = document.createElement('button');
    exportBtn.type = 'button';
    exportBtn.textContent = 'Export';
    exportBtn.title = 'Download the whole palette library as a file';
    exportBtn.addEventListener('click', downloadPaletteBundle);
    actions.appendChild(exportBtn);

    const importBtn = document.createElement('button');
    importBtn.type = 'button';
    importBtn.textContent = 'Import';
    importBtn.title = 'Replace the palette library from a file';
    const importIn = document.createElement('input');
    importIn.type = 'file';
    importIn.accept = '.json,application/json';
    importIn.style.display = 'none';
    importBtn.addEventListener('click', () => importIn.click());
    importIn.addEventListener('change', () => {
        const f = importIn.files?.[0];
        importIn.value = '';
        if (f) void importPaletteBundleFile(f);
    });
    actions.appendChild(importBtn);
    actions.appendChild(importIn);

    section.appendChild(actions);

    // ── Saved-palette library: load a cosine one back into the sliders, or delete.
    if (state.paletteLibrary.length > 0) {
        section.appendChild(renderPaletteLibraryList(gen));
    }

    updateOutputs({ applyLive: false });
    return section;
}

// One coefficient block (a/b/c/d): a labelled header with a "link RGB" toggle
// and three R/G/B rows, each a slider + two-way-bound numeric input.
function buildPaletteCoeffGroup(gen, key, updateOutputs) {
    const [min, max] = PALETTE_COEFF_RANGES[key];
    const meta = PALETTE_COEFF_META[key];
    const group = document.createElement('div');
    group.className = 'palette-coeff-group';

    const head = document.createElement('div');
    head.className = 'palette-coeff-head';
    const title = Object.assign(document.createElement('span'), {
        className: 'param-label',
        textContent: `${key} · ${meta.title}`,
    });
    title.title = meta.desc;
    head.appendChild(title);

    const linkLabel = document.createElement('label');
    linkLabel.className = 'palette-link-toggle';
    linkLabel.title = 'Move all three channels together';
    const linkBox = document.createElement('input');
    linkBox.type = 'checkbox';
    linkBox.checked = gen.link[key];
    linkLabel.appendChild(linkBox);
    linkLabel.appendChild(Object.assign(document.createElement('span'), { textContent: 'link RGB' }));
    head.appendChild(linkLabel);
    group.appendChild(head);

    const rows = [];
    // Set one channel (clamped) and sync its slider; when linked, mirror to the
    // other channels. `skipNumber` leaves the actively-edited number field alone
    // so typing (e.g. "0.5") isn't clobbered by a mid-entry reformat.
    const setChannel = (ch, value, { fromLink = false, skipNumber = false } = {}) => {
        const clamped = Math.min(max, Math.max(min, value));
        gen.coeffs[key][ch] = clamped;
        rows[ch].slider.value = String(clamped);
        if (!skipNumber) rows[ch].number.value = clamped.toFixed(2);
        if (!fromLink && gen.link[key]) {
            for (let o = 0; o < 3; o += 1) if (o !== ch) setChannel(o, clamped, { fromLink: true });
        }
    };

    for (let ch = 0; ch < 3; ch += 1) {
        const row = document.createElement('div');
        row.className = 'palette-slider-row';
        row.appendChild(Object.assign(document.createElement('span'), {
            className: 'palette-slider-label',
            textContent: PALETTE_CHANNEL_LABELS[ch],
        }));
        const slider = document.createElement('input');
        slider.type = 'range';
        slider.min = String(min);
        slider.max = String(max);
        slider.step = '0.01';
        slider.value = String(gen.coeffs[key][ch]);
        const number = document.createElement('input');
        number.type = 'number';
        number.className = 'palette-slider-value';
        number.min = String(min);
        number.max = String(max);
        number.step = '0.01';
        number.value = gen.coeffs[key][ch].toFixed(2);

        slider.addEventListener('input', () => {
            setChannel(ch, parseFloat(slider.value), {});
            updateOutputs();
        });
        number.addEventListener('input', () => {
            const v = parseFloat(number.value);
            if (Number.isFinite(v)) {
                setChannel(ch, v, { skipNumber: true });
                updateOutputs();
            }
        });
        // Normalise (clamp + reformat) the field once editing commits.
        number.addEventListener('change', () => {
            number.value = gen.coeffs[key][ch].toFixed(2);
        });

        row.appendChild(slider);
        row.appendChild(number);
        rows.push({ slider, number });
        group.appendChild(row);
    }

    linkBox.addEventListener('change', () => {
        gen.link[key] = linkBox.checked;
        if (linkBox.checked) {
            setChannel(0, gen.coeffs[key][0], {});
            updateOutputs();
        }
    });

    return group;
}

// The saved-palette library list with per-palette Load (cosine) / Delete.
function renderPaletteLibraryList(gen) {
    const wrap = document.createElement('div');
    wrap.className = 'palette-library';
    wrap.appendChild(Object.assign(document.createElement('h3'), { textContent: 'Saved palettes' }));
    for (const entry of state.paletteLibrary) {
        const row = document.createElement('div');
        row.className = 'palette-library-row';

        const swatch = document.createElement('div');
        swatch.className = 'palette-lib-swatch';
        if (Array.isArray(entry.data?.entries)) {
            swatch.style.background = paletteGradientCss(entry.data.entries);
        }
        row.appendChild(swatch);

        row.appendChild(Object.assign(document.createElement('span'), {
            className: 'palette-lib-name',
            textContent: entry.data?.name ?? entry.name,
        }));

        if (entry.data?.type === 'cosine' && entry.data.coeffs) {
            const loadBtn = document.createElement('button');
            loadBtn.type = 'button';
            loadBtn.textContent = 'Load';
            loadBtn.title = 'Load these coefficients into the sliders';
            loadBtn.addEventListener('click', () => {
                syncPaletteGeneratorFromChoice(`lib:${entry.name}`);
                if (gen.liveApply) applyPaletteChoiceToScene(`lib:${entry.name}`, { syncGenerator: false });
                rebuildPanel();
            });
            row.appendChild(loadBtn);
        }

        const delBtn = document.createElement('button');
        delBtn.type = 'button';
        delBtn.textContent = 'Delete';
        delBtn.title = 'Delete this palette from the library';
        delBtn.addEventListener('click', () => { void deletePaletteBySlug(entry.name); });
        row.appendChild(delBtn);

        wrap.appendChild(row);
    }
    return wrap;
}

// True when the two 16-entry palettes have identical RGB values.
function paletteEntriesMatch(a, b) {
    if (!Array.isArray(a) || !Array.isArray(b) || a.length !== b.length) return false;
    for (let i = 0; i < a.length; i += 1) {
        for (let ch = 0; ch < 3; ch += 1) {
            if (a[i]?.[ch] !== b[i]?.[ch]) return false;
        }
    }
    return true;
}

function matchingLibraryPalette(entries) {
    if (!Array.isArray(entries)) return null;
    return state.paletteLibrary.find((p) => paletteEntriesMatch(p.data?.entries, entries)) ?? null;
}

function currentPaletteChoice(scene) {
    if (Array.isArray(scene.paletteEntries)) {
        const libMatch = matchingLibraryPalette(scene.paletteEntries);
        return libMatch ? `lib:${libMatch.name}` : 'embedded';
    }
    return `builtin:${scene.paletteId ?? PALETTES[0]?.id ?? 0}`;
}

function appendPaletteOptions(select, scene) {
    for (const palette of PALETTES) {
        const opt = document.createElement('option');
        opt.value = `builtin:${palette.id}`;
        opt.textContent = palette.name;
        select.appendChild(opt);
    }
    for (const entry of state.paletteLibrary) {
        const opt = document.createElement('option');
        opt.value = `lib:${entry.name}`;
        opt.textContent = entry.data?.name ?? entry.name;
        select.appendChild(opt);
    }
    if (Array.isArray(scene.paletteEntries) && !matchingLibraryPalette(scene.paletteEntries)) {
        const opt = document.createElement('option');
        opt.value = 'embedded';
        opt.textContent = 'Embedded custom';
        select.appendChild(opt);
    }
}

function paletteLibraryEntryFromChoice(choice) {
    if (!choice.startsWith('lib:')) return null;
    return state.paletteLibrary.find((p) => p.name === choice.slice('lib:'.length)) ?? null;
}

function syncPaletteGeneratorFromChoice(choice) {
    const gen = ensurePaletteGenerator();
    gen.startFrom = choice;
    const entry = paletteLibraryEntryFromChoice(choice);
    if (entry?.data?.type === 'cosine' && entry.data.coeffs) {
        gen.coeffs = cloneCoeffs(entry.data.coeffs);
        gen.savedSnapshot = cloneCoeffs(entry.data.coeffs);
    }
}

function syncPaletteGeneratorFromScene() {
    const scene = sceneStore.scene;
    const choice = currentPaletteChoice(scene);
    const entry = paletteLibraryEntryFromChoice(choice);
    if (entry?.data?.type === 'cosine' && entry.data.coeffs) {
        syncPaletteGeneratorFromChoice(choice);
        return;
    }
    if (scene.paletteCoeffs) {
        const gen = ensurePaletteGenerator();
        gen.startFrom = choice;
        gen.coeffs = cloneCoeffs(scene.paletteCoeffs);
        gen.savedSnapshot = null;
        return;
    }
    syncPaletteGeneratorFromChoice(choice);
}

function applyPaletteChoiceToScene(choice, { syncGenerator = true } = {}) {
    if (syncGenerator) syncPaletteGeneratorFromChoice(choice);
    if (choice.startsWith('builtin:')) {
        const paletteId = parseInt(choice.slice('builtin:'.length), 10);
        mutateScene('paletteId', paletteId, (scene) => {
            scene.paletteId = paletteId;
            scene.paletteEntries = null;
            scene.paletteCoeffs = null;
        });
        return true;
    }
    if (choice.startsWith('lib:')) {
        const entry = paletteLibraryEntryFromChoice(choice);
        if (!entry?.data?.entries) return false;
        mutateScene('paletteEntries', entry.name, (scene) => {
            scene.paletteId = null;
            scene.paletteEntries = entry.data.entries.map((e) => e.slice());
            scene.paletteCoeffs = entry.data.type === 'cosine' && entry.data.coeffs
                ? cloneCoeffs(entry.data.coeffs)
                : null;
        });
        return true;
    }
    return choice === 'embedded';
}

// Save an unmatched embedded palette. PSC v3 palettes keep their cosine
// coefficients; v2 entries-only palettes are saved as raw.
async function saveEmbeddedPaletteToLibrary(scene) {
    const name = window.prompt('Palette name', 'Imported palette');
    if (name === null) return;
    const trimmed = name.trim();
    if (!trimmed) { showStatus('palette name is required', true); return; }
    const entries = scene.paletteEntries;
    if (!Array.isArray(entries)) return;
    const data = scene.paletteCoeffs
        ? {
            type: 'cosine',
            name: trimmed,
            coeffs: cloneCoeffs(scene.paletteCoeffs),
            entries: entries.map((e) => e.slice()),
        }
        : { type: 'raw', name: trimmed, entries: entries.map((e) => e.slice()) };
    try {
        await savePaletteData(data);
        rebuildPanel();
        showStatus(`saved palette ${trimmed}`, false);
    } catch (e) {
        showStatus(`palette save error: ${e.message}`, true);
    }
}

function renderPaletteSelector() {
    const wrap = document.createElement('label');
    wrap.className = 'param';
    const labelText = document.createElement('span');
    labelText.className = 'param-label';
    labelText.textContent = 'Palette';
    wrap.appendChild(labelText);

    const scene = sceneStore.scene;
    const sel = document.createElement('select');
    appendPaletteOptions(sel, scene);
    const hasEmbedded = Array.isArray(scene.paletteEntries);
    const libMatch = hasEmbedded ? matchingLibraryPalette(scene.paletteEntries) : null;
    sel.value = currentPaletteChoice(scene);

    sel.addEventListener('change', () => {
        applyPaletteChoiceToScene(sel.value);
        rebuildPanel();
    });
    wrap.appendChild(sel);

    if (hasEmbedded && !libMatch) {
        const saveBtn = document.createElement('button');
        saveBtn.type = 'button';
        saveBtn.textContent = 'Save to library…';
        saveBtn.title = 'Save this embedded palette to your library';
        // The button lives inside the <label>; preventDefault stops the click
        // from also being forwarded to (and re-focusing) the <select>.
        saveBtn.addEventListener('click', (e) => {
            e.preventDefault();
            void saveEmbeddedPaletteToLibrary(scene);
        });
        wrap.appendChild(saveBtn);
    }
    return wrap;
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
        if (!signalsObj[s.name]) signalsObj[s.name] = defaultSignalForSlot(s);
    }
}

// ─────────────────────────────────────────────────────────────────────
// Pattern picker
// ─────────────────────────────────────────────────────────────────────

function renderPatternSection() {
    const section = document.createElement('section');
    section.className = 'panel-section';

    const availableDomains = availablePatternDomainsForDisplay(state.displayId);
    const selectedDomain = patternDomain(sceneStore.scene.pattern?.id);
    const selectedDomainAvailable = currentPatternDomainIsAvailable();
    const header = document.createElement('div');
    header.className = 'pattern-header';
    const h = document.createElement('h2');
    h.textContent = 'Pattern';
    header.appendChild(h);
    if (availableDomains.length > 1) {
        const typeSel = document.createElement('select');
        typeSel.className = 'pattern-type-select';
        for (const domain of availableDomains) {
            const opt = document.createElement('option');
            opt.value = domain;
            opt.textContent = PATTERN_DOMAIN_LABELS[domain] ?? domain;
            typeSel.appendChild(opt);
        }
        typeSel.value = selectedDomain;
        typeSel.addEventListener('change', () => {
            const nextDomain = typeSel.value;
            const nextPatternId = firstPatternIdForDomain(nextDomain);
            mutateScene('pattern.domain', nextDomain, (scene) => {
                setScenePattern(scene, nextPatternId);
            });
            rebuildPanel();
        });
        header.appendChild(typeSel);
    }
    section.appendChild(header);

    if (!selectedDomainAvailable) {
        const warning = document.createElement('div');
        warning.className = 'inline-warning';
        warning.textContent = `${PATTERN_DOMAIN_LABELS[selectedDomain] ?? selectedDomain} patterns need a matrix display.`;
        section.appendChild(warning);
    }

    const row = document.createElement('div');
    row.className = 'pattern-picker-row';
    const sel = document.createElement('select');
    appendPatternDropdownOptions(sel, selectedDomain);
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
            setScenePattern(scene, sel.value);
        });
        rebuildPanel();
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
    const addBtn = document.createElement('button'); addBtn.textContent = 'Add';
    addRow.appendChild(addSel); addRow.appendChild(addBtn);
    section.appendChild(addRow);

    function rebuildAddOptions() {
        addSel.innerHTML = '';
        for (const [id, def] of Object.entries(TRANSFORMS)) {
            if (def.hidden || !isTransformAddable(sceneStore.scene, id)) continue;
            const opt = document.createElement('option');
            opt.value = id; opt.textContent = def.label;
            addSel.appendChild(opt);
        }
        addBtn.disabled = addSel.options.length === 0;
    }

    function rebuildList() {
        list.innerHTML = '';
        sceneStore.scene.transforms.forEach((t, i) => {
            const def = TRANSFORMS[t.id];
            hydrateConfig(def.config, t.config);
            hydrateSignals(def.signals, t.signals);

            const card = document.createElement('div'); card.className = 'transform-card';
            if (t.enabled === false) card.classList.add('transform-disabled');
            const isPaletteTransform = transformDomain(t.id) === 'palette';
            const isFixedGreyscalePalette = isPaletteTransform && isGreyscalePattern(sceneStore.scene.pattern?.id);
            if (isFixedGreyscalePalette) {
                t.enabled = true;
                card.classList.remove('transform-disabled');
            }
            const header = document.createElement('div'); header.className = 'transform-header';
            const title = document.createElement('span'); title.className = 'transform-title';
            title.textContent = `${i + 1}. ${def.label}`;
            header.appendChild(title);

            // Enabled toggle: unchecking drops this transform from the render
            // push (sceneForRender) without touching its params, so its effect
            // can be compared on/off instantly.
            if (!isFixedGreyscalePalette) {
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
            }

            const btnRm   = document.createElement('button'); btnRm.textContent   = '✕';
            if (!isPaletteTransform) {
                const btnUp   = document.createElement('button'); btnUp.textContent   = '↑';
                const btnDown = document.createElement('button'); btnDown.textContent = '↓';
                btnUp.disabled = (
                    i === 0 ||
                    transformDomain(sceneStore.scene.transforms[i - 1]?.id) === 'palette'
                );
                btnDown.disabled = (i === sceneStore.scene.transforms.length - 1);
                btnUp.addEventListener('click', () => {
                    mutateScene('transforms.reorder', `up:${i}`, (scene) => {
                        if (transformDomain(scene.transforms[i - 1]?.id) === 'palette') return;
                        [scene.transforms[i - 1], scene.transforms[i]] =
                            [scene.transforms[i], scene.transforms[i - 1]];
                    });
                    rebuildList();
                });
                btnDown.addEventListener('click', () => {
                    mutateScene('transforms.reorder', `down:${i}`, (scene) => {
                        if (transformDomain(scene.transforms[i]?.id) === 'palette') return;
                        [scene.transforms[i + 1], scene.transforms[i]] =
                            [scene.transforms[i], scene.transforms[i + 1]];
                    });
                    rebuildList();
                });
                header.appendChild(btnUp);
                header.appendChild(btnDown);
            }
            if (!isFixedGreyscalePalette) {
                btnRm.addEventListener('click', () => {
                    mutateScene('transforms.remove', `${i}:${t.id}`, (scene) => {
                        scene.transforms.splice(i, 1);
                    });
                    rebuildAddOptions();
                    rebuildList();
                });
                header.appendChild(btnRm);
            }
            card.appendChild(header);

            if (isPaletteTransform) {
                const paletteForm = document.createElement('div');
                paletteForm.className = 'config-form';
                paletteForm.appendChild(renderPaletteSelector());
                card.appendChild(paletteForm);
            }

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
            if (t.id === 'vortex') {
                // Web-only quick-set: snap the vortex strength to the golden-angle
                // ("Fibonacci") swirl in either direction. Writes a plain constant
                // signal, so it round-trips through the normal signal editor.
                const fibRow = document.createElement('div');
                fibRow.className = 'config-form vortex-fibonacci';
                fibRow.appendChild(Object.assign(document.createElement('span'),
                    { className: 'param-label', textContent: 'Fibonacci twist' }));
                const setFibonacciStrength = (permille) => {
                    mutateScene(`transforms.${i}.${t.id}.signals.strength`,
                        `constant(${permille})`, () => {
                            t.signals.strength = { id: 'constant', params: { permille } };
                        });
                    rebuildList();
                };
                const cwBtn = document.createElement('button');
                cwBtn.type = 'button';
                cwBtn.textContent = 'φ CW';
                cwBtn.title = 'Golden-angle swirl, clockwise (≈137.5° twist at the rim)';
                cwBtn.addEventListener('click', () => setFibonacciStrength(VORTEX_FIBONACCI_CW_PERMILLE));
                const ccwBtn = document.createElement('button');
                ccwBtn.type = 'button';
                ccwBtn.textContent = 'φ CCW';
                ccwBtn.title = 'Golden-angle swirl, counter-clockwise (≈137.5° twist at the rim)';
                ccwBtn.addEventListener('click', () => setFibonacciStrength(VORTEX_FIBONACCI_CCW_PERMILLE));
                fibRow.appendChild(cwBtn);
                fibRow.appendChild(ccwBtn);
                card.appendChild(fibRow);
            }
            list.appendChild(card);
        });
    }

    addBtn.addEventListener('click', () => {
        if (!addSel.value) return;
        mutateScene('transforms.add', addSel.value, (scene) => {
            if (!isTransformAddable(scene, addSel.value)) return;
            const transform = transformDomain(addSel.value) === 'palette'
                ? DEFAULT_PALETTE_TRANSFORM()
                : { id: addSel.value, config: {}, signals: {} };
            if (transformDomain(transform.id) === 'palette') {
                scene.transforms.unshift(transform);
            } else {
                scene.transforms.push(transform);
            }
        });
        rebuildAddOptions();
        rebuildList();
    });

    rebuildAddOptions();
    rebuildList();
    return section;
}

// ─────────────────────────────────────────────────────────────────────
// Save/load + status
// ─────────────────────────────────────────────────────────────────────

function renderTopSection() {
    const section = document.createElement('section'); section.className = 'panel-section';

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
    // A runtime-loaded .pds display has no built-in id; surface it as a synthetic
    // "Custom: <name>" option so the selector reflects the active geometry.
    if (state.displayId === DISPLAY_LOADED && state.loadedDisplay) {
        const o = document.createElement('option');
        o.value = DISPLAY_LOADED;
        o.textContent = `Custom: ${state.loadedDisplay.name}`;
        dispSel.appendChild(o);
    }
    dispSel.value = state.displayId;
    dispSel.addEventListener('change', () => {
        const which = parseInt(dispSel.value, 10);
        if (which === state.displayId) return;
        // Leaving a custom display for a built-in: drop the persisted .pds so a
        // reload lands on the chosen built-in, not the custom display.
        clearLoadedDisplay();
        state.displayId = which;
        saveLastGoodBootSceneState();
        const seq = nextRenderSeq();
        const label = DISPLAYS.find((d) => d.id === which)?.name ?? which;
        showStatus(`switching display: ${label}`, false);
        logDebug('display', `reloading for ${label} seq ${seq}`);
        reloadForDisplay(which);
    });
    dispRow.appendChild(dispSel);
    const loadDisplayBtn = document.createElement('button');
    loadDisplayBtn.textContent = 'Load display';
    loadDisplayBtn.title = 'Load a custom .pds display (in-place, no reload)';
    const displayIn = document.createElement('input');
    displayIn.type = 'file';
    displayIn.accept = '.pds,application/octet-stream';
    displayIn.style.display = 'none';
    loadDisplayBtn.addEventListener('click', () => displayIn.click());
    displayIn.addEventListener('change', () => {
        const f = displayIn.files?.[0];
        displayIn.value = '';
        if (!f) return;
        const reader = new FileReader();
        reader.onload = (ev) => {
            const bytes = new Uint8Array(ev.target.result);
            void loadDisplayFromBytes(bytes, f.name);
        };
        reader.onerror = () => {
            showStatus(`display load error: ${reader.error?.message ?? 'file read failed'}`, true);
            console.error(`[composer] FileReader failed to read "${f.name}":`, reader.error);
        };
        reader.readAsArrayBuffer(f);
    });
    dispRow.appendChild(loadDisplayBtn);
    dispRow.appendChild(displayIn);
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

    const diffusionRow = document.createElement('div'); diffusionRow.className = 'top-row';
    diffusionRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Diffusion' }));
    const diffusionSlider = document.createElement('input');
    diffusionSlider.type = 'range';
    diffusionSlider.min = String(DIFFUSION_MIN_PERCENT);
    diffusionSlider.max = String(DIFFUSION_MAX_PERCENT);
    diffusionSlider.step = '1';
    diffusionSlider.value = String(state.diffusionPercent);
    diffusionSlider.title = 'Adjust LED halo size';
    const diffusionValue = document.createElement('span');
    diffusionValue.className = 'diffusion-value';
    diffusionValue.textContent = `${state.diffusionPercent}%`;
    diffusionSlider.addEventListener('input', () => {
        const percent = clampDiffusionPercent(diffusionSlider.value);
        state.diffusionPercent = percent;
        diffusionValue.textContent = `${percent}%`;
        void setWorkerDiffusion(percent).then((result) => {
            if (!result?.ok) {
                logDebug('display', `diffusion update deferred: ${result?.reason ?? 'renderer not ready'}`);
            }
        });
    });
    diffusionRow.appendChild(diffusionSlider);
    diffusionRow.appendChild(diffusionValue);
    section.appendChild(diffusionRow);

    const ioRow = document.createElement('div'); ioRow.className = 'top-row action-row';
    const saveBtn = document.createElement('button'); saveBtn.textContent = 'Download .psc';
    const savePlaylistBtn = document.createElement('button'); savePlaylistBtn.textContent = 'Add to playlist';
    const importPlaylistBtn = document.createElement('button'); importPlaylistBtn.textContent = 'Import';
    const downloadPlaylistBtn = document.createElement('button'); downloadPlaylistBtn.textContent = 'Download playlist';
    const replacePlaylistBtn = document.createElement('button'); replacePlaylistBtn.textContent = 'Replace playlist';
    const clearPlaylistBtn = document.createElement('button'); clearPlaylistBtn.textContent = 'Clear playlist';
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
    const replaceIn = document.createElement('input');
    replaceIn.type = 'file'; replaceIn.accept = '.json,application/json';
    replaceIn.style.display = 'none';
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
    const playlistIoBusy = state.playlistBusy || state.deployBusy;
    downloadPlaylistBtn.disabled = playlistIoBusy || !playlistBrowsingAvailable() || state.playlistItems.length === 0;
    if (!playlistBrowsingAvailable()) {
        downloadPlaylistBtn.title = state.playlistApiChecked
            ? 'Load a playlist first, or run web/serve.sh to download build/psc'
            : 'Checking local playlist API';
    } else if (state.playlistItems.length === 0) {
        downloadPlaylistBtn.title = 'The playlist is empty';
    } else {
        downloadPlaylistBtn.title = 'Download every composition as a single playlist file';
    }
    downloadPlaylistBtn.addEventListener('click', () => {
        void downloadPlaylistBundle();
    });
    // With the local server this replaces build/psc on disk; on the static
    // build it loads the bundle into an in-browser playlist instead.
    replacePlaylistBtn.textContent = state.playlistApiAvailable ? 'Replace playlist' : 'Load playlist';
    replacePlaylistBtn.disabled = playlistIoBusy || !state.playlistApiChecked;
    if (!state.playlistApiChecked) {
        replacePlaylistBtn.title = 'Checking local playlist API';
    } else if (state.deployBusy) {
        replacePlaylistBtn.title = 'Wait for deploy to finish before changing build/psc';
    } else if (state.playlistApiAvailable) {
        replacePlaylistBtn.title = 'Replace all saved compositions with a playlist file';
    } else {
        replacePlaylistBtn.title = 'Load a playlist file into an in-browser playlist';
    }
    replacePlaylistBtn.addEventListener('click', () => replaceIn.click());
    replaceIn.addEventListener('change', () => {
        const bundleFile = replaceIn.files?.[0] ?? null;
        replaceIn.value = '';
        void loadPlaylistBundleFile(bundleFile);
    });
    clearPlaylistBtn.disabled = playlistIoBusy || !playlistBrowsingAvailable() || state.playlistItems.length === 0;
    if (!playlistBrowsingAvailable()) {
        clearPlaylistBtn.title = state.playlistApiChecked
            ? 'Load a playlist first, or run web/serve.sh to clear build/psc'
            : 'Checking local playlist API';
    } else if (state.playlistItems.length === 0) {
        clearPlaylistBtn.title = 'The playlist is empty';
    } else if (state.playlistLocalMode) {
        clearPlaylistBtn.title = 'Clear the loaded in-browser playlist';
    } else {
        clearPlaylistBtn.title = 'Delete every composition from build/psc';
    }
    clearPlaylistBtn.addEventListener('click', () => {
        void clearPlaylist();
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
                clearPlaylistBaseline({ clearSelection: true });
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
    ioRow.appendChild(loadBtn);
    ioRow.appendChild(saveBtn);
    if (state.composerTab === 'new') {
        ioRow.appendChild(savePlaylistBtn);
    }
    if (state.composerTab === 'playlist') {
        ioRow.appendChild(importPlaylistBtn);
        ioRow.appendChild(downloadPlaylistBtn);
        ioRow.appendChild(replacePlaylistBtn);
        ioRow.appendChild(clearPlaylistBtn);
    }
    ioRow.appendChild(resetBtn);
    ioRow.appendChild(fileIn);
    ioRow.appendChild(importIn);
    ioRow.appendChild(replaceIn);
    section.appendChild(ioRow);

    const tabs = document.createElement('div');
    tabs.className = 'composer-tabs';
    for (const tab of [
        { id: 'new', label: 'New composition' },
        { id: 'playlist', label: 'Playlist' },
        { id: 'palette-generator', label: 'Palette Generator' },
    ]) {
        const btn = document.createElement('button');
        btn.type = 'button';
        btn.textContent = tab.label;
        btn.className = state.composerTab === tab.id ? 'active' : '';
        btn.addEventListener('click', () => {
            if (state.composerTab === tab.id) return;
            if (tab.id === 'palette-generator') {
                syncPaletteGeneratorFromScene();
            }
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

    if (state.composerTab === 'playlist') {
        const compatiblePlaylistItems = supportedPlaylistItems();
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
            || compatiblePlaylistItems.length === 0
            || !state.deployTarget;
        if (compatiblePlaylistItems.length === 0) {
            deployBtn.title = 'Save at least one compatible composition to build/psc first';
        } else if (state.displayBrightnessBusy) {
            deployBtn.title = 'Wait for brightness to save';
        } else if (state.deployDetectBusy) {
            deployBtn.title = 'Wait for device detection to finish';
        } else if (!state.deployTarget) {
            deployBtn.title = 'Choose an MCU target first';
        } else {
            deployBtn.title = `Build and upload ${compatiblePlaylistItems.length} playlist composition${compatiblePlaylistItems.length === 1 ? '' : 's'} to ${deployTargetLabel(state.deployTarget)}`;
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

        const playlistRow = document.createElement('div'); playlistRow.className = 'top-row';
        playlistRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Files' }));
        if (!state.playlistApiChecked) {
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: 'checking local playlist API',
            }));
        } else if (!playlistBrowsingAvailable()) {
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: 'load a playlist file to browse it here',
            }));
        } else if (state.playlistItems.length === 0) {
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: 'no saved compositions in build/psc',
            }));
        } else {
            const location = state.playlistLocalMode ? 'in loaded playlist' : 'in build/psc';
            const incompatibleCount = state.playlistItems.length - compatiblePlaylistItems.length;
            const compatibilitySuffix = incompatibleCount > 0 ? `, ${incompatibleCount} incompatible` : '';
            playlistRow.appendChild(Object.assign(document.createElement('span'), {
                className: 'muted-note',
                textContent: `${compatiblePlaylistItems.length}/${state.playlistItems.length} compatible composition${compatiblePlaylistItems.length === 1 ? '' : 's'} ${location}${compatibilitySuffix}`,
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

        if (playlistBrowsingAvailable() && state.playlistItems.length > 0) {
            const playlistList = document.createElement('div');
            playlistList.className = 'playlist-list';
            for (const file of state.playlistItems) {
                const item = document.createElement('div');
                const itemClasses = ['playlist-item'];
                const isSelected = file.name === state.playlistSelected;
                const isKeyboardActive = isSelected && file.name === state.playlistActiveName;
                const isSupported = playlistFileSupported(file);
                if (isSelected) itemClasses.push('selected');
                if (isKeyboardActive) itemClasses.push('active');
                if (!isSupported) itemClasses.push('unsupported');
                item.className = itemClasses.join(' ');
                const loadSavedBtn = document.createElement('button');
                loadSavedBtn.type = 'button';
                loadSavedBtn.textContent = file.name;
                loadSavedBtn.title = isSupported
                    ? `Load ${file.name}`
                    : `${file.name}: ${playlistFileIssue(file)}`;
                loadSavedBtn.className = 'playlist-load-btn';
                loadSavedBtn.dataset.playlistName = file.name;
                loadSavedBtn.disabled = !isSupported || state.playlistBusy || state.deployBusy;
                loadSavedBtn.addEventListener('click', () => {
                    void loadPlaylistScene(file.name, { activate: true });
                });
                item.appendChild(loadSavedBtn);
                // build/psc mutations (dirty edits, delete) only apply when the
                // local server API is running, not in the in-browser playlist.
                if (!state.playlistLocalMode
                    && file.name === state.playlistSelected && file.name === state.playlistBaselineName && state.playlistDirty) {
                    const dirtyActions = document.createElement('div');
                    dirtyActions.className = 'playlist-dirty-actions';

                    const updateBtn = document.createElement('button');
                    updateBtn.type = 'button';
                    updateBtn.textContent = 'Update';
                    updateBtn.title = `Overwrite ${file.name}`;
                    updateBtn.disabled = state.playlistBusy || state.deployBusy;
                    updateBtn.addEventListener('click', () => {
                        void updateSelectedPlaylistScene();
                    });
                    dirtyActions.appendChild(updateBtn);

                    const saveNewBtn = document.createElement('button');
                    saveNewBtn.type = 'button';
                    saveNewBtn.textContent = 'Save as new';
                    saveNewBtn.title = `Save a new copy based on ${file.name}`;
                    saveNewBtn.disabled = state.playlistBusy || state.deployBusy;
                    saveNewBtn.addEventListener('click', () => {
                        void saveSelectedPlaylistSceneAsNew();
                    });
                    dirtyActions.appendChild(saveNewBtn);

                    const discardBtn = document.createElement('button');
                    discardBtn.type = 'button';
                    discardBtn.textContent = 'Discard';
                    discardBtn.title = `Reload ${file.name}`;
                    discardBtn.disabled = state.playlistBusy || state.deployBusy;
                    discardBtn.addEventListener('click', discardSelectedPlaylistChanges);
                    dirtyActions.appendChild(discardBtn);

                    item.appendChild(dirtyActions);
                }
                if (!state.playlistLocalMode) {
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
                    item.appendChild(deleteSavedBtn);
                }
                playlistList.appendChild(item);
            }
            section.appendChild(playlistList);
        }
    }

    if (state.composerTab === 'palette-generator') {
        section.appendChild(renderPaletteGeneratorSection());
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
    const showSceneEditor = shouldShowSceneEditor();
    if (showSceneEditor) ensurePatternTypeAvailableForDisplay();
    panelContentEl.innerHTML = '';
    const title = document.createElement('h1');
    title.textContent = 'Composer';
    panelContentEl.appendChild(title);
    panelContentEl.appendChild(renderTopSection());
    if (showSceneEditor) {
        panelContentEl.appendChild(renderPatternSection());
        panelContentEl.appendChild(renderTransformsSection());
    }
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
    document.addEventListener('click', handlePlaylistDocumentClick);
    document.addEventListener('focusin', handlePlaylistDocumentFocus);
    document.addEventListener('keydown', handlePlaylistKeydown);
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
        // A custom .pds display only lives on whichever module handled the load.
        // If the worker came up after a main-thread-fallback load (or a reload
        // restored the display from sessionStorage), push the .pds into the
        // worker before the scene so it renders on the custom geometry.
        if (state.displayId === DISPLAY_LOADED && state.loadedDisplay) {
            const loadSeq = nextRenderSeq();
            const loadResult = await callLoadDisplay(state.loadedDisplay.bytes, loadSeq);
            if (loadResult.status !== 0) {
                logDebug('error', `worker .pds resync failed via ${loadResult.target}: code ${loadResult.status}`
                    + (loadResult.detail ? ` — ${loadResult.detail}` : ''));
            }
        }
        const sceneSeq = nextRenderSeq();
        state.latestSceneSeq = sceneSeq;
        await pushSceneNow(state.latestEvent, sceneSeq);
        await setWorkerBloom(bloomPreference);
        await setWorkerDiffusion(state.diffusionPercent);
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
          && typeof M._composer_load_display_pds === 'function'
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
