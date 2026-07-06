// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// Reusable numeric stepper: wraps a numeric <input> (range or number) with
// manual [−] and [+] buttons that nudge the value by `step`, clamped to the
// input's own min/max. Clicking a button dispatches a native 'input' event on
// the wrapped input, so any listeners already attached to the input react
// exactly as they would to a keyboard/drag edit — no extra wiring needed.

// Attach −/+ controls to `input` and return the wrapper element to insert in
// place of the bare input. Options: { step } (default 1).
export function attachStepper(input, { step = 1 } = {}) {
    const group = document.createElement('div');
    group.className = 'stepper';

    const minus = document.createElement('button');
    minus.type = 'button';
    minus.className = 'stepper-btn';
    minus.textContent = '−';
    minus.setAttribute('aria-label', 'decrease');

    const plus = document.createElement('button');
    plus.type = 'button';
    plus.className = 'stepper-btn';
    plus.textContent = '+';
    plus.setAttribute('aria-label', 'increase');

    const nudge = (dir) => {
        const min = input.min !== '' ? Number(input.min) : -Infinity;
        const max = input.max !== '' ? Number(input.max) : Infinity;
        const cur = Number(input.value);
        const base = Number.isNaN(cur) ? 0 : cur;
        const next = Math.min(max, Math.max(min, base + dir * step));
        if (next === base) return;
        input.value = String(next);
        input.dispatchEvent(new Event('input', { bubbles: true }));
    };

    minus.addEventListener('click', () => nudge(-1));
    plus.addEventListener('click', () => nudge(1));

    group.appendChild(minus);
    group.appendChild(input);
    group.appendChild(plus);
    return group;
}
