/* Executor (STM32F103 + Ra-02 on SPI, no ROS). Boxes: Brief Intake, Beacon Handler, Actions Taker,
   Fire Action Node / First Aid Node, Beacon Update. Same superloop pattern as writer_stm.
   One firmware, two robots: -DEXECUTOR_TYPE=EX_FIRE (EX-F, extinguisher) or EX_MED (EX-M, first-aid kits). */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "lm_msgs.h"
#include "lm_lora_sx127x.h"

#define EX_FIRE 1
#define EX_MED  2
#if !defined(EXECUTOR_TYPE) || (EXECUTOR_TYPE != EX_FIRE && EXECUTOR_TYPE != EX_MED)
#error "build with -DEXECUTOR_TYPE=EX_FIRE or -DEXECUTOR_TYPE=EX_MED"
#endif

/* ---- Brief Intake: BriefHeader + N BriefStep from ONA (via radio, at the entrance) ---- */
#define EX_MAX_STEPS 16
typedef struct { uint16_t mission_id; uint8_t n, got; lm_brief_step_t step[EX_MAX_STEPS]; } brief_t;
void brief_on_header(brief_t *b, const lm_brief_header_t *h);
void brief_on_step(brief_t *b, const lm_brief_step_t *s);
int  brief_complete(const brief_t *b);

/* ---- Beacon Handler: table of heard beacons + RSSI + trust state (lm_aging) ---- */
#define EX_MAX_BEACONS 32
typedef struct { lm_beacon_payload_t p; int8_t rssi; uint32_t heard_ms; uint8_t state; uint8_t valid; } beacon_entry_t;
typedef struct { beacon_entry_t b[EX_MAX_BEACONS]; } beacon_table_t;
void beacon_on_obs(beacon_table_t *t, const lm_beacon_obs_t *o, uint32_t now_ms);
const beacon_entry_t *beacon_get(const beacon_table_t *t, uint8_t id);

/* ---- Actions Taker: walks the brief step by step ---- */
typedef enum { EX_IDLE, EX_NAVIGATE, EX_VERIFY, EX_ACT, EX_REPORT, EX_DONE } ex_state_t;
typedef struct { ex_state_t st; uint8_t idx; uint8_t tries; uint32_t t_state; } actions_t;
void actions_tick(actions_t *a, brief_t *b, beacon_table_t *t, uint32_t now_ms);

/* ---- Actuator nodes: same contract, one per capability ---- */
typedef enum { ACT_RUNNING, ACT_OK, ACT_FAIL } act_status_t;
typedef struct {
    uint8_t      action;                                   /* lm_action_t */
    int          (*precond)(const beacon_entry_t *);       /* capacity left? verified? */
    void         (*start)(const beacon_entry_t *, uint32_t now_ms);
    act_status_t (*tick)(uint32_t now_ms);                 /* hard timeout inside */
    int          (*verify)(void);                          /* own sensors confirm effect */
} actuator_node_t;
extern const actuator_node_t FIRE_ACTION_NODE, FIRST_AID_NODE;   /* only the one matching EXECUTOR_TYPE is built */
const actuator_node_t *ex_actuator(void);                          /* this robot's one capability */

/* ---- Beacon Update: "with confirm" -> overwrite beacon (version+1), report to ONA ---- */
void beacon_update_send(const beacon_entry_t *e, uint8_t new_flags, uint8_t action, uint8_t result, uint16_t mission_id, uint32_t now_ms);
/* ActionReport only, beacon left untouched (step not attempted) */
void action_report_send(uint8_t beacon_id, uint8_t action, uint8_t result, uint16_t mission_id, uint32_t now_ms);

/* ---- Radio: everything goes out through an 8-entry queue, sent only in the TDMA reader window ---- */
#define EX_TXQ 8
int ex_tx(uint8_t id, const void *p, uint8_t n);   /* 0 = queued, -1 = queue full (dropped) */

/* ---- Navigation between beacons (RSSI homing + local avoidance) ---- */
typedef enum { NAV_RUNNING, NAV_ARRIVED, NAV_LOST } nav_status_t;
nav_status_t nav_to_beacon(const beacon_table_t *t, uint8_t target_id, uint32_t now_ms);

/* ---- board glue (implement in board.c) ---- */
uint32_t board_millis(void);
const lm_lora_board_t *board_lora(void);          /* Ra-02: spi_xfer, nss, reset, dio0, millis */
void     board_pump(int on);                      /* EX-F only */
void     board_kit_servo_release(void);            /* EX-M only */
int      board_kit_bay_empty(void);                /* EX-M only */
float    board_read_temp_c(void);
void     board_drive(float v, float w);
