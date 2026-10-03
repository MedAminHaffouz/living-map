"""Beacon aging. Mirror of lm_aging.c — keep both in sync (tests/test_aging.py checks thresholds).
c(age) = c0 * exp(-age / tau[type]); state by thresholds; SUSPECT only via flag (contradiction)."""
import math
TAU_S = {1: 120.0, 2: 300.0, 3: 900.0, 4: 1e9, 5: 180.0}   # FIRE, GAS, PERSON, JUNCTION, SMOKE
FRESH_C, STALE_C = 0.7, 0.4
FLAG_SUSPECT = 0x01

def confidence(c0: float, age_s: float, event_type: int) -> float:
    return c0 * math.exp(-age_s / TAU_S.get(event_type, 300.0))

def state(c0: float, age_s: float, event_type: int, flags: int = 0) -> int:
    if flags & FLAG_SUSPECT: return 3
    c = confidence(c0, age_s, event_type)
    return 0 if c >= FRESH_C else 1 if c >= STALE_C else 2
