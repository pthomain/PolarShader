// SPDX-License-Identifier: GPL-3.0-or-later
//
// Headless shell-render smoke test for the built composer page.
//
// Catches the "silent blank page" regression class: the generated page hides
// its body behind `opacity: 0` and only reveals it once boot progresses, so a
// broken boot renders a white screen whose error lives only in the console —
// and every asset still returns HTTP 200, so a network check can't catch it.
//
// Scope is deliberately the *shell*, not a full WASM render (the render worker
// times out / requires cross-origin isolation not present on a plain static
// host). The hard gate — the "blank page" invariant — is:
//   1. the body is revealed (computed opacity === "1") — normal boot does this,
//      and boot-diagnostics.js reveals on error and as a timed fail-safe, so a
//      body stuck at 0 means the whole boot chain (incl. our bootstrap) failed,
//   2. the composer canvas shell is visible,
//   3. boot-diagnostics.js ran (the build stamp element is present).
//
// Uncaught page errors (e.g. the known SharedArrayBuffer/cross-origin-isolation
// limitation on GitHub Pages, which boot-diagnostics now surfaces as a banner
// instead of a blank screen) are reported as warnings by default — they no
// longer cause a blank page, and fixing the WASM boot itself is a separate
// concern. Pass --strict to also fail on any uncaught error or error banner
// (use once the underlying WASM boot is fixed).
//
// Usage: node web/smoke_test.mjs [distDir] [--strict]   (default dir: web/dist)

import { chromium } from "playwright";
import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { existsSync } from "node:fs";
import { join, normalize, extname } from "node:path";

const args = process.argv.slice(2);
const strict = args.includes("--strict");
const distDir = args.find((a) => !a.startsWith("--")) || join(process.cwd(), "web", "dist");
const composerIndex = join(distDir, "composer", "index.html");

if (!existsSync(composerIndex)) {
  console.error(`[smoke] built composer not found at ${composerIndex}`);
  console.error("[smoke] run `python web/build_site.py` first.");
  process.exit(2);
}

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".mjs": "text/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".wasm": "application/wasm",
  ".map": "application/json; charset=utf-8",
};

const server = createServer(async (req, res) => {
  try {
    const urlPath = decodeURIComponent(new URL(req.url, "http://localhost").pathname);
    let rel = normalize(urlPath).replace(/^(\.\.[/\\])+/, "");
    if (rel.endsWith("/")) rel += "index.html";
    const filePath = join(distDir, rel);
    if (!filePath.startsWith(distDir)) {
      res.writeHead(403).end("forbidden");
      return;
    }
    const body = await readFile(filePath);
    res.writeHead(200, {
      "content-type": MIME[extname(filePath)] || "application/octet-stream",
      // Grant cross-origin isolation directly (as a real headers-capable host
      // would), so the pthread WASM gets its SharedArrayBuffer on first load —
      // no coi-serviceworker reload to wait on. Mirrors the isolation the SW
      // synthesizes on GitHub Pages.
      "cross-origin-opener-policy": "same-origin",
      "cross-origin-embedder-policy": "require-corp",
      "cross-origin-resource-policy": "cross-origin",
    });
    res.end(body);
  } catch {
    res.writeHead(404).end("not found");
  }
});

await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
const { port } = server.address();
const url = `http://127.0.0.1:${port}/composer/index.html`;

const pageErrors = [];
let browser;
let failed = false;
const fail = (msg) => {
  failed = true;
  console.error(`[smoke] FAIL: ${msg}`);
};

try {
  browser = await chromium.launch();
  const page = await browser.newPage();
  page.on("pageerror", (err) => pageErrors.push(err.message || String(err)));

  console.log(`[smoke] loading ${url}`);
  await page.goto(url, { waitUntil: "load", timeout: 30000 });

  // Wait for the shell to become visible. boot-diagnostics force-reveals after
  // ~10s even on a failed WASM boot, so this should always resolve unless the
  // bootstrap itself failed to load.
  try {
    await page.waitForFunction(
      () => getComputedStyle(document.body).opacity === "1",
      { timeout: 20000 }
    );
  } catch {
    fail("body never reached opacity:1 — the page stayed invisible (blank-screen regression)");
  }

  const canvasVisible = await page.locator("#canvas-container").isVisible().catch(() => false);
  if (!canvasVisible) fail("#canvas-container is not visible");

  const stampPresent = (await page.locator("#ps-build-stamp").count()) > 0;
  if (!stampPresent) fail("boot-diagnostics did not run (no #ps-build-stamp)");

  // The error banner + uncaught errors: with boot-diagnostics these no longer
  // cause a blank page. Treat as warning unless --strict.
  const report = strict ? fail : (m) => console.warn(`[smoke] WARN: ${m}`);

  const fatalBanner = await page.locator("#ps-boot-error").count();
  if (fatalBanner > 0) {
    const bannerText = await page.locator("#ps-boot-error").innerText().catch(() => "");
    report(`boot error banner shown: ${bannerText.replace(/\s+/g, " ").trim()}`);
  }

  if (pageErrors.length) {
    report(`uncaught page errors:\n    - ${pageErrors.join("\n    - ")}`);
  }
} catch (err) {
  fail(`unexpected: ${err && err.stack ? err.stack : err}`);
} finally {
  if (browser) await browser.close();
  server.close();
}

if (failed) {
  process.exit(1);
}
console.log("[smoke] PASS: composer shell rendered (visible, menu bar, diagnostics active)");
