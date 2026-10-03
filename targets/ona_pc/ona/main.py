"""ONA loop: read beacons -> situation to CP (HTTP over cellular) -> poll mission -> brief Executor.
Run: python -m ona.main --area config/area.yaml   (from targets/ona_pc)"""
import argparse, json, time, urllib.request
import serial
from .area_knowledge import load
from .beacons_reader import BeaconsReader
from .situation_debrief import build
from .mission_debrief import frames

def http(url, data=None):
    req = urllib.request.Request(url, data=json.dumps(data).encode() if data is not None else None,
                                 headers={"Content-Type": "application/json"}, method="POST" if data is not None else "GET")
    with urllib.request.urlopen(req, timeout=3) as r: return json.loads(r.read() or b"null")

def main():
    a = argparse.ArgumentParser(); a.add_argument("--area", default="config/area.yaml"); args = a.parse_args()
    area = load(args.area); radio = serial.Serial(area["radio_port"], 921600, timeout=0)
    rd, sent = BeaconsReader(radio), set()
    while True:
        rd.poll()
        try:
            http(area["cp_url"] + "/situation", build(rd.beacons, area))
            m = http(area["cp_url"] + "/mission")
            if m and m["mission_id"] not in sent:
                for f in frames(m): radio.write(f); time.sleep(0.02)
                sent.add(m["mission_id"])
        except OSError as e: print("CP link down:", e)   # TODO: sat backup path
        time.sleep(1.0)

if __name__ == "__main__": main()
