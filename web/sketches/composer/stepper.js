// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// Reusable numeric stepper: wraps a numeric <input> (range or number) with
// manual [−] and [+] buttons that nudge the value by `step`, clamped to the
// input's own min/max. Clicking a button debounces the native 'input' event so
// rapid nudges update the renderer only after the user stops clicking.

export const STEPPER_INPUT_DEBOUNCE_MS = 500;

let activeSliderStepper = null;
let activeListenersInstalled = false;

function deactivateActiveSliderStepper({ blur = false } = {}) {
    if (!activeSliderStepper) return;
    const { group, input } = activeSliderStepper;
    group.classList.remove('active');
    activeSliderStepper = null;
    if (blur) input.blur();
}

function installActiveSliderListeners() {
    if (activeListenersInstalled) return;
    activeListenersInstalled = true;

    document.addEventListener('click', (event) => {
        if (!activeSliderStepper) return;
        if (activeSliderStepper.group.contains(event.target)) return;
        deactivateActiveSliderStepper();
    });

    document.addEventListener('focusin', (event) => {
        if (!activeSliderStepper) return;
        if (activeSliderStepper.group.contains(event.target)) return;
        deactivateActiveSliderStepper();
    });

    document.addEventListener('keydown', (event) => {
        if (!activeSliderStepper || event.defaultPrevented) return;
        if (event.key === 'Escape') {
            event.preventDefault();
            deactivateActiveSliderStepper({ blur: true });
            return;
        }
        if (event.key === 'ArrowLeft') {
            event.preventDefault();
            activeSliderStepper.nudge(-1);
        } else if (event.key === 'ArrowRight') {
            event.preventDefault();
            activeSliderStepper.nudge(1);
        }
    });
}

// Attach −/+ controls to `input` and return the wrapper element to insert in
// place of the bare input. Options: { step, debounceMs }.
export function attachStepper(input, { step = 1, debounceMs = STEPPER_INPUT_DEBOUNCE_MS } = {}) {
    const group = document.createElement('div');
    group.className = 'stepper';
    let debounceTimer = null;
    const isSlider = input.type === 'range';

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

    const dispatchInput = () => {
        debounceTimer = null;
        // If the input was detached by a re-render (e.g. switching signal type)
        // while the edit was pending, dropping the dispatch avoids mutating the
        // now-stale model.
        if (!input.isConnected) return;
        input.dispatchEvent(new Event('input', { bubbles: true }));
    };

    const scheduleInput = () => {
        if (debounceTimer !== null) clearTimeout(debounceTimer);
        debounceTimer = setTimeout(dispatchInput, debounceMs);
    };

    const nudge = (dir) => {
        const min = input.min !== '' ? Number(input.min) : -Infinity;
        const max = input.max !== '' ? Number(input.max) : Infinity;
        const cur = Number(input.value);
        const base = Number.isNaN(cur) ? 0 : cur;
        const next = Math.min(max, Math.max(min, base + dir * step));
        if (next === base) return;
        input.value = String(next);
        input.dispatchEvent(new Event('stepper-preview', { bubbles: true }));
        scheduleInput();
    };

    const activate = () => {
        if (!isSlider) return;
        installActiveSliderListeners();
        if (activeSliderStepper?.group === group) return;
        deactivateActiveSliderStepper();
        group.classList.add('active');
        activeSliderStepper = { group, input, nudge };
    };

    input.addEventListener('input', () => {
        if (debounceTimer === null) return;
        clearTimeout(debounceTimer);
        debounceTimer = null;
    });

    if (isSlider) {
        group.addEventListener('click', activate);
        group.addEventListener('focusin', activate);
    }

    minus.addEventListener('click', () => {
        activate();
        nudge(-1);
    });
    plus.addEventListener('click', () => {
        activate();
        nudge(1);
    });

    group.appendChild(minus);
    group.appendChild(input);
    group.appendChild(plus);
    return group;
}
