/*
 * YurLaps one-loop decoder - sampled 5 MHz differential-phase demodulator.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "rch_demod.h"

#include <string.h>

static size_t wrapped_offset(size_t index, int offset, size_t count)
{
    if (offset < 0)
    {
        size_t magnitude = (size_t)(-offset) % count;
        return (index + count - magnitude) % count;
    }
    return (index + (size_t)offset) % count;
}

static uint8_t differential_sample(const uint8_t *samples,
                                   size_t count,
                                   uint8_t mask,
                                   size_t index)
{
    size_t previous = wrapped_offset(index, -(int)RCH_DIFFERENTIAL_DELAY,
                                     count);
    uint8_t now = (uint8_t)((samples[index] & mask) != 0u);
    uint8_t before = (uint8_t)((samples[previous] & mask) != 0u);
    return (uint8_t)(now ^ before);
}

static uint8_t voted_bit(const uint8_t *samples,
                         size_t count,
                         uint8_t mask,
                         size_t center,
                         uint8_t *confidence)
{
    static const int offsets[RCH_DEMOD_VOTE_WIDTH] = {-2, -1, 0, 1, 2};
    uint8_t ones = 0u;
    uint8_t vote;

    for (vote = 0u; vote < RCH_DEMOD_VOTE_WIDTH; ++vote)
    {
        size_t index = wrapped_offset(center, offsets[vote], count);
        ones = (uint8_t)(ones +
                         differential_sample(samples, count, mask, index));
    }

    if (confidence != NULL)
    {
        uint8_t zeros = (uint8_t)(RCH_DEMOD_VOTE_WIDTH - ones);
        *confidence = (ones > zeros) ? ones : zeros;
    }
    return (uint8_t)(ones > (RCH_DEMOD_VOTE_WIDTH / 2u));
}

static uint8_t telegram_bit(const uint8_t *telegram, uint8_t bit_index)
{
    return (uint8_t)((telegram[bit_index >> 3] >>
                      (7u - (bit_index & 7u))) & 1u);
}

static int preamble_matches(const uint8_t *samples,
                            size_t count,
                            uint8_t mask,
                            size_t start)
{
    static const uint8_t preamble[2] = {RCH_PREAMBLE_0, RCH_PREAMBLE_1};
    uint8_t bit;

    for (bit = 0u; bit < 16u; ++bit)
    {
        size_t center = (start + ((size_t)bit * RCH_SAMPLES_PER_BIT)) % count;
        if (voted_bit(samples, count, mask, center, NULL) !=
            telegram_bit(preamble, bit))
        {
            return 0;
        }
    }
    return 1;
}

static void decode_candidate(const uint8_t *samples,
                             size_t count,
                             uint8_t mask,
                             size_t start,
                             uint8_t telegram[RCH_TELEGRAM_BYTES],
                             uint8_t *quality)
{
    uint16_t confidence_sum = 0u;
    uint8_t bit;

    memset(telegram, 0, RCH_TELEGRAM_BYTES);
    for (bit = 0u; bit < RCH_TELEGRAM_BITS; ++bit)
    {
        uint8_t bit_confidence;
        size_t center = (start + ((size_t)bit * RCH_SAMPLES_PER_BIT)) % count;
        uint8_t value = voted_bit(samples, count, mask, center,
                                  &bit_confidence);
        telegram[bit >> 3] = (uint8_t)(telegram[bit >> 3] |
                                      (uint8_t)(value <<
                                      (7u - (bit & 7u))));
        confidence_sum = (uint16_t)(confidence_sum + bit_confidence);
    }

    *quality = (uint8_t)(((uint32_t)confidence_sum * 100u) /
               ((uint32_t)RCH_TELEGRAM_BITS * RCH_DEMOD_VOTE_WIDTH));
}

static size_t find_existing(const rch_demod_result_t *results,
                            size_t result_count,
                            const uint8_t telegram[RCH_TELEGRAM_BYTES])
{
    size_t index;
    for (index = 0u; index < result_count; ++index)
    {
        if (rch_protocol_equal(results[index].telegram, telegram,
                               RCH_TELEGRAM_BYTES))
        {
            return index;
        }
    }
    return result_count;
}

size_t rch_demod_scan_ring(const uint8_t *samples,
                           size_t sample_count,
                           uint8_t pin_mask,
                           rch_demod_result_t *out,
                           size_t out_capacity,
                           rch_demod_stats_t *stats)
{
    size_t start;
    size_t result_count = 0u;
    rch_demod_stats_t local_stats;

    memset(&local_stats, 0, sizeof(local_stats));
    if ((samples == NULL) || (out == NULL) || (sample_count < 256u) ||
        (pin_mask == 0u) || (out_capacity == 0u))
    {
        if (stats != NULL)
        {
            *stats = local_stats;
        }
        return 0u;
    }

    local_stats.samples_scanned = (uint32_t)sample_count;

    /* Trying every raw offset removes packet, bit and carrier phase assumptions. */
    for (start = 0u; start < sample_count; ++start)
    {
        uint8_t telegram[RCH_TELEGRAM_BYTES];
        uint8_t quality;
        rch_packet_info_t decoded;
        size_t existing;

        if (!preamble_matches(samples, sample_count, pin_mask, start))
        {
            continue;
        }
        ++local_stats.preamble_candidates;

        decode_candidate(samples, sample_count, pin_mask, start,
                         telegram, &quality);
        if (!rch_protocol_decode(telegram, &decoded))
        {
            ++local_stats.coding_rejects;
            continue;
        }
        ++local_stats.valid_candidates;

        existing = find_existing(out, result_count, telegram);
        if (existing < result_count)
        {
            if (quality > out[existing].quality_percent)
            {
                out[existing].first_sample = (uint16_t)start;
                out[existing].quality_percent = quality;
            }
            continue;
        }

        if (result_count < out_capacity)
        {
            memcpy(out[result_count].telegram, telegram,
                   RCH_TELEGRAM_BYTES);
            out[result_count].decoded = decoded;
            out[result_count].first_sample = (uint16_t)start;
            out[result_count].quality_percent = quality;
            ++result_count;
        }
    }

    local_stats.unique_packets = (uint16_t)result_count;
    if (stats != NULL)
    {
        *stats = local_stats;
    }
    return result_count;
}
