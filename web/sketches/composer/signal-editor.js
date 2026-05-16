// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// Recursive Sf16Signal editor. Each slot renders as
//   [type dropdown] [params row]
// where each param of kind 'signal' is itself a SignalSlot (recursion).
// The model is a plain object: { id: 'sine', params: { phaseVelocity: {...}, phaseOffset: 0 } }
// Whenever the model mutates, onChange(updatedModel) fires. The parent
// (composer.js) re-encodes and pushes to WASM.

import { SIGNALS, DEFAULT_SIGNAL } from './schema.js';

// Render param controls for a leaf or modulator. Mutates `signal.params`
// in place; onChange() is invoked after every keystroke / dropdown change.
function renderParamControl(p, signal, onChange) {
    const wrap = document.createElement('label');
    wrap.className = 'param';
    const labelText = document.createElement('span');
    labelText.className = 'param-label';
    labelText.textContent = p.label ?? p.name;
    wrap.appendChild(labelText);

    if (p.kind === 'signal') {
        const inner = renderSignalSlot(
            signal.params[p.name] ?? DEFAULT_SIGNAL(),
            (next) => { signal.params[p.name] = next; onChange(); }
        );
        wrap.appendChild(inner);
        return wrap;
    }

    if (p.kind === 'enum') {
        const sel = document.createElement('select');
        for (let i = 0; i < p.options.length; ++i) {
            const opt = document.createElement('option');
            opt.value = i; opt.textContent = p.options[i];
            sel.appendChild(opt);
        }
        sel.value = signal.params[p.name] ?? p.default ?? 0;
        sel.addEventListener('change', () => {
            signal.params[p.name] = parseInt(sel.value, 10);
            onChange();
        });
        wrap.appendChild(sel);
        return wrap;
    }

    if (p.kind === 'bool') {
        const cb = document.createElement('input');
        cb.type = 'checkbox';
        cb.checked = !!(signal.params[p.name] ?? p.default ?? 0);
        cb.addEventListener('change', () => {
            signal.params[p.name] = cb.checked ? 1 : 0;
            onChange();
        });
        wrap.appendChild(cb);
        return wrap;
    }

    // Numeric: 'permille', 'f16', 'u8', 'u16', 'u32', 'i32'.
    // permille and f16 get a slider; the rest get a number input.
    // Bound match the C++ helper expectations:
    //   - permille → constant(uint16_t) takes 0..1000 (1000 = 1.0)
    //   - f16      → raw f16 fraction, full uint16 range (65535 ≈ 1.0)
    const isSlider = p.kind === 'permille' || p.kind === 'f16';
    const input = document.createElement('input');
    input.type = isSlider ? 'range' : 'number';
    if (isSlider) {
        input.min = '0';
        input.max = p.kind === 'permille' ? '1000' : '65535';
        input.step = '1';
    }
    input.value = signal.params[p.name] ?? p.default ?? 0;
    input.addEventListener('input', () => {
        const v = parseInt(input.value, 10);
        if (!Number.isNaN(v)) {
            signal.params[p.name] = v;
            onChange();
        }
    });
    wrap.appendChild(input);

    if (isSlider) {
        const readout = document.createElement('span');
        readout.className = 'param-readout';
        const updateReadout = () => { readout.textContent = input.value; };
        updateReadout();
        input.addEventListener('input', updateReadout);
        wrap.appendChild(readout);
    }

    return wrap;
}

// Render one Sf16Signal slot. Returns a DOM element. `model` is the
// signal object; mutations (id change, param change) call onChange(next)
// with the new model so the parent can update its tree.
export function renderSignalSlot(model, onChange) {
    const slot = document.createElement('div');
    slot.className = 'signal-slot';

    // Type dropdown
    const typeSel = document.createElement('select');
    typeSel.className = 'signal-type';
    for (const [id, def] of Object.entries(SIGNALS)) {
        const opt = document.createElement('option');
        opt.value = id;
        opt.textContent = def.label;
        typeSel.appendChild(opt);
    }
    typeSel.value = model.id;

    const paramsRow = document.createElement('div');
    paramsRow.className = 'signal-params';

    function rebuildParams() {
        paramsRow.innerHTML = '';
        const def = SIGNALS[model.id];
        for (const p of def.params) {
            paramsRow.appendChild(renderParamControl(p, model, () => onChange(model)));
        }
    }

    typeSel.addEventListener('change', () => {
        const newId = typeSel.value;
        // Reset params to schema defaults when switching type. Inner signal
        // slots default to constant(500).
        const def = SIGNALS[newId];
        const newParams = {};
        for (const p of def.params) {
            if (p.kind === 'signal') newParams[p.name] = DEFAULT_SIGNAL();
            else newParams[p.name] = p.default ?? 0;
        }
        model = { id: newId, params: newParams };
        onChange(model);
        rebuildParams();
    });

    slot.appendChild(typeSel);
    slot.appendChild(paramsRow);
    rebuildParams();
    return slot;
}
