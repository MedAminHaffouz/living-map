"""Command Post (PC). Minimal HTTP JSON hub, stdlib only.
POST /situation  <- ONA (cellular)           GET /situation -> operator view
POST /mission    <- operator (approve plan)  GET /mission   -> ONA polls
Run: python cp_server.py --port 8080"""
import argparse, json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
STATE = {"situation": None, "mission": None}

class H(BaseHTTPRequestHandler):
    def _send(self, obj, code=200):
        b = json.dumps(obj).encode(); self.send_response(code); self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(b))); self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        k = self.path.strip("/"); self._send(STATE.get(k)) if k in STATE else self._send({"err": "404"}, 404)
    def do_POST(self):
        k = self.path.strip("/")
        if k not in STATE: return self._send({"err": "404"}, 404)
        STATE[k] = json.loads(self.rfile.read(int(self.headers["Content-Length"]))); self._send({"ok": True})
    def log_message(self, *a): pass

if __name__ == "__main__":
    a = argparse.ArgumentParser(); a.add_argument("--port", type=int, default=8080)
    ThreadingHTTPServer(("0.0.0.0", a.parse_args().port), H).serve_forever()
