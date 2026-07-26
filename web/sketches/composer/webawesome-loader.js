// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// Progressive Web Awesome bootstrap for the composer panel. The panel renders
// native controls first, then rebuilds into Web Awesome controls once the
// component is registered. If the CDN is unavailable, native controls remain.

(function loadComposerWebAwesome() {
    const version = '3.10.0';
    const base = `https://cdn.jsdelivr.net/npm/@awesome.me/webawesome@${version}/dist-cdn`;
    const root = document.documentElement;
    const state = {
        version,
        base,
        ready: false,
        error: null,
    };

    window.polarWebAwesome = state;
    root.classList.add('wa-theme-default', 'wa-palette-default', 'wa-brand-blue');

    function dispatch(name, detail) {
        window.dispatchEvent(new CustomEvent(name, { detail }));
    }

    function loadStyle(id, href) {
        if (document.getElementById(id)) return Promise.resolve();
        return new Promise((resolve, reject) => {
            const link = document.createElement('link');
            link.id = id;
            link.rel = 'stylesheet';
            link.href = href;
            link.onload = () => resolve();
            link.onerror = () => reject(new Error(`failed to load ${href}`));
            document.head.appendChild(link);
        });
    }

    function markReady() {
        state.ready = true;
        root.classList.add('webawesome-ready');
        root.classList.remove('webawesome-unavailable');
        dispatch('polar-webawesome-ready', state);
    }

    function markUnavailable(error) {
        state.error = error instanceof Error ? error.message : String(error);
        root.classList.add('webawesome-unavailable');
        dispatch('polar-webawesome-unavailable', state);
        console.warn('[composer] Web Awesome unavailable; using native controls.', error);
    }

    if (!('customElements' in window)) {
        markUnavailable(new Error('custom elements are not supported'));
        return;
    }

    Promise.all([
        loadStyle('polar-webawesome-core', `${base}/styles/webawesome.css`),
        loadStyle('polar-webawesome-theme', `${base}/styles/themes/default.css`),
        import(`${base}/components/button/button.js`),
    ])
        .then(() => customElements.whenDefined('wa-button'))
        .then(markReady)
        .catch(markUnavailable);
}());
