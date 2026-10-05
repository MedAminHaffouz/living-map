"""ONA Translate uses the shared lm_core frame math (no private copy)."""
import inspect, math
from modules.ona import ona
from modules.ona.ona import Translate

def test_translate_delegates_to_lm_core():
    p = {"heading_deg": 30.0, "entrance_lat": 36.843, "entrance_lon": 10.197}
    t = Translate("translate", p)
    assert t.to_wgs84(7.0, -2.5) == ona.w_to_wgs84(7.0, -2.5, 30.0, 36.843, 10.197)
    assert ona.w_to_wgs84.__module__ == "lm_core.frame"
    assert "111_320" not in inspect.getsource(Translate)
    lat, lon = t.to_wgs84(0.0, 0.0); assert math.isclose(lat, 36.843) and math.isclose(lon, 10.197)
