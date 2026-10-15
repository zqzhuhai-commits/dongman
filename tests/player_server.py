"""Check HLS decoding and required request headers on playlist and segments."""
import functools
import http.server
import os
import subprocess
import sys
import threading


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        expected = f"http://127.0.0.1:{self.server.server_port}/watch"
        if (self.headers.get("Referer") != expected or
                "Chrome/120.0.0.0" not in self.headers.get("User-Agent", "")):
            self.send_error(403)
            return
        super().do_GET()

    def log_message(self, *_):
        pass


handler = functools.partial(Handler, directory=sys.argv[2])
with http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler) as server:
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = f"http://127.0.0.1:{server.server_port}"
    env = dict(os.environ, NO_PROXY="127.0.0.1", no_proxy="127.0.0.1",
               HTTP_PROXY="", http_proxy="", ALL_PROXY="", all_proxy="")
    try:
        subprocess.run([sys.argv[1], base + "/clip.m3u8", base + "/watch"],
                       check=True, env=env, timeout=30)
    finally:
        server.shutdown()
        thread.join()
