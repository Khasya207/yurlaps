/* Minimal RCHourglass-compatible passage UART output. SPDX-License-Identifier: BSD-3-Clause */
#ifndef RCH_OUTPUT_H
#define RCH_OUTPUT_H

#include "passage_tracker.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RCH_OUTPUT_DECODER_ID (0x01u)

/* Start the TX-only 57600 8-N-1 UART. No startup text is emitted. */
void rch_output_init(void);

/* passage_emit_fn callback; emits one 25-character RCHourglass record + CRLF. */
void rch_output_emit_passage(const passage_event_t *event, void *context);

#ifdef __cplusplus
}
#endif

#endif /* RCH_OUTPUT_H */
