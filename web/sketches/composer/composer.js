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

const sceneStore = createSceneStore(DEFAULT_SCENE());

const state = {
    pushFrame: null,             // animation-frame push handle
    latestEvent: null,           // most-recent scene-store emission
    statusEl: null,              // toast target
    debugLogEl: null,            // FastLED-side debug console
};

function mutateScene(path, value, mutator) {
    sceneStore.mutate({ path, value }, mutator);
}

function formatValue(value) {
    const text = typeof value === 'string' ? value : (JSON.stringify(value) ?? String(value));
    if (text.length <= 140) return text;
    return `${text.slice(0, 137)}...`;
}

function signalSummary(signal) {
    return {
        id: signal?.id,
        params: signal?.params ?? {},
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

    const panel = document.createElement('div');
    panel.id = 'composer-debug-console';

    const title = document.createElement('div');
    title.className = 'composer-debug-title';
    title.textContent = 'Composer event console';
    panel.appendChild(title);

    state.debugLogEl = document.createElement('div');
    state.debugLogEl.className = 'composer-debug-log';
    panel.appendChild(state.debugLogEl);

    const anchor = document.getElementById('fastled-async-controls')
        ?? document.getElementById('canvas-container');
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
    const manager = window.fastLEDWorkerManager;
    if (!manager?.isWorkerActive || !manager.worker
            || typeof manager.sendMessageWithResponse !== 'function') {
        return null;
    }
    return manager;
}

async function callApplyScene(bytes) {
    const workerManager = getWorkerManager();
    if (workerManager) {
        const buffer = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
        const response = await workerManager.sendMessageWithResponse(
            { type: 'composer_apply_scene', payload: { bytes: buffer } },
            [buffer]
        );
        return { status: response?.status ?? -1, target: 'worker' };
    }

    const M = getWasmModule();
    if (!M || !M._composer_apply_scene || !M._malloc) return { status: -1, target: 'none' };
    const ptr = M._malloc(bytes.length);
    M.HEAPU8.set(bytes, ptr);
    const status = M._composer_apply_scene(ptr, bytes.length);
    M._free(ptr);
    return { status, target: 'main' };
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
function schedulePush() {
    const event = state.latestEvent;
    if (state.pushFrame !== null) {
        logDebug('coalesce', `queued latest rev ${event?.revision ?? '?'} before next frame`);
        return;
    }
    state.pushFrame = requestAnimationFrame(async () => {
        state.pushFrame = null;
        const pushEvent = state.latestEvent;
        try {
            const bytes = encodeScene(sceneStore.scene);
            const result = await callApplyScene(bytes);
            const status = result.status;
            if (status === 0) {
                showStatus(`applied (${bytes.length} bytes)`, false);
                logDebug(
                    'update',
                    `display updated rev ${pushEvent?.revision ?? '?'} via ${result.target} (${bytes.length} bytes): ${formatValue(sceneSummary(sceneStore.scene))}`
                );
            } else {
                showStatus(`decode error: ${STATUS_LABELS[status] ?? `code ${status}`}`, true);
                logDebug(
                    'error',
                    `display update failed rev ${pushEvent?.revision ?? '?'} via ${result.target}: ${STATUS_LABELS[status] ?? `code ${status}`}`
                );
            }
        } catch (e) {
            showStatus(`encode error: ${e.message}`, true);
            logDebug('error', `encode failed rev ${pushEvent?.revision ?? '?'}: ${e.message}`);
        }
    });
}

sceneStore.subscribe((event) => {
    state.latestEvent = event;
    logDebug(
        'emit',
        `emit rev ${event.revision}: ${event.change.path} = ${formatValue(event.change.value)}`
    );
    schedulePush();
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
            const header = document.createElement('div'); header.className = 'transform-header';
            const title = document.createElement('span'); title.className = 'transform-title';
            title.textContent = `${i + 1}. ${def.label}`;
            header.appendChild(title);

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

    const ioRow = document.createElement('div'); ioRow.className = 'top-row';
    const saveBtn = document.createElement('button'); saveBtn.textContent = 'Save .psc';
    const loadBtn = document.createElement('button'); loadBtn.textContent = 'Load .psc';
    const fileIn  = document.createElement('input');
    fileIn.type = 'file'; fileIn.accept = '.psc,application/octet-stream';
    fileIn.style.display = 'none';
    saveBtn.addEventListener('click', () => {
        try {
            const bytes = encodeScene(sceneStore.scene);
            const blob = new Blob([bytes], { type: 'application/octet-stream' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url; a.download = 'composer.psc';
            document.body.appendChild(a); a.click(); document.body.removeChild(a);
            URL.revokeObjectURL(url);
        } catch (e) {
            showStatus(`save error: ${e.message}`, true);
        }
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
    ioRow.appendChild(saveBtn); ioRow.appendChild(loadBtn); ioRow.appendChild(fileIn);
    section.appendChild(ioRow);

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

function rebuildPanel() {
    if (!panelEl) return;
    panelEl.innerHTML = '';
    const title = document.createElement('h1');
    title.textContent = 'Composer';
    panelEl.appendChild(title);
    panelEl.appendChild(renderTopSection());
    panelEl.appendChild(renderPatternSection());
    panelEl.appendChild(renderTransformsSection());
}

function mountPanel() {
    panelEl = document.createElement('aside');
    panelEl.id = 'composer-panel';
    document.body.appendChild(panelEl);
    mountDebugConsole();
    rebuildPanel();
    // Push the initial scene once WASM is ready.
    sceneStore.notify({ path: 'scene.initial', value: sceneSummary(sceneStore.scene) });
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

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', bootComposer);
} else {
    bootComposer();
}
