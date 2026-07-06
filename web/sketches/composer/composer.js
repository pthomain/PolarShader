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
    pushFrame: null,             // animation-frame push handle
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
    playlistApiChecked: false,
    playlistApiAvailable: false,
    playlistItems: [],
    playlistSelected: '',
    playlistBusy: false,
};

// Display geometry tags — must match the display enum in composer.ino.
const DISPLAYS = [
    { id: 0, name: 'Fabric (20×20 matrix)' },
    { id: 1, name: 'Round (241-pixel radial)' },
];

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
        state.playlistApiChecked = true;
        if (state.playlistApiAvailable) {
            await refreshPlaylistItems();
        }
    } catch {
        state.playlistApiAvailable = false;
        state.playlistApiChecked = true;
        state.playlistItems = [];
        state.playlistSelected = '';
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

async function saveCurrentSceneToPlaylist() {
    if (!state.playlistApiAvailable || state.playlistBusy) return;
    const filename = (state.composerTab === 'playlist' && state.playlistSelected)
        ? state.playlistSelected
        : pscDownloadFilename(sceneStore.scene);
    state.playlistBusy = true;
    rebuildPanel();
    let statusText = null;
    let statusError = false;
    try {
        const bytes = encodeScene(sceneStore.scene);
        const body = await fetchPlaylistJson('/api/playlist/save', {
            method: 'POST',
            params: { name: filename },
            headers: { 'Content-Type': 'application/octet-stream' },
            body: bytes,
        });
        const savedName = body.file?.name ?? filename;
        state.playlistSelected = savedName;
        await refreshPlaylistItems();
        statusText = `saved ${savedName}`;
        logDebug('playlist', `saved ${savedName} (${bytes.length} bytes)`);
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
    while (state.debugLogEl.children.length > 80) {
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
// (patch_render_worker / patch_threejs_bloom_toggle). State is mirrored in
// `state.bloomEnabled` so it can be replayed when the worker (re)activates.
let bloomPreference = true;

async function setWorkerBloom(enabled) {
    bloomPreference = !!enabled;
    const workerManager = getWorkerManager() ?? await waitForWorkerManager(2000);
    if (!workerManager) return { ok: false, reason: 'worker not active' };
    try {
        return await workerManager.sendMessageWithResponse(
            { type: 'composer_set_bloom', payload: { enabled: bloomPreference } }
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

function schedulePush(seq = nextRenderSeq()) {
    state.latestSceneSeq = seq;
    const event = state.latestEvent;
    if (state.pushFrame !== null) {
        logDebug('coalesce', `queued latest rev ${event?.revision ?? '?'} seq ${seq} before next frame`);
        return;
    }
    state.pushFrame = requestAnimationFrame(async () => {
        state.pushFrame = null;
        const pushEvent = state.latestEvent;
        const pushSeq = state.latestSceneSeq;
        await pushSceneNow(pushEvent, pushSeq);
    });
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
    const h = document.createElement('h2'); h.textContent = 'Scene'; section.appendChild(h);

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
            }
        });
        tabs.appendChild(btn);
    }
    section.appendChild(tabs);

    const ioRow = document.createElement('div'); ioRow.className = 'top-row';
    const saveBtn = document.createElement('button'); saveBtn.textContent = 'Save .psc';
    const savePlaylistBtn = document.createElement('button'); savePlaylistBtn.textContent = 'Save to playlist';
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
    saveBtn.addEventListener('click', () => {
        try {
            const bytes = encodeScene(sceneStore.scene);
            const blob = new Blob([bytes], { type: 'application/octet-stream' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url; a.download = pscDownloadFilename(sceneStore.scene);
            document.body.appendChild(a); a.click(); document.body.removeChild(a);
            URL.revokeObjectURL(url);
        } catch (e) {
            showStatus(`save error: ${e.message}`, true);
        }
    });
    savePlaylistBtn.disabled = !state.playlistApiAvailable || state.playlistBusy;
    if (!state.playlistApiAvailable) {
        savePlaylistBtn.title = state.playlistApiChecked
            ? 'Run web/serve.sh locally to save directly into build/psc'
            : 'Checking local playlist API';
    } else if (state.composerTab === 'playlist' && state.playlistSelected) {
        savePlaylistBtn.title = `Overwrite build/psc/${state.playlistSelected}`;
    } else {
        savePlaylistBtn.title = 'Save a new .psc file into build/psc';
    }
    savePlaylistBtn.addEventListener('click', () => {
        void saveCurrentSceneToPlaylist();
    });
    loadBtn.addEventListener('click', () => fileIn.click());
    fileIn.addEventListener('change', () => {
        const f = fileIn.files?.[0]; if (!f) return;
        const reader = new FileReader();
        reader.onload = (ev) => {
            try {
                const bytes = new Uint8Array(ev.target.result);
                const decoded = decodeScene(bytes);
                sceneStore.setScene(decoded, {
                    path: 'scene.load',
                    value: sceneSummary(decoded),
                });
                rebuildPanel();           // rerender the whole panel
            } catch (e) {
                showStatus(`load error: ${e.message}`, true);
            }
        };
        reader.readAsArrayBuffer(f);
    });
    resetBtn.addEventListener('click', resetRenderer);
    ioRow.appendChild(saveBtn);
    ioRow.appendChild(savePlaylistBtn);
    ioRow.appendChild(loadBtn);
    ioRow.appendChild(resetBtn);
    ioRow.appendChild(fileIn);
    section.appendChild(ioRow);

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
        const presetBtn = document.createElement('button'); presetBtn.textContent = 'Load';
        presetBtn.addEventListener('click', () => {
            const preset = PF_PRESETS.find((p) => p.id === presetSel.value);
            if (!preset) return;
            const nextScene = preset.scene();
            sceneStore.setScene(nextScene, {
                path: 'scene.preset',
                value: preset.label,
            });
            rebuildPanel();
        });
        presetRow.appendChild(presetSel); presetRow.appendChild(presetBtn);
        section.appendChild(presetRow);
    }

    if (state.composerTab === 'playlist') {
        const playlistRow = document.createElement('div'); playlistRow.className = 'top-row';
        playlistRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Saved' }));
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
            const playlistSel = document.createElement('select');
            const placeholder = document.createElement('option');
            placeholder.value = '';
            placeholder.textContent = 'Choose saved composition';
            playlistSel.appendChild(placeholder);
            for (const file of state.playlistItems) {
                const o = document.createElement('option');
                o.value = file.name;
                o.textContent = file.name;
                playlistSel.appendChild(o);
            }
            playlistSel.value = state.playlistSelected;
            playlistSel.disabled = state.playlistBusy;
            playlistSel.addEventListener('change', () => {
                if (playlistSel.value) void loadPlaylistScene(playlistSel.value);
            });
            playlistRow.appendChild(playlistSel);
        }
        const refreshBtn = document.createElement('button');
        refreshBtn.textContent = 'Refresh';
        refreshBtn.disabled = !state.playlistApiAvailable || state.playlistBusy;
        refreshBtn.addEventListener('click', () => {
            void refreshPlaylistItems({ rebuild: true });
        });
        playlistRow.appendChild(refreshBtn);
        section.appendChild(playlistRow);
    }

    // Bloom on/off: toggles the ThreeJS bloom post-processing pass in the render
    // worker's graphics manager (see setWorkerBloom).
    const bloomRow = document.createElement('div'); bloomRow.className = 'top-row';
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
    bloomRow.appendChild(bloomLabel);
    section.appendChild(bloomRow);

    state.statusEl = document.createElement('div');
    state.statusEl.className = 'status';
    state.statusEl.textContent = 'idle';
    section.appendChild(state.statusEl);

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
