/* Host-side deterministic tests. SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "passage_tracker.h"
#include "rch_demod.h"
#include "rch_protocol.h"

#define TEST_SAMPLES (4096u)
#define LOOP_MASK    (0x04u)

static int failures;

#define CHECK(expression) do {                                                \
    if (!(expression)) {                                                      \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
        ++failures;                                                           \
    }                                                                         \
} while (0)

static const uint8_t VECTOR_4961721[RCH_TELEGRAM_BYTES] = {
    0xF9, 0x16, 0xEF, 0xDA, 0x35, 0x3B, 0x28, 0x29, 0x0B, 0x0B, 0xCF, 0x3C
};
static const uint8_t VECTOR_5843805[RCH_TELEGRAM_BYTES] = {
    0xF9, 0x16, 0xE2, 0x9B, 0xAD, 0x5D, 0x7B, 0x44, 0x79, 0x87, 0x7F, 0x03
};
static const uint8_t VECTOR_7632095[RCH_TELEGRAM_BYTES] = {
    0xF9, 0x16, 0xDA, 0xE7, 0x94, 0x77, 0xE9, 0x3C, 0x91, 0xD7, 0xC3, 0xCC
};

static uint8_t packet_bit(const uint8_t *packet, unsigned bit)
{
    return (uint8_t)((packet[bit >> 3] >> (7u - (bit & 7u))) & 1u);
}

static void check_vector(uint32_t id, uint16_t status,
                         const uint8_t expected[RCH_TELEGRAM_BYTES])
{
    uint8_t encoded[RCH_TELEGRAM_BYTES];
    rch_packet_info_t info;

    rch_protocol_encode(id, status, encoded);
    CHECK(rch_protocol_equal(encoded, expected, RCH_TELEGRAM_BYTES));
    CHECK(rch_protocol_decode(expected, &info));
    CHECK(info.id30 == id);
    CHECK(info.status10 == status);
    CHECK(info.preamble_ok == 1u);
    CHECK(info.coding_ok == 1u);
}

static void test_protocol(void)
{
    uint8_t damaged[RCH_TELEGRAM_BYTES];
    uint32_t random_state = 0x20759A31u;
    unsigned iteration;

    check_vector(4961721u, 0u, VECTOR_4961721);
    check_vector(5843805u, 510u, VECTOR_5843805);
    check_vector(7632095u, 76u, VECTOR_7632095);

    for (iteration = 0u; iteration < 1000u; ++iteration)
    {
        uint8_t packet[RCH_TELEGRAM_BYTES];
        rch_packet_info_t decoded;
        uint32_t id;
        uint16_t status;

        random_state = random_state * 1664525u + 1013904223u;
        id = random_state & 0x3FFFFFFFu;
        random_state = random_state * 1664525u + 1013904223u;
        status = (uint16_t)(random_state & 0x03FFu);
        rch_protocol_encode(id, status, packet);
        CHECK(rch_protocol_decode(packet, &decoded));
        CHECK(decoded.id30 == id);
        CHECK(decoded.status10 == status);
    }

    for (iteration = 16u; iteration < RCH_TELEGRAM_BITS; ++iteration)
    {
        memcpy(damaged, VECTOR_4961721, sizeof(damaged));
        damaged[iteration / 8u] ^=
            (uint8_t)(0x80u >> (iteration % 8u));
        CHECK(!rch_protocol_validate(damaged));
    }

    memcpy(damaged, VECTOR_4961721, sizeof(damaged));
    damaged[0] = 0x79u;
    CHECK(!rch_protocol_validate(damaged));
}

/*
 * Make an ideal differential-phase burst. Packet transitions are synchronous
 * to the four-sample carrier, as they are in the published transmitter.
 */
static void make_burst(uint8_t samples[TEST_SAMPLES],
                       const uint8_t packet[RCH_TELEGRAM_BYTES],
                       unsigned packet_start,
                       unsigned carrier_phase,
                       int invert)
{
    unsigned n;
    unsigned bit = 0u;
    uint8_t phase = 0u;
    unsigned burst_start = packet_start - 48u;
    unsigned burst_end = packet_start +
                         (RCH_TELEGRAM_BITS * RCH_SAMPLES_PER_BIT) + 48u;

    memset(samples, 0, TEST_SAMPLES);
    for (n = burst_start; n < burst_end; ++n)
    {
        uint8_t carrier;
        uint8_t level;

        while ((bit < RCH_TELEGRAM_BITS) &&
               (n == packet_start + (bit * RCH_SAMPLES_PER_BIT)))
        {
            phase ^= packet_bit(packet, bit);
            ++bit;
        }

        carrier = (uint8_t)((((n + carrier_phase) / 2u) & 1u) != 0u);
        level = (uint8_t)(carrier ^ phase ^ (uint8_t)(invert != 0));
        samples[n] = level ? LOOP_MASK : 0u;
    }
}

static void rotate_ring(uint8_t samples[TEST_SAMPLES], unsigned distance)
{
    uint8_t temporary[TEST_SAMPLES];
    unsigned i;

    for (i = 0u; i < TEST_SAMPLES; ++i)
    {
        temporary[(i + distance) % TEST_SAMPLES] = samples[i];
    }
    memcpy(samples, temporary, sizeof(temporary));
}

static int result_contains(const rch_demod_result_t *results, size_t count,
                           const uint8_t *packet)
{
    size_t i;
    for (i = 0u; i < count; ++i)
    {
        if (rch_protocol_equal(results[i].telegram, packet,
                               RCH_TELEGRAM_BYTES))
        {
            return 1;
        }
    }
    return 0;
}

static void test_demod_case(unsigned start, unsigned carrier_phase,
                            unsigned rotation, int invert, int add_noise)
{
    uint8_t samples[TEST_SAMPLES];
    rch_demod_result_t results[4];
    rch_demod_stats_t stats;
    size_t count;

    make_burst(samples, VECTOR_4961721, start, carrier_phase, invert);
    if (add_noise)
    {
        /* Deterministic isolated errors; five-way voting should reject them. */
        unsigned n;
        for (n = 701u; n < 2300u; n += 113u)
        {
            samples[n] ^= LOOP_MASK;
        }
    }
    rotate_ring(samples, rotation);

    count = rch_demod_scan_ring(samples, TEST_SAMPLES, LOOP_MASK,
                                results, 4u, &stats);
    CHECK(count >= 1u);
    CHECK(result_contains(results, count, VECTOR_4961721));
    CHECK(stats.samples_scanned == TEST_SAMPLES);
    CHECK(stats.valid_candidates >= 1u);
}

static void test_demod(void)
{
    test_demod_case(512u, 0u, 0u, 0, 0);
    test_demod_case(515u, 1u, 0u, 1, 0);
    test_demod_case(704u, 3u, 3179u, 0, 0); /* Packet wraps in DMA ring. */
    test_demod_case(928u, 2u, 2001u, 1, 1);
}

static passage_event_t emitted[4];
static unsigned emitted_count;

static void collect_passage(const passage_event_t *event, void *context)
{
    (void)context;
    if (emitted_count < 4u)
    {
        emitted[emitted_count++] = *event;
    }
}

static void test_passage_tracker(void)
{
    passage_tracker_t tracker;
    rch_packet_info_t decoded;
    rch_packet_info_t decoded_second;

    emitted_count = 0u;
    CHECK(rch_protocol_decode(VECTOR_4961721, &decoded));
    CHECK(rch_protocol_decode(VECTOR_5843805, &decoded_second));
    passage_tracker_init(&tracker, collect_passage, NULL);
    passage_tracker_push(&tracker, VECTOR_4961721, &decoded, 90u, 1000u);
    passage_tracker_push(&tracker, VECTOR_4961721, &decoded, 80u, 1010u);
    passage_tracker_push(&tracker, VECTOR_4961721, &decoded, 70u, 1020u);
    passage_tracker_poll(&tracker, 1052u);

    CHECK(emitted_count == 1u);
    CHECK(emitted[0].id == 4961721u);
    CHECK(emitted[0].timestamp_qms == 1000u);
    CHECK(emitted[0].hits == 3u);
    CHECK(emitted[0].quality_percent == 80u);

    /* Same ID is suppressed for 300 ms, then accepted as a new passage. */
    passage_tracker_push(&tracker, VECTOR_4961721, &decoded, 100u, 1100u);
    CHECK(tracker.suppressed_packets == 1u);
    passage_tracker_poll(&tracker, 2200u);
    passage_tracker_push(&tracker, VECTOR_4961721, &decoded, 100u, 2201u);
    passage_tracker_poll(&tracker, 2233u);
    CHECK(emitted_count == 2u);

    /* The passage protocol has six ID hex digits; never truncate larger IDs. */
    {
        uint8_t large_id_packet[RCH_TELEGRAM_BYTES];
        rch_packet_info_t large_id;
        rch_protocol_encode(RCH_MAX_DISPLAY_ID + 1u, 0u, large_id_packet);
        CHECK(rch_protocol_decode(large_id_packet, &large_id));
        CHECK(large_id.display_id_ok == 0u);
        passage_tracker_push(&tracker, large_id_packet, &large_id, 100u, 3000u);
        CHECK(tracker.dropped_packets == 1u);
    }

    /* Interleaved close passages retain each transponder's first timestamp. */
    emitted_count = 0u;
    passage_tracker_init(&tracker, collect_passage, NULL);
    passage_tracker_push(&tracker, VECTOR_4961721, &decoded, 90u, 4000u);
    passage_tracker_push(&tracker, VECTOR_5843805, &decoded_second, 80u, 4002u);
    passage_tracker_push(&tracker, VECTOR_4961721, &decoded, 70u, 4008u);
    passage_tracker_push(&tracker, VECTOR_5843805, &decoded_second, 100u, 4012u);
    passage_tracker_poll(&tracker, 4044u);
    CHECK(emitted_count == 2u);
    CHECK(emitted[0].id == 4961721u);
    CHECK(emitted[0].timestamp_qms == 4000u);
    CHECK(emitted[0].hits == 2u);
    CHECK(emitted[1].id == 5843805u);
    CHECK(emitted[1].timestamp_qms == 4002u);
    CHECK(emitted[1].hits == 2u);
}

int main(void)
{
    test_protocol();
    test_demod();
    test_passage_tracker();

    if (failures != 0)
    {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    puts("All protocol and 20 MS/s demodulator tests passed.");
    return EXIT_SUCCESS;
}
