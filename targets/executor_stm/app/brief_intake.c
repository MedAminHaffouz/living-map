#include "executor.h"
#include <string.h>
void brief_on_header(brief_t *b, const lm_brief_header_t *h) { memset(b, 0, sizeof *b); b->mission_id = h->mission_id; b->n = h->n_steps > EX_MAX_STEPS ? EX_MAX_STEPS : h->n_steps; }
void brief_on_step(brief_t *b, const lm_brief_step_t *s) {
    if (s->mission_id != b->mission_id || s->idx >= b->n) return;
    if (b->step[s->idx].mission_id == 0) b->got++;
    b->step[s->idx] = *s;
}
int brief_complete(const brief_t *b) { return b->n && b->got == b->n; }
