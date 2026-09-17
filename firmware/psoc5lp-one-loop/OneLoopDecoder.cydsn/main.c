/*
 * YurLaps independent one-loop PSoC 5LP decoder.
 *
 * Receiver path: P12[2] -> 20 MS/s GPIO DMA -> differential-delay correlator
 * -> strict 96-bit packet validator -> multi-ID passage aggregation -> UART.
 * P12[3] is a hardware-routed copy of P12[2] for R11 hysteresis feedback.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "project.h"

#include "capture_dma.h"
#include "passage_tracker.h"
#include "rch_demod.h"
#include "serial_console.h"
#include "timebase.h"

#include <string.h>

#define MAX_PACKETS_PER_WINDOW (4u)
#define LED_ON_TIME_QMS        (400u) /* 100 ms */

static passage_tracker_t passage_tracker;
static console_status_t console_status;
static uint32_t scan_preambles;
static uint32_t scan_rejects;
static uint32_t valid_packets;
static uint32_t led_off_qms;
static uint8_t led_is_on;

static void refresh_console_status(uint32_t now_qms)
{
    const capture_counters_t *capture = capture_dma_counters();

    console_status.capture = *capture;
    console_status.scan_preambles = scan_preambles;
    console_status.scan_rejects = scan_rejects;
    console_status.valid_packets = valid_packets;
    console_status.now_qms = now_qms;
    console_status.accepted_packets = passage_tracker.accepted_packets;
    console_status.suppressed_packets = passage_tracker.suppressed_packets;
    console_status.emitted_passages = passage_tracker.emitted_passages;
    console_status.dropped_packets = passage_tracker.dropped_packets;
}

static int process_capture_window(void)
{
    rch_demod_result_t packets[MAX_PACKETS_PER_WINDOW];
    rch_demod_stats_t stats;
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
                                       &stats);

    scan_preambles += stats.preamble_candidates;
    scan_rejects += stats.coding_rejects;
    valid_packets += (uint32_t)packet_count;

    /* Resume high-rate capture before any potentially slow UART diagnostics. */
    arm_ok = capture_dma_arm();

    serial_console_report_scan(&stats);
    for (index = 0u; index < packet_count; ++index)
    {
        serial_console_report_packet(&packets[index], packet_timestamp);
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

    memset(&console_status, 0, sizeof(console_status));
    StatusLED_Write(1u);
    timebase_ok = (uint8_t)timebase_init();

    CyGlobalIntEnable;
    serial_console_init();
    passage_tracker_init(&passage_tracker, serial_console_emit_passage, NULL);
    capture_ok = (uint8_t)capture_dma_init();

    if (timebase_ok == 0u)
    {
        HostUART_PutString("FATAL DWT CYCLE COUNTER UNAVAILABLE\r\n");
    }
    if (capture_ok == 0u)
    {
        HostUART_PutString("FATAL SAMPLE DMA INITIALIZATION FAILED\r\n");
    }

    for (;;)
    {
        uint32_t now_qms = timebase_now_qms();

        if ((capture_ok != 0u) && capture_dma_window_due())
        {
            capture_ok = (uint8_t)process_capture_window();
            if (capture_ok == 0u)
            {
                HostUART_PutString("FATAL SAMPLE DMA RE-ARM FAILED\r\n");
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

        refresh_console_status(now_qms);
        serial_console_poll(&console_status);
    }
}

/* [] END OF FILE */
