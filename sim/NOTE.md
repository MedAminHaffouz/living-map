`sim/` is the Phase 1 simulation (previous scaffold, unchanged). It runs every agent in one process with an
in-memory bus and enforces the spec's communication rules at load. Next step: replace its local aging/translation
with `libs/lm_core` so sim and targets share one implementation.
