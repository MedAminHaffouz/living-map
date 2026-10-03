# libs/ — shared code, linked into targets

- `lm_core/` (Python, no rclpy, no I/O): `link.py` framing, `aging.py`, `frame.py` (beacon chain → W → WGS84). Imported by `targets/writer_pi` ROS nodes, `targets/ona_pc`, `targets/cp_pc`, `sim/`.
- `lm_embedded/` (C99, no HAL): `lm_link.c` framing, `lm_aging.c`. Compiled into every firmware target.

Python and C versions of the same logic are cross-checked in `tests/`. Change one → change both.
