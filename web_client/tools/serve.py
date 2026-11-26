#!/usr/bin/env python3
"""Serve the web client's dist/ folder for testing.

Usage: serve.py <dir> [port]

Sends the headers the site needs (README.md, "Deploying"): the
cross-origin isolation pair on every response, application/wasm for
.wasm, and no caching for index.html and sw.js.
"""
import functools
import http.server
import sys


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
        ".webmanifest": "application/manifest+json",
        ".svg": "image/svg+xml",
    }

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        path = self.path.split("?")[0]
        if path in ("/", "/index.html", "/sw.js"):
            self.send_header("Cache-Control", "no-cache")
        super().end_headers()


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8001
    handler = functools.partial(Handler, directory=sys.argv[1])
    with http.server.ThreadingHTTPServer(("", port), handler) as httpd:
        print("Serving %s on http://localhost:%d/" % (sys.argv[1], port))
        httpd.serve_forever()


if __name__ == "__main__":
    main()
