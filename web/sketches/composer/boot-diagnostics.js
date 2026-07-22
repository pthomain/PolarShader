// SPDX-License-Identifier: GPL-3.0-or-later
//
// Boot diagnostics for the composer page. Loaded as a *classic* (non-module)
// script in <head> so it runs before FastLED's app.js and the composer
// modules — early enough to install global error handlers that catch a failing
// WASM/UI boot.
//
// Why this exists: the generated page hides its whole body behind
// `body { opacity: 0 }` and only reveals it from JS once boot progresses. If
// boot throws, nothing flips the opacity and the user sees a silent white
// screen with the error only in the devtools console. This script turns that
// failure mode into a visible, readable banner and guarantees the page is
// never left permanently invisible.
//
// It is intentionally dependency-free and defensive: it may run before
// <body> exists, and must never itself throw during a broken boot.

(function () {
  "use strict";

  // Grace period after which we force the page visible even if the normal
  // boot path never revealed it. Generous so a slow (but working) WASM boot
  // on a cold cache isn't prematurely flagged.
  var FAILSAFE_REVEAL_MS = 10000;

  var boot = {
    revealed: false,
    fatalShown: false,
    build: (typeof window !== "undefined" && window.__PS_BUILD__) || "dev",
  };
  window.__psBoot = boot;

  function reveal() {
    if (boot.revealed) return;
    boot.revealed = true;
    if (document.body) document.body.style.opacity = "1";
  }

  // Render (or update) a fixed banner at the top of the page. Created lazily so
  // this works even if an error fires before <body> is parsed.
  function showFatal(message) {
    reveal();
    var text = "Composer failed to load: " + message;
    // Prefer the console for the full detail; the banner is a summary.
    try {
      console.error("[boot-diagnostics] " + text);
    } catch (_) {}
    if (!document.body) {
      // Body not ready yet — flush once it is.
      document.addEventListener("DOMContentLoaded", function () {
        renderBanner(text);
      });
      return;
    }
    renderBanner(text);
  }

  function renderBanner(text) {
    boot.fatalShown = true;
    var el = document.getElementById("ps-boot-error");
    if (!el) {
      el = document.createElement("div");
      el.id = "ps-boot-error";
      el.setAttribute("role", "alert");
      el.style.cssText =
        "position:fixed;top:0;left:0;right:0;z-index:2147483647;" +
        "background:#7f1d1d;color:#fff;font:13px/1.5 ui-monospace,Menlo,Consolas,monospace;" +
        "padding:10px 14px;white-space:pre-wrap;word-break:break-word;" +
        "box-shadow:0 1px 6px rgba(0,0,0,.5);";
      // Insert first so it's visible above the (initially hidden) content.
      document.body.insertBefore(el, document.body.firstChild);
    }
    el.textContent =
      text +
      "\nOpen the browser console for the full stack trace. Build " +
      boot.build +
      ".";
  }

  window.addEventListener("error", function (e) {
    // Ignore ResourceLoadingError-less script errors we can't describe.
    var msg = (e && (e.message || (e.error && e.error.message))) || "unknown error";
    if (e && e.filename) msg += "  (" + e.filename + ":" + (e.lineno || 0) + ")";
    showFatal(msg);
  });

  window.addEventListener("unhandledrejection", function (e) {
    var reason = e && e.reason;
    var msg =
      (reason && (reason.message || reason.toString && reason.toString())) ||
      "unhandled promise rejection";
    showFatal(msg);
  });

  // Capability preflight. The generated app has a main-thread fallback when
  // cross-origin isolation (SharedArrayBuffer) is unavailable — e.g. on plain
  // GitHub Pages — so this is informational, not fatal. Surfacing it makes the
  // "why is it slow / no worker" question answerable from the console.
  function preflight() {
    var caps = {
      build: boot.build,
      crossOriginIsolated:
        typeof crossOriginIsolated !== "undefined" ? crossOriginIsolated : "n/a",
      sharedArrayBuffer: typeof SharedArrayBuffer !== "undefined",
      webAssembly: typeof WebAssembly !== "undefined",
      offscreenCanvas: typeof OffscreenCanvas !== "undefined",
    };
    try {
      console.info("[boot-diagnostics] composer capabilities", caps);
    } catch (_) {}
    if (!caps.webAssembly) {
      showFatal("this browser has no WebAssembly support");
    }
  }

  // Small, unobtrusive build stamp so you can confirm which build is actually
  // live (vs. a stale cached one) without opening devtools.
  function stampBuild() {
    if (!document.body) return;
    if (document.getElementById("ps-build-stamp")) return;
    var s = document.createElement("div");
    s.id = "ps-build-stamp";
    s.textContent = "build " + boot.build;
    s.title = "Composer build id (asset hash)";
    s.style.cssText =
      "position:fixed;bottom:4px;right:6px;z-index:2147483646;" +
      "font:10px/1 ui-monospace,Menlo,Consolas,monospace;color:#666;" +
      "opacity:.5;pointer-events:none;user-select:none;";
    document.body.appendChild(s);
  }

  function onReady() {
    preflight();
    stampBuild();
    // Fail-safe: never leave the page permanently invisible. If the normal
    // boot path never revealed the body within the grace period, force it
    // visible so the user at least sees the shell (and any error banner).
    setTimeout(function () {
      if (boot.revealed) return;
      var hidden =
        document.body &&
        getComputedStyle(document.body).opacity === "0";
      if (hidden) {
        try {
          console.warn(
            "[boot-diagnostics] boot did not reveal the page within " +
              FAILSAFE_REVEAL_MS +
              "ms; forcing visible."
          );
        } catch (_) {}
        reveal();
      }
    }, FAILSAFE_REVEAL_MS);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", onReady);
  } else {
    onReady();
  }
})();
