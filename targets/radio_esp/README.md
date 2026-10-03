# radio_esp — one bridge firmware, three boards

`host (UART/USB) <-> ESP-NOW`, same `lm_link` frames both sides. Beacon broadcasts are forwarded to the host as `BeaconObs` with RSSI.
Envs: `writer` (Pi USB), `executor` (STM UART), `ona` (ONA PC USB). `pio run -e writer -t upload`.
