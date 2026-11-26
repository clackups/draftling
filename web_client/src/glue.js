/*
 * Web client: helpers the page (web/client.js) calls to put data into
 * the program's memory. Linked into draftling.js (--post-js), so its
 * memory accesses follow the WebAssembly memory as it grows (the page's
 * own code would keep a stale view of it).
 */
Module.webCopyIn = function (bytes) {
    var p = _malloc(bytes.length || 1);
    if (p) HEAPU8.set(bytes, p);
    return p;
};
