/* Host test for exact RCHourglass-compatible TX records. SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"
#include "rch_output.h"

#define OUTPUT_CAPACITY (256u)

DWT_Type stub_dwt;
CoreDebug_Type stub_core_debug;

static char output[OUTPUT_CAPACITY];
static size_t output_length;
static int failures;

#define CHECK(expression) do {                                                \
    if (!(expression)) {                                                      \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
        ++failures;                                                           \
    }                                                                         \
} while (0)

void LapUART_Start(void) {}

void LapUART_PutChar(uint8 value)
{
    if (output_length < (OUTPUT_CAPACITY - 1u))
    {
        output[output_length++] = (char)value;
        output[output_length] = '\0';
    }
}

static void clear_output(void)
{
    output_length = 0u;
    output[0] = '\0';
}

int main(void)
{
    passage_event_t event;

    clear_output();
    rch_output_init();
    CHECK(output_length == 0u); /* Production stream has no startup banner. */

    memset(&event, 0, sizeof(event));
    event.id = 2351957u;          /* 0x23E355 */
    event.timestamp_qms = 0x00075A06u;
    event.hits = 0x15u;
    event.quality_percent = 0x2Au;
    rch_output_emit_passage(&event, NULL);
    CHECK(strcmp(output, "23E35500075A06-01152A0000\r\n") == 0);
    CHECK(output_length == 27u); /* 25 record characters plus CRLF. */

    clear_output();
    event.id = RCH_MAX_DISPLAY_ID;
    event.timestamp_qms = 0xFFFFFFFFu;
    event.hits = 0xFFu;
    event.quality_percent = 100u;
    rch_output_emit_passage(&event, NULL);
    CHECK(strcmp(output, "98967FFFFFFFFF-01FF640000\r\n") == 0);

    if (failures != 0)
    {
        fprintf(stderr, "%d output test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("All RCHourglass-compatible TX record tests passed.");
    return EXIT_SUCCESS;
}
