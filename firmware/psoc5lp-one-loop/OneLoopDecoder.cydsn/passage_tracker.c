/*
 * YurLaps one-loop decoder - multi-transponder passage aggregation.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "passage_tracker.h"

#include <string.h>

static uint32_t elapsed_qms(uint32_t now, uint32_t then)
{
    /* Unsigned subtraction intentionally supports the 32-bit timer rollover. */
    return now - then;
}

static void emit_slot(passage_tracker_t *tracker, passage_slot_t *slot)
{
    passage_event_t event;

    event.id = slot->id;
    event.timestamp_qms = slot->first_qms;
    event.status10 = slot->status10;
    event.hits = slot->hits;
    event.quality_percent = (slot->hits == 0u) ? 0u :
        (uint8_t)(slot->quality_sum / slot->hits);
    memcpy(event.telegram, slot->telegram, RCH_TELEGRAM_BYTES);

    if (tracker->emit != NULL)
    {
        tracker->emit(&event, tracker->emit_context);
    }
    ++tracker->emitted_passages;
    slot->state = PASSAGE_SLOT_HOLDOFF;
}

void passage_tracker_init(passage_tracker_t *tracker,
                          passage_emit_fn emit,
                          void *context)
{
    memset(tracker, 0, sizeof(*tracker));
    tracker->emit = emit;
    tracker->emit_context = context;
}

void passage_tracker_poll(passage_tracker_t *tracker, uint32_t now_qms)
{
    uint8_t index;

    for (index = 0u; index < PASSAGE_TRACKER_SLOTS; ++index)
    {
        passage_slot_t *slot = &tracker->slots[index];
        if ((slot->state == PASSAGE_SLOT_COLLECTING) &&
            (elapsed_qms(now_qms, slot->last_qms) >= PASSAGE_END_GAP_QMS))
        {
            emit_slot(tracker, slot);
        }
        if ((slot->state == PASSAGE_SLOT_HOLDOFF) &&
            (elapsed_qms(now_qms, slot->first_qms) >=
             PASSAGE_ID_HOLDOFF_QMS))
        {
            memset(slot, 0, sizeof(*slot));
        }
    }
}

void passage_tracker_push(passage_tracker_t *tracker,
                          const uint8_t telegram[RCH_TELEGRAM_BYTES],
                          const rch_packet_info_t *decoded,
                          uint8_t quality_percent,
                          uint32_t timestamp_qms)
{
    passage_slot_t *free_slot = NULL;
    uint8_t index;

    if ((tracker == NULL) || (telegram == NULL) || (decoded == NULL) ||
        (decoded->coding_ok == 0u) || (decoded->preamble_ok == 0u))
    {
        return;
    }
    if (decoded->display_id_ok == 0u)
    {
        /* Never silently truncate a 30-bit value into Cano's six ID digits. */
        ++tracker->dropped_packets;
        return;
    }

    passage_tracker_poll(tracker, timestamp_qms);

    for (index = 0u; index < PASSAGE_TRACKER_SLOTS; ++index)
    {
        passage_slot_t *slot = &tracker->slots[index];

        if (slot->state == PASSAGE_SLOT_FREE)
        {
            if (free_slot == NULL)
            {
                free_slot = slot;
            }
            continue;
        }
        if (slot->id != decoded->id30)
        {
            continue;
        }

        if (slot->state == PASSAGE_SLOT_HOLDOFF)
        {
            ++tracker->suppressed_packets;
            return;
        }

        slot->last_qms = timestamp_qms;
        slot->status10 = decoded->status10;
        if (slot->hits != 0xFFu)
        {
            ++slot->hits;
            slot->quality_sum = (uint16_t)(slot->quality_sum +
                                           quality_percent);
        }
        memcpy(slot->telegram, telegram, RCH_TELEGRAM_BYTES);
        ++tracker->accepted_packets;
        return;
    }

    if (free_slot == NULL)
    {
        ++tracker->dropped_packets;
        return;
    }

    memset(free_slot, 0, sizeof(*free_slot));
    free_slot->state = PASSAGE_SLOT_COLLECTING;
    free_slot->id = decoded->id30;
    free_slot->first_qms = timestamp_qms;
    free_slot->last_qms = timestamp_qms;
    free_slot->status10 = decoded->status10;
    free_slot->quality_sum = quality_percent;
    free_slot->hits = 1u;
    memcpy(free_slot->telegram, telegram, RCH_TELEGRAM_BYTES);
    ++tracker->accepted_packets;
}
