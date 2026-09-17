/* UART diagnostics and Cano passage output. SPDX-License-Identifier: BSD-3-Clause */
#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include <stdint.h>

#include "capture_dma.h"
#include "passage_tracker.h"
#include "rch_demod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CONSOLE_MODE_CANO = 0,
    CONSOLE_MODE_DIAGNOSTIC = 1
} console_mode_t;

typedef struct
{
    capture_counters_t capture;
    uint32_t scan_preambles;
    uint32_t scan_rejects;
    uint32_t valid_packets;
    uint32_t now_qms;
    uint32_t accepted_packets;
    uint32_t suppressed_packets;
    uint32_t emitted_passages;
    uint32_t dropped_packets;
} console_status_t;

void serial_console_init(void);
void serial_console_poll(const console_status_t *status);
void serial_console_report_scan(const rch_demod_stats_t *stats);
void serial_console_report_packet(const rch_demod_result_t *packet,
                                  uint32_t timestamp_qms);
void serial_console_emit_passage(const passage_event_t *event, void *context);
console_mode_t serial_console_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_CONSOLE_H */
