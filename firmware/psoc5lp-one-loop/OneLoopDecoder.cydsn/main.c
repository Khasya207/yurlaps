/*
 * YurLaps independent one-loop PSoC 5LP lap decoder.
 *
 * Receiver path: P12[2] -> 20 MS/s GPIO DMA -> differential-delay correlator
 * -> strict 96-bit packet validator -> passage aggregation -> TX-only UART.
 * P12[3] is a hardware-routed copy of P12[2] for R11 hysteresis feedback.
 *
 * UART output is the published 25-character RCHourglass passage format:
 * nnnnnntttttttt-idhhqqvvtm followed by CRLF.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "project.h"

#include "capture_dma.h"
#include "passage_tracker.h"
#include "rch_demod.h"
#include "rch_output.h"
#include "timebase.h"

#define MAX_PACKETS_PER_WINDOW (4u)
#define LED_ON_TIME_QMS        (400u) /* 100 ms */

static passage_tracker_t passage_tracker;
static uint32_t led_off_qms;
static uint8_t led_is_on;

static int process_capture_window(void)
{
    rch_demod_result_t packets[MAX_PACKETS_PER_WINDOW];
    size_t packet_count;
    size_t index;
    uint32_t packet_timestamp;
    int arm_ok;

    capture_dma_freeze();
    packet_timestamp = timebase_recent_low_to_qms(
        capture_dma_trigger_low_cycles());
    packet_count = rch_demod_scan_ring(capture_dma_samples(),
                                       CAPTURE_SAMPLE_COUNT,
                                       CAPTURE_LOOP_PIN_MASK,
                                       packets,
                                       MAX_PACKETS_PER_WINDOW,
                                       NULL);

    /* Resume high-rate capture before updating passage state or transmitting. */
    arm_ok = capture_dma_arm();

    for (index = 0u; index < packet_count; ++index)
    {
        passage_tracker_push(&passage_tracker,
                             packets[index].telegram,
                             &packets[index].decoded,
                             packets[index].quality_percent,
                             packet_timestamp);
    }

    if (packet_count != 0u)
    {
        StatusLED_Write(0u); /* CY8CKIT-059 blue LED is active low. */
        led_is_on = 1u;
        led_off_qms = packet_timestamp + LED_ON_TIME_QMS;
    }
    return arm_ok;
}

int main(void)
{
    uint8_t capture_ok;
    uint8_t timebase_ok;

    StatusLED_Write(1u);
    timebase_ok = (uint8_t)timebase_init();

    CyGlobalIntEnable;
    rch_output_init();
    passage_tracker_init(&passage_tracker, rch_output_emit_passage, NULL);
    capture_ok = (timebase_ok != 0u) ? (uint8_t)capture_dma_init() : 0u;

    if ((timebase_ok == 0u) || (capture_ok == 0u))
    {
        /* Steady blue LED means initialization failed. TX remains record-only. */
        StatusLED_Write(0u);
        for (;;)
        {
            /* Recovery is always possible through KitProg/SWD. */
        }
    }

    for (;;)
    {
        uint32_t now_qms = timebase_now_qms();

        if (capture_dma_window_due())
        {
            capture_ok = (uint8_t)process_capture_window();
            if (capture_ok == 0u)
            {
                StatusLED_Write(0u);
                for (;;)
                {
                    /* Do not emit malformed/error text into the lap stream. */
                }
            }
            now_qms = timebase_now_qms();
        }

        passage_tracker_poll(&passage_tracker, now_qms);
        if ((led_is_on != 0u) &&
            ((int32_t)(now_qms - led_off_qms) >= 0))
        {
            StatusLED_Write(1u);
            led_is_on = 0u;
        }
    }
}

/* [] END OF FILE */
