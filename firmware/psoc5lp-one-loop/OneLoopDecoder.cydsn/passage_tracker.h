/*
 * YurLaps one-loop decoder - multi-transponder passage aggregation.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PASSAGE_TRACKER_H
#define PASSAGE_TRACKER_H

#include <stdint.h>

#include "rch_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PASSAGE_TRACKER_SLOTS          (16u)
#define PASSAGE_END_GAP_QMS            (32u)    /* 8 ms */
#define PASSAGE_ID_HOLDOFF_QMS         (1200u)  /* 300 ms */

typedef struct
{
    uint32_t id;
    uint32_t timestamp_qms;
    uint16_t status10;
    uint8_t hits;
    uint8_t quality_percent;
    uint8_t telegram[RCH_TELEGRAM_BYTES];
} passage_event_t;

typedef void (*passage_emit_fn)(const passage_event_t *event, void *context);

typedef enum
{
    PASSAGE_SLOT_FREE = 0,
    PASSAGE_SLOT_COLLECTING = 1,
    PASSAGE_SLOT_HOLDOFF = 2
} passage_slot_state_t;

typedef struct
{
    passage_slot_state_t state;
    uint32_t id;
    uint32_t first_qms;
    uint32_t last_qms;
    uint16_t status10;
    uint16_t quality_sum;
    uint8_t hits;
    uint8_t telegram[RCH_TELEGRAM_BYTES];
} passage_slot_t;

typedef struct
{
    passage_slot_t slots[PASSAGE_TRACKER_SLOTS];
    passage_emit_fn emit;
    void *emit_context;
    uint32_t accepted_packets;
    uint32_t suppressed_packets;
    uint32_t emitted_passages;
    uint32_t dropped_packets;
} passage_tracker_t;

void passage_tracker_init(passage_tracker_t *tracker,
                          passage_emit_fn emit,
                          void *context);

void passage_tracker_push(passage_tracker_t *tracker,
                          const uint8_t telegram[RCH_TELEGRAM_BYTES],
                          const rch_packet_info_t *decoded,
                          uint8_t quality_percent,
                          uint32_t timestamp_qms);

void passage_tracker_poll(passage_tracker_t *tracker, uint32_t now_qms);

#ifdef __cplusplus
}
#endif

#endif /* PASSAGE_TRACKER_H */
