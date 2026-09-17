/*
 * YurLaps one-loop decoder - legacy 96-bit telegram codec.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "rch_protocol.h"

static uint8_t parity64(uint64_t value)
{
    /* Fold to one parity bit; no compiler-specific built-in is required. */
    value ^= value >> 32;
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value &= 0x0Fu;
    return (uint8_t)((0x6996u >> value) & 1u);
}

static uint64_t punctuate(uint32_t id30, uint16_t status10)
{
    uint64_t stream = 0u;
    uint8_t id_index = 0u;
    uint8_t status_index = 0u;
    uint8_t position;

    id30 &= 0x3FFFFFFFUL;
    status10 &= 0x03FFu;

    /* Three ID bits followed by one status bit, repeated ten times. */
    for (position = 0u; position < 40u; ++position)
    {
        stream <<= 1;
        if ((position & 3u) != 3u)
        {
            stream |= (uint64_t)((id30 >> (29u - id_index)) & 1u);
            ++id_index;
        }
        else
        {
            stream |= (uint64_t)((status10 >> (9u - status_index)) & 1u);
            ++status_index;
        }
    }

    return stream;
}

void rch_protocol_encode(uint32_t id30,
                         uint16_t status10,
                         uint8_t telegram[RCH_TELEGRAM_BYTES])
{
    uint64_t input = punctuate(id30, status10);
    uint64_t history = input & 1u;
    uint8_t pair_index;

    telegram[0] = RCH_PREAMBLE_0;
    telegram[1] = RCH_PREAMBLE_1;
    for (pair_index = 2u; pair_index < RCH_TELEGRAM_BYTES; ++pair_index)
    {
        telegram[pair_index] = 0u;
    }

    input >>= 1;

    /*
     * Forty source bits become forty two-bit code symbols. Symbols are packed
     * MSB first in bytes 2..11. The final next_bit is the defined zero tail.
     */
    for (pair_index = 0u; pair_index < 40u; ++pair_index)
    {
        uint8_t next_bit = (uint8_t)(input & 1u);
        uint8_t feedback = parity64(history & RCH_CODING_POLYNOMIAL);
        uint8_t first = (uint8_t)(feedback ^ next_bit);
        uint8_t second = (uint8_t)(first ^ (uint8_t)(history & 1u));
        uint8_t byte_index = (uint8_t)(2u + (pair_index >> 2));

        telegram[byte_index] = (uint8_t)((telegram[byte_index] << 1) | first);
        telegram[byte_index] = (uint8_t)((telegram[byte_index] << 1) | second);

        input >>= 1;
        history = (history << 1) | next_bit;
    }
}

static uint64_t recover_punctuated(const uint8_t telegram[RCH_TELEGRAM_BYTES])
{
    uint64_t stream = 0u;
    uint8_t symbol;

    /*
     * In each two-bit symbol, unequal bits represent a one in the delayed
     * source stream and equal bits represent a zero.
     */
    for (symbol = 0u; symbol < 40u; ++symbol)
    {
        uint8_t byte_index = (uint8_t)(2u + (symbol >> 2));
        uint8_t shift = (uint8_t)(6u - ((symbol & 3u) << 1));
        uint8_t pair = (uint8_t)((telegram[byte_index] >> shift) & 3u);
        uint8_t source_bit = (uint8_t)(((pair >> 1) ^ pair) & 1u);
        stream |= ((uint64_t)source_bit << symbol);
    }

    return stream;
}

int rch_protocol_equal(const uint8_t *left, const uint8_t *right, size_t count)
{
    uint8_t difference = 0u;
    size_t index;

    for (index = 0u; index < count; ++index)
    {
        difference |= (uint8_t)(left[index] ^ right[index]);
    }
    return difference == 0u;
}

int rch_protocol_decode(const uint8_t telegram[RCH_TELEGRAM_BYTES],
                        rch_packet_info_t *info)
{
    uint64_t stream;
    uint32_t id30 = 0u;
    uint16_t status10 = 0u;
    uint8_t rebuilt[RCH_TELEGRAM_BYTES];
    uint8_t position;
    uint8_t id_index = 0u;
    uint8_t status_index = 0u;
    uint8_t preamble_ok;
    uint8_t coding_ok;

    if ((telegram == NULL) || (info == NULL))
    {
        return 0;
    }

    preamble_ok = (uint8_t)((telegram[0] == RCH_PREAMBLE_0) &&
                            (telegram[1] == RCH_PREAMBLE_1));
    stream = recover_punctuated(telegram);

    /* Undo the 3+1 punctuation in transmission order (bit 39 first). */
    for (position = 0u; position < 40u; ++position)
    {
        uint8_t bit = (uint8_t)((stream >> (39u - position)) & 1u);
        if ((position & 3u) != 3u)
        {
            id30 = (id30 << 1) | bit;
            ++id_index;
        }
        else
        {
            status10 = (uint16_t)((status10 << 1) | bit);
            ++status_index;
        }
    }

    (void)id_index;
    (void)status_index;
    rch_protocol_encode(id30, status10, rebuilt);
    coding_ok = (uint8_t)rch_protocol_equal(telegram, rebuilt,
                                             RCH_TELEGRAM_BYTES);

    info->id30 = id30;
    info->status10 = status10;
    info->preamble_ok = preamble_ok;
    info->coding_ok = coding_ok;
    info->display_id_ok = (uint8_t)(id30 <= RCH_MAX_DISPLAY_ID);

    return (preamble_ok != 0u) && (coding_ok != 0u);
}

int rch_protocol_validate(const uint8_t telegram[RCH_TELEGRAM_BYTES])
{
    rch_packet_info_t info;
    return rch_protocol_decode(telegram, &info);
}
