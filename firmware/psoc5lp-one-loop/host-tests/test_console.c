/* Host UART-format/command tests. SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"
#include "serial_console.h"

#define OUTPUT_CAPACITY (8192u)
#define RX_CAPACITY     (256u)

DWT_Type stub_dwt;
CoreDebug_Type stub_core_debug;

static char output[OUTPUT_CAPACITY];
static size_t output_length;
static uint8_t rx_data[RX_CAPACITY];
static size_t rx_read;
static size_t rx_write;
static int failures;

#define CHECK(expression) do {                                                \
    if (!(expression)) {                                                      \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
        ++failures;                                                           \
    }                                                                         \
} while (0)

void HostUART_Start(void) {}

void HostUART_PutChar(uint8 value)
{
    if (output_length < (OUTPUT_CAPACITY - 1u))
    {
        output[output_length++] = (char)value;
        output[output_length] = '\0';
    }
}

void HostUART_PutString(const char *text)
{
    while (*text != '\0')
    {
        HostUART_PutChar((uint8)*text++);
    }
}

uint8 HostUART_GetRxBufferSize(void)
{
    return (uint8)(rx_write - rx_read);
}

uint8 HostUART_ReadRxData(void)
{
    return rx_data[rx_read++];
}

static void send_command(const char *command, const console_status_t *status)
{
    size_t length = strlen(command);
    CHECK((rx_write + length + 1u) < RX_CAPACITY);
    memcpy(&rx_data[rx_write], command, length);
    rx_write += length;
    rx_data[rx_write++] = '\r';
    serial_console_poll(status);
}

static int output_contains(const char *text)
{
    return strstr(output, text) != NULL;
}

int main(void)
{
    static const uint8_t vector[RCH_TELEGRAM_BYTES] = {
        0xF9u, 0x16u, 0xEFu, 0xDAu, 0x35u, 0x3Bu,
        0x28u, 0x29u, 0x0Bu, 0x0Bu, 0xCFu, 0x3Cu
    };
    console_status_t status;
    passage_event_t event;
    rch_demod_result_t packet;
    rch_demod_stats_t scan;
    size_t passage_start;

    memset(&status, 0, sizeof(status));
    status.now_qms = 1234u;
    status.capture.triggers = 5u;
    status.capture.windows = 4u;
    status.valid_packets = 3u;

    serial_console_init();
    CHECK(serial_console_mode() == CONSOLE_MODE_CANO);
    CHECK(output_contains("YURLAPS ONELOOP 0.1 READY 57600 8N1"));

    memset(&event, 0, sizeof(event));
    event.id = 4961721u;
    event.timestamp_qms = 0x1234u;
    event.status10 = 510u;
    event.hits = 3u;
    event.quality_percent = 98u;
    memcpy(event.telegram, vector, sizeof(vector));
    passage_start = output_length;
    serial_console_emit_passage(&event, NULL);
    CHECK(strcmp(&output[passage_start], "4BB5B900001234\r\n") == 0);

    send_command("MODE DIAG", &status);
    CHECK(serial_console_mode() == CONSOLE_MODE_DIAGNOSTIC);
    send_command("MODE", &status);
    CHECK(output_contains("MODE DIAG\r\n"));
    send_command("STATUS", &status);
    CHECK(output_contains("STATUS t_qms=1234 triggers=5 windows=4"));
    send_command("SELFTEST", &status);
    CHECK(output_contains("SELFTEST PASS id=4961721"));

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.telegram, vector, sizeof(vector));
    CHECK(rch_protocol_decode(vector, &packet.decoded));
    packet.quality_percent = 97u;
    packet.first_sample = 811u;
    serial_console_report_packet(&packet, 1234u);
    CHECK(output_contains("PKT raw=F916EFDA353B28290B0BCF3C id=4961721"));
    CHECK(output_contains(" q=97 sample=811 valid=1\r\n"));

    memset(&scan, 0, sizeof(scan));
    scan.samples_scanned = 4096u;
    scan.preamble_candidates = 8u;
    scan.valid_candidates = 5u;
    scan.unique_packets = 1u;
    serial_console_report_scan(&scan);
    CHECK(output_contains("SCAN samples=4096 preambles=8 rejects=0"));

    passage_start = output_length;
    serial_console_emit_passage(&event, NULL);
    CHECK(strncmp(&output[passage_start], "4BB5B900001234\r\nPASS id=4961721",
                  strlen("4BB5B900001234\r\nPASS id=4961721")) == 0);

    send_command("MODE CANO", &status);
    CHECK(serial_console_mode() == CONSOLE_MODE_CANO);

    if (failures != 0)
    {
        fprintf(stderr, "%d console test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("All UART console and Cano-format tests passed.");
    return EXIT_SUCCESS;
}
