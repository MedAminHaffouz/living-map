/* Mirror of libs/lm_core/aging.py. Used by beacon_esp (broadcast state) and executor_stm (trust). */
#pragma once
#include <stdint.h>
typedef enum { LM_AGE_FRESH = 0, LM_AGE_AGING = 1, LM_AGE_STALE = 2, LM_AGE_SUSPECT = 3 } lm_age_state_e;
float   lm_age_confidence(float c0, uint32_t age_s, uint8_t event_type);
uint8_t lm_age_state(float c0, uint32_t age_s, uint8_t event_type, uint8_t flags);
