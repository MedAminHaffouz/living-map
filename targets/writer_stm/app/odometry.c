#include "odometry.h"
#include <math.h>
void odom_init(odom_t *o, float tpm, float wb) { *o = (odom_t){ .ticks_per_m = tpm, .wheel_base_m = wb }; }
void odom_reset(odom_t *o) { o->x = o->y = o->th = 0.f; }
void odom_update(odom_t *o, int32_t l, int32_t r, uint32_t now, lm_wheel_odom_t *out) {
    float dl = (float)(l - o->last_l) / o->ticks_per_m, dr = (float)(r - o->last_r) / o->ticks_per_m;
    float dt = (now - o->t_last) / 1000.f; o->last_l = l; o->last_r = r; o->t_last = now;
    float ds = 0.5f * (dl + dr), dth = (dr - dl) / o->wheel_base_m;
    o->x += ds * cosf(o->th + 0.5f * dth); o->y += ds * sinf(o->th + 0.5f * dth); o->th += dth;
    if (dt > 0) { o->v = ds / dt; o->w = dth / dt; }
    *out = (lm_wheel_odom_t){ o->x, o->y, o->th, o->v, o->w, now };
}
