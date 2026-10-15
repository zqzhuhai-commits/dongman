"""Run deterministic HTTP regression tests without any external website."""
import http.server
import os
import subprocess
import sys
import threading


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path in ("/redirect", "/bad-redirect"):
            self.send_response(302)
            self.send_header("Location", "/ok" if self.path == "/redirect"
                             else "file:///etc/hostname")
            self.end_headers()
            return
        status = 200 if self.path in ("/ok", "/large") else 404
        self.send_response(status)
        self.end_headers()
        self.wfile.write(b"x" * 128 if self.path == "/large"
                         else b"hello\0world")

    def log_message(self, *_):
        pass


with http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler) as server:
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        env = dict(os.environ, NO_PROXY="127.0.0.1", no_proxy="127.0.0.1")
        subprocess.run([sys.argv[1], f"http://127.0.0.1:{server.server_port}"],
                       check=True, env=env, timeout=30)
    finally:
        server.shutdown()
        thread.join()
