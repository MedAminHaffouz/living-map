"""Area Knowledge: entrance GPS + heading + static notes (config/area.yaml)."""
import yaml
def load(path): return yaml.safe_load(open(path))
