import sys, pathlib, math
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "libs"))
from lm_core.frame import chain_positions
from lm_core import aging

def test_chain_straight_line():
    # B1 is 4 m east of B0 -> direction from B1 to B0 is 180 deg
    p = chain_positions({1: (0, 180, 4.0), 2: (1, 180, 3.0)})
    assert math.isclose(p[2][0], 7.0, abs_tol=1e-9) and math.isclose(p[2][1], 0.0, abs_tol=1e-9)

def test_broken_chain_leaves_orphan():
    assert 5 not in chain_positions({1: (0, 180, 4.0), 5: (4, 0, 1.0)})

def test_aging_monotonic_and_suspect():
    states = [aging.state(0.9, t, 1) for t in (0, 60, 200)]
    assert states == sorted(states) and aging.state(0.9, 0, 1, flags=1) == 3
