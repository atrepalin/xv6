#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler, HTTPServer
import urllib.parse

HOST = '0.0.0.0'
PORT = 8080

class SimpleHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"Hello from Python server!\n")
        elif self.path == '/test':
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"This is the /test endpoint!\n")
        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found\n")

    def do_POST(self):
        if self.path.startswith('/greet/'):
            name = urllib.parse.unquote(self.path[len('/greet/'):])
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(f"Hello, {name.capitalize()}!\n".encode())
        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found\n")

    def log_message(self, format, *args):
        return

server = HTTPServer((HOST, PORT), SimpleHandler)
print(f"Serving on {HOST}:{PORT}")
server.serve_forever()
