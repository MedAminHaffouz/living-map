/* Generic sensor node (one instance per sensor): sample -> median5 -> hysteresis -> debounce(k of n).
   Per-sensor differences live in sensor_cfg_t, not in code. Output = lm_sensor_det_t, sent to the Pi. */
#pragma once
#include <stdint.h>
#include "lm_msgs.h"

typedef float (*sensor_read_fn)(void);          /* returns physical value: °C, Rs/R0, ppm */

typedef struct {
    uint8_t  type;            /* lm_event_type_t */
    sensor_read_fn read;
    float    th_on, th_off;   /* hysteresis */
    uint8_t  k, n;            /* debounce: k of last n samples above th_on (n <= 8) */
    uint32_t warmup_ms;       /* MQ heaters; conf = 0 until elapsed */
    uint16_t period_ms;
} sensor_cfg_t;

typedef struct {
    const sensor_cfg_t *cfg;
    float    win[5]; uint8_t wi, wfill;
    uint8_t  hist, hn;        /* bit history for debounce */
    uint8_t  detected;
    uint32_t t_start, t_last;
} sensor_node_t;

void sensor_node_init(sensor_node_t *s, const sensor_cfg_t *cfg, uint32_t now_ms);
/* call every loop; returns 1 and fills *out when a new sample is ready */
int  sensor_node_tick(sensor_node_t *s, uint32_t now_ms, lm_sensor_det_t *out);
