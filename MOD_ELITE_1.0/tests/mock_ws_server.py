#!/usr/bin/env python3
"""Servidor WebSocket de teste, independente do cliente C++ (usa hashlib/base64 do Python).
Eco binário; 'F' => mensagem fragmentada (A..E + F..J) com ping no meio; 'C' => close."""
import base64, hashlib, socket, struct, sys
GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
def recvn(c, n):
    b = b""
    while len(b) < n:
        d = c.recv(n - len(b))
        if not d: raise EOFError
        b += d
    return b
def frame(op, payload, fin=True):
    h = bytes([(0x80 if fin else 0) | op])
    n = len(payload)
    if n < 126: h += bytes([n])
    elif n < 65536: h += bytes([126]) + struct.pack(">H", n)
    else: h += bytes([127]) + struct.pack(">Q", n)
    return h + payload
srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", int(sys.argv[1]))); srv.listen(1); print("ready", flush=True)
c, _ = srv.accept()
req = b""
while b"\r\n\r\n" not in req: req += c.recv(4096)
hdrs = {l.split(b":", 1)[0].strip().lower(): l.split(b":", 1)[1].strip() for l in req.split(b"\r\n")[1:] if b":" in l}
assert req.startswith(b"GET /slither "), req[:40]
assert hdrs[b"origin"] == b"https://slither.com"
acc = base64.b64encode(hashlib.sha1(hdrs[b"sec-websocket-key"] + GUID.encode()).digest())
c.sendall(b"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + acc + b"\r\n\r\n")
try:
    while True:
        b0, b1 = recvn(c, 2); op = b0 & 15; n = b1 & 127
        assert b1 & 0x80, "cliente deve mascarar"
        if n == 126: n = struct.unpack(">H", recvn(c, 2))[0]
        elif n == 127: n = struct.unpack(">Q", recvn(c, 8))[0]
        mk = recvn(c, 4); p = bytearray(recvn(c, n))
        for i in range(n): p[i] ^= mk[i & 3]
        if op == 8: break
        if len(p) == 1 and p[0] == ord("F"):
            c.sendall(frame(2, b"ABCDE", fin=False)); c.sendall(frame(9, b"hi")); c.sendall(frame(0, b"FGHIJ", fin=True))
        elif len(p) == 1 and p[0] == ord("C"):
            c.sendall(frame(8, struct.pack(">H", 1000))); break
        else: c.sendall(frame(2, bytes(p)))
except EOFError: pass
c.close()
