/*
 * YurLaps one-loop decoder - sampled 5 MHz differential-phase demodulator.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef RCH_DEMOD_H
#define RCH_DEMOD_H

#include <stddef.h>
#include <stdint.h>

#include "rch_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 20 MS/s gives four samples/carrier and sixteen samples/encoded bit. */
#define RCH_SAMPLE_RATE_HZ        (20000000UL)
#define RCH_CARRIER_HZ            (5000000UL)
#define RCH_SAMPLES_PER_CARRIER   (4u)
#define RCH_SAMPLES_PER_BIT       (16u)
#define RCH_DIFFERENTIAL_DELAY    (16u)
#define RCH_DEMOD_VOTE_WIDTH      (5u)

typedef struct
{
    uint8_t telegram[RCH_TELEGRAM_BYTES];
    rch_packet_info_t decoded;
    uint16_t first_sample;
    uint8_t quality_percent;
} rch_demod_result_t;

typedef struct
{
    uint32_t samples_scanned;
    uint16_t preamble_candidates;
    uint16_t coding_rejects;
    uint16_t valid_candidates;
    uint16_t unique_packets;
} rch_demod_stats_t;

/*
 * Search one circular DMA buffer. Each byte is a sampled GPIO port; pin_mask
 * selects the LoopIn bit. The buffer may begin at any carrier/packet phase.
 *
 * Returns the number of unique, strictly validated telegrams written to out.
 */
size_t rch_demod_scan_ring(const uint8_t *samples,
                           size_t sample_count,
                           uint8_t pin_mask,
                           rch_demod_result_t *out,
                           size_t out_capacity,
                           rch_demod_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* RCH_DEMOD_H */
