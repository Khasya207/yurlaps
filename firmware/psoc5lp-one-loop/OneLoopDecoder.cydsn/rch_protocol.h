/*
 * YurLaps one-loop decoder - legacy 96-bit telegram codec.
 *
 * This is an independent implementation of the published, observable wire
 * format. It does not contain RCHourglass decoder firmware or recovered source.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef RCH_PROTOCOL_H
#define RCH_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RCH_TELEGRAM_BYTES       (12u)
#define RCH_TELEGRAM_BITS        (96u)
#define RCH_PREAMBLE_0           (0xF9u)
#define RCH_PREAMBLE_1           (0x16u)
#define RCH_CODING_POLYNOMIAL    (0x00776107ULL)
#define RCH_MAX_DISPLAY_ID       (9999999UL)

/* Information decoded from one 12-byte legacy telegram. */
typedef struct
{
    uint32_t id30;       /* All 30 de-punctured ID/data bits. */
    uint16_t status10;   /* The ten interleaved status bits. */
    uint8_t preamble_ok;
    uint8_t coding_ok;   /* Re-encoding all 80 coded payload bits matched. */
    uint8_t display_id_ok;
} rch_packet_info_t;

/*
 * Build a packet from a 30-bit ID/data value and ten status bits. This helper
 * exists primarily for deterministic validation and transponder development.
 */
void rch_protocol_encode(uint32_t id30,
                         uint16_t status10,
                         uint8_t telegram[RCH_TELEGRAM_BYTES]);

/* Decode and validate a received packet. Returns non-zero only when valid. */
int rch_protocol_decode(const uint8_t telegram[RCH_TELEGRAM_BYTES],
                        rch_packet_info_t *info);

/* Strict validation without requiring the caller to keep decoded fields. */
int rch_protocol_validate(const uint8_t telegram[RCH_TELEGRAM_BYTES]);

/* Constant-time byte comparison suitable for short telegram checks. */
int rch_protocol_equal(const uint8_t *left, const uint8_t *right, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* RCH_PROTOCOL_H */
