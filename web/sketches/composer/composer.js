// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// Top-level composer panel. Builds the entire UI after FastLED's WASM
// module is ready, owns the scene model, debounces encode-and-push.
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
// Scene model — the source of truth on the JS side.
// ─────────────────────────────────────────────────────────────────────

const state = {
    display: 0,                  // 0 = fabric, 1 = round
    scene: DEFAULT_SCENE(),
    pushTimer: null,             // debounce handle
    statusEl: null,              // toast target
};

// ─────────────────────────────────────────────────────────────────────
// WASM bridge
// ─────────────────────────────────────────────────────────────────────

// FastLED runs the Emscripten module on the main thread but does not
// expose it as `window.Module`. The controller (built by FastLED's
// runtime) holds the module reference under `.module`.
function getWasmModule() {
    const ctrl = window.getFastLEDController?.();
    return ctrl?.module ?? null;
}

function callApplyScene(bytes) {
    const M = getWasmModule();
    if (!M || !M._composer_apply_scene || !M._malloc) return -1;
    const ptr = M._malloc(bytes.length);
    M.HEAPU8.set(bytes, ptr);
    const status = M._composer_apply_scene(ptr, bytes.length);
    M._free(ptr);
    return status;
}

function callSetDisplay(which) {
    const M = getWasmModule();
    if (!M || !M._composer_set_display) return;
    M._composer_set_display(which);
}

const STATUS_LABELS = [
    'OK', 'BAD_MAGIC', 'BAD_VERSION', 'TRUNCATED', 'UNKNOWN_TAG', 'BAD_ENUM',
];

function showStatus(text, isError) {
    if (!state.statusEl) return;
    state.statusEl.textContent = text;
    state.statusEl.className = isError ? 'status err' : 'status ok';
}

// Debounced encode + push.
function schedulePush() {
    clearTimeout(state.pushTimer);
    state.pushTimer = setTimeout(() => {
        try {
            const bytes = encodeScene(state.scene);
            const status = callApplyScene(bytes);
            if (status === 0) {
                showStatus(`applied (${bytes.length} bytes)`, false);
            } else {
                showStatus(`decode error: ${STATUS_LABELS[status] ?? `code ${status}`}`, true);
            }
        } catch (e) {
            showStatus(`encode error: ${e.message}`, true);
        }
    }, 100);
}

// ─────────────────────────────────────────────────────────────────────
// Static-config controls (re-used between pattern + transform forms)
// ─────────────────────────────────────────────────────────────────────

function renderConfigControl(c, configObj, onChange) {
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
            configObj[c.name] = parseInt(sel.value, 10); onChange();
        });
        wrap.appendChild(sel);
        return wrap;
    }

    if (c.kind === 'bool') {
        const cb = document.createElement('input');
        cb.type = 'checkbox';
        cb.checked = !!(configObj[c.name] ?? c.default ?? 0);
        cb.addEventListener('change', () => {
            configObj[c.name] = cb.checked ? 1 : 0; onChange();
        });
        wrap.appendChild(cb);
        return wrap;
    }

    const input = document.createElement('input');
    input.type = 'number';
    input.value = configObj[c.name] ?? c.default ?? 0;
    input.addEventListener('input', () => {
        const v = parseInt(input.value, 10);
        if (!Number.isNaN(v)) { configObj[c.name] = v; onChange(); }
    });
    wrap.appendChild(input);
    return wrap;
}

function renderConfigForm(configSchema, configObj, onChange) {
    const form = document.createElement('div');
    form.className = 'config-form';
    for (const c of configSchema) {
        form.appendChild(renderConfigControl(c, configObj, onChange));
    }
    return form;
}

function renderSignalsForm(signalsSchema, signalsObj, onChange) {
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
            (next) => { signalsObj[s.name] = next; onChange(); }
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
    sel.value = state.scene.pattern.id;
    row.appendChild(sel);
    section.appendChild(row);

    const body = document.createElement('div'); body.className = 'pattern-body';
    section.appendChild(body);

    function rebuildBody() {
        body.innerHTML = '';
        const def = PATTERNS[state.scene.pattern.id];
        hydrateConfig(def.config, state.scene.pattern.config);
        hydrateSignals(def.signals, state.scene.pattern.signals);
        body.appendChild(renderConfigForm(def.config, state.scene.pattern.config, schedulePush));
        body.appendChild(renderSignalsForm(def.signals, state.scene.pattern.signals, schedulePush));
    }

    sel.addEventListener('change', () => {
        state.scene.pattern = { id: sel.value, config: {}, signals: {} };
        rebuildBody();
        schedulePush();
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
        state.scene.transforms.forEach((t, i) => {
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
            btnDown.disabled = (i === state.scene.transforms.length - 1);
            btnUp.addEventListener('click', () => {
                [state.scene.transforms[i - 1], state.scene.transforms[i]] =
                    [state.scene.transforms[i], state.scene.transforms[i - 1]];
                rebuildList(); schedulePush();
            });
            btnDown.addEventListener('click', () => {
                [state.scene.transforms[i + 1], state.scene.transforms[i]] =
                    [state.scene.transforms[i], state.scene.transforms[i + 1]];
                rebuildList(); schedulePush();
            });
            btnRm.addEventListener('click', () => {
                state.scene.transforms.splice(i, 1);
                rebuildList(); schedulePush();
            });
            header.appendChild(btnUp); header.appendChild(btnDown); header.appendChild(btnRm);
            card.appendChild(header);

            card.appendChild(renderConfigForm(def.config,   t.config,  schedulePush));
            card.appendChild(renderSignalsForm(def.signals, t.signals, schedulePush));
            list.appendChild(card);
        });
    }

    addBtn.addEventListener('click', () => {
        state.scene.transforms.push({ id: addSel.value, config: {}, signals: {} });
        rebuildList(); schedulePush();
    });

    rebuildList();
    return section;
}

// ─────────────────────────────────────────────────────────────────────
// Display + palette + save/load + status
// ─────────────────────────────────────────────────────────────────────

function renderTopSection() {
    const section = document.createElement('section'); section.className = 'panel-section';
    const h = document.createElement('h2'); h.textContent = 'Scene'; section.appendChild(h);

    const dispRow = document.createElement('div'); dispRow.className = 'top-row';
    dispRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Display' }));
    const dispSel = document.createElement('select');
    [['Fabric', 0], ['Round', 1]].forEach(([label, val]) => {
        const o = document.createElement('option'); o.value = val; o.textContent = label;
        dispSel.appendChild(o);
    });
    dispSel.value = state.display;
    dispSel.addEventListener('change', () => {
        state.display = parseInt(dispSel.value, 10);
        callSetDisplay(state.display);
    });
    dispRow.appendChild(dispSel);
    section.appendChild(dispRow);

    const palRow = document.createElement('div'); palRow.className = 'top-row';
    palRow.appendChild(Object.assign(document.createElement('span'), { textContent: 'Palette' }));
    const palSel = document.createElement('select');
    for (const p of PALETTES) {
        const o = document.createElement('option'); o.value = p.id; o.textContent = p.name;
        palSel.appendChild(o);
    }
    palSel.value = state.scene.paletteId;
    palSel.addEventListener('change', () => {
        state.scene.paletteId = parseInt(palSel.value, 10);
        schedulePush();
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
            const bytes = encodeScene(state.scene);
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
                state.scene = decoded;
                rebuildPanel();           // rerender the whole panel
                schedulePush();
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
    rebuildPanel();
    // Push the initial scene once WASM is ready.
    schedulePush();
}

// Wait for FastLED's runtime to expose getFastLEDController() with our
// composer exports attached to the Emscripten module.
function waitForModule(cb) {
    const check = () => {
        const M = getWasmModule();
        if (M && typeof M._composer_apply_scene === 'function'
              && typeof M._composer_set_display === 'function'
              && typeof M._malloc === 'function'
              && M.HEAPU8) {
            cb();
        } else {
            setTimeout(check, 100);
        }
    };
    check();
}

document.addEventListener('DOMContentLoaded', () => {
    waitForModule(mountPanel);
});
