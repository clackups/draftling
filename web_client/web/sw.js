/*
 * Draftling web client: service worker.
 *
 * 1. Cross-origin isolation. The program's threads need
 *    SharedArrayBuffer, which a page only gets when it is served with
 *    Cross-Origin-Opener-Policy: same-origin and
 *    Cross-Origin-Embedder-Policy: require-corp. A server that sends
 *    them needs nothing more; for one that cannot (GitHub Pages, ...)
 *    this worker adds them to every response of the site.
 * 2. Offline use: the files of this build (@BUILD@) are kept in a
 *    cache, so Draftling starts without a network connection. The page
 *    itself comes from the network first, so a new build is picked up
 *    as soon as there is one.
 *
 * Requests to other sites (Git sync, Draftbox) are not touched.
 */
"use strict";

const BUILD = "@BUILD@";
const CACHE = "draftling-" + BUILD;
const FILES = [@FILES@];

function isolated(resp) {
  if (!resp || resp.status === 0 || resp.type === "opaque") return resp;
  const h = new Headers(resp.headers);
  h.set("Cross-Origin-Opener-Policy", "same-origin");
  h.set("Cross-Origin-Embedder-Policy", "require-corp");
  return new Response(resp.body, { status: resp.status, statusText: resp.statusText, headers: h });
}

self.addEventListener("install", (e) => {
  e.waitUntil((async () => {
    const c = await caches.open(CACHE);
    await c.addAll(FILES.filter((f) => f !== "index.html" && f !== "sw.js")
      .map((f) => f + "?v=" + BUILD).concat(["./"]));
    await self.skipWaiting();
  })());
});

self.addEventListener("activate", (e) => {
  e.waitUntil((async () => {
    for (const k of await caches.keys()) {
      if (k.startsWith("draftling-") && k !== CACHE) await caches.delete(k);
    }
    await self.clients.claim();
  })());
});

self.addEventListener("fetch", (e) => {
  const req = e.request;
  const url = new URL(req.url);
  if (req.method !== "GET" || url.origin !== self.location.origin) return;
  if (req.mode === "navigate") {
    e.respondWith((async () => {
      try {
        const resp = await fetch(req);
        if (resp.ok) (await caches.open(CACHE)).put("./", resp.clone());
        return isolated(resp);
      } catch (err) {
        const cached = await caches.match("./");
        if (cached) return isolated(cached);
        throw err;
      }
    })());
    return;
  }
  e.respondWith((async () => {
    const cached = await caches.match(req);
    return isolated(cached || await fetch(req));
  })());
});
