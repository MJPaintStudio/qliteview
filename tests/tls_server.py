"""Loopback-only HTTPS fixture. Its test certificate must never be trusted."""
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import ssl


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        body = b"<!doctype html><title>Untrusted certificate accepted</title>"
        self.send_response(200)
        self.send_header("Content-Type", "text/html")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
fixtures = Path(__file__).parent / "fixtures"
context.load_cert_chain(fixtures / "untrusted-cert.pem", fixtures / "untrusted-key.pem")
server = HTTPServer(("127.0.0.1", 0), Handler)
server.socket = context.wrap_socket(server.socket, server_side=True)
print(server.server_port, flush=True)
server.serve_forever()
