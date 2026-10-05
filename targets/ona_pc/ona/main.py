"""ONA loop: read beacons from the LoRa gateway -> situation to CP (cellular, satellite failover) -> poll mission
-> brief frames to the gateway (it queues them and sends in the LoRa reader window).
Run: python -m ona.main --area config/area.yaml   (from targets/ona_pc)"""
import argparse, time
import serial
from .area_knowledge import load
from .beacons_reader import BeaconsReader
from .situation_debrief import build
from .mission_debrief import frames
from .cp_link import Failover, HttpCellular, SatelliteSBD

GATEWAY_BAUD = 921600

def cp_links(area: dict) -> Failover:
    return Failover([HttpCellular(area["cp_url"]), SatelliteSBD(area["sat_port"], area["entrance"])])

def main():
    a = argparse.ArgumentParser(); a.add_argument("--area", default="config/area.yaml"); args = a.parse_args()
    area = load(args.area)
    gateway = serial.Serial(area["gateway_port"], GATEWAY_BAUD, timeout=0)
    rd, cp, sent = BeaconsReader(gateway), cp_links(area), set()
    while True:
        rd.poll()
        try:
            cp.send_situation(build(rd.beacons, area))
            m = cp.fetch_mission()
            if m and m["mission_id"] not in sent:
                gateway.write(b"".join(frames(m)))      # gateway queue = 16 frames: briefs up to 15 steps
                sent.add(m["mission_id"])
        except OSError as e: print("CP link down:", e)
        time.sleep(1.0)                                 # TODO: slower situation rate while on satellite (cost)

if __name__ == "__main__": main()
