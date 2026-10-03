"""Framing: A5 5A | id | len | payload | crc16 (CCITT-FALSE over id|len|payload). Mirror of lm_link.c."""
SYNC = b"\xA5\x5A"

def crc16(data: bytes, crc: int = 0xFFFF) -> int:
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def encode(msg_id: int, payload: bytes) -> bytes:
    body = bytes([msg_id, len(payload)]) + payload
    c = crc16(body); return SYNC + body + bytes([c & 0xFF, c >> 8])

class Decoder:
    """Feed raw bytes (serial reads of any size); yields (msg_id, payload) for every valid frame."""
    def __init__(self): self.buf = bytearray()
    def feed(self, data: bytes):
        self.buf += data
        while True:
            i = self.buf.find(SYNC)
            if i < 0: self.buf = self.buf[-1:]; return
            del self.buf[:i]
            if len(self.buf) < 4: return
            n = self.buf[3]
            if len(self.buf) < 6 + n: return
            body, rx = bytes(self.buf[2:4 + n]), self.buf[4 + n] | self.buf[5 + n] << 8
            if crc16(body) == rx:
                del self.buf[:6 + n]; yield body[0], body[2:]
            else:
                del self.buf[:2]          # bad CRC: resync after this SYNC
