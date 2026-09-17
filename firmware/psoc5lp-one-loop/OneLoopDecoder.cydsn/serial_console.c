/* UART diagnostics and Cano passage output. SPDX-License-Identifier: BSD-3-Clause */
#include "serial_console.h"

#include "project.h"
#include "rch_protocol.h"

#include <string.h>

#define COMMAND_BUFFER_BYTES (48u)

static char command_buffer[COMMAND_BUFFER_BYTES];
static uint8_t command_length;
static console_mode_t output_mode = CONSOLE_MODE_CANO;

static void write_char(char value)
{
    HostUART_PutChar((uint8)value);
}

static void write_text(const char *text)
{
    HostUART_PutString(text);
}

static char hex_digit(uint8_t value)
{
    value &= 0x0Fu;
    if (value < 10u)
    {
        return (char)('0' + (int)value);
    }
    return (char)('A' + (int)value - 10);
}

static void write_hex(uint32_t value, uint8_t digits)
{
    uint8_t position;
    for (position = 0u; position < digits; ++position)
    {
        uint8_t shift = (uint8_t)((digits - 1u - position) * 4u);
        write_char(hex_digit((uint8_t)(value >> shift)));
    }
}

static void write_decimal(uint32_t value)
{
    char reversed[10];
    uint8_t length = 0u;

    do
    {
        reversed[length++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while ((value != 0u) && (length < sizeof(reversed)));

    while (length != 0u)
    {
        write_char(reversed[--length]);
    }
}

static void write_packet_hex(const uint8_t packet[RCH_TELEGRAM_BYTES])
{
    uint8_t index;
    for (index = 0u; index < RCH_TELEGRAM_BYTES; ++index)
    {
        write_hex(packet[index], 2u);
    }
}

static void uppercase_command(void)
{
    uint8_t index;
    for (index = 0u; index < command_length; ++index)
    {
        if ((command_buffer[index] >= 'a') && (command_buffer[index] <= 'z'))
        {
            command_buffer[index] = (char)(command_buffer[index] - 'a' + 'A');
        }
    }
    command_buffer[command_length] = '\0';
}

static void report_status(const console_status_t *status)
{
    write_text("STATUS t_qms=");
    write_decimal(status->now_qms);
    write_text(" triggers=");
    write_decimal(status->capture.triggers);
    write_text(" windows=");
    write_decimal(status->capture.windows);
    write_text(" dma_errors=");
    write_decimal(status->capture.dma_errors);
    write_text(" preambles=");
    write_decimal(status->scan_preambles);
    write_text(" rejects=");
    write_decimal(status->scan_rejects);
    write_text(" packets=");
    write_decimal(status->valid_packets);
    write_text(" passages=");
    write_decimal(status->emitted_passages);
    write_text(" accepted=");
    write_decimal(status->accepted_packets);
    write_text(" suppressed=");
    write_decimal(status->suppressed_packets);
    write_text(" dropped=");
    write_decimal(status->dropped_packets);
    write_text("\r\n");
}

static void run_selftest(void)
{
    static const uint8_t known[RCH_TELEGRAM_BYTES] = {
        0xF9u, 0x16u, 0xEFu, 0xDAu, 0x35u, 0x3Bu,
        0x28u, 0x29u, 0x0Bu, 0x0Bu, 0xCFu, 0x3Cu
    };
    uint8_t rebuilt[RCH_TELEGRAM_BYTES];
    rch_packet_info_t decoded;
    int ok;

    rch_protocol_encode(4961721u, 0u, rebuilt);
    ok = rch_protocol_equal(known, rebuilt, RCH_TELEGRAM_BYTES) &&
         rch_protocol_decode(known, &decoded) &&
         (decoded.id30 == 4961721u);
    write_text(ok ? "SELFTEST PASS id=4961721 vector=F916EFDA353B28290B0BCF3C\r\n" :
                    "SELFTEST FAIL\r\n");
}

static void execute_command(const console_status_t *status)
{
    uppercase_command();

    if ((strcmp(command_buffer, "HELP") == 0) ||
        (strcmp(command_buffer, "?") == 0))
    {
        write_text("COMMANDS: STATUS, SELFTEST, MODE, MODE CANO, MODE DIAG, VERSION, HELP\r\n");
    }
    else if (strcmp(command_buffer, "STATUS") == 0)
    {
        report_status(status);
    }
    else if (strcmp(command_buffer, "SELFTEST") == 0)
    {
        run_selftest();
    }
    else if (strcmp(command_buffer, "MODE") == 0)
    {
        write_text((output_mode == CONSOLE_MODE_CANO) ?
                   "MODE CANO\r\n" : "MODE DIAG\r\n");
    }
    else if (strcmp(command_buffer, "MODE CANO") == 0)
    {
        output_mode = CONSOLE_MODE_CANO;
        write_text("OK MODE CANO\r\n");
    }
    else if (strcmp(command_buffer, "MODE DIAG") == 0)
    {
        output_mode = CONSOLE_MODE_DIAGNOSTIC;
        write_text("OK MODE DIAG\r\n");
    }
    else if (strcmp(command_buffer, "VERSION") == 0)
    {
        write_text("YurLaps OneLoop PSoC5LP 0.1-independent\r\n");
    }
    else if (command_length != 0u)
    {
        write_text("ERROR UNKNOWN COMMAND; TYPE HELP\r\n");
    }
}

void serial_console_init(void)
{
    HostUART_Start();
    command_length = 0u;
    output_mode = CONSOLE_MODE_CANO;
    write_text("YURLAPS ONELOOP 0.1 READY 57600 8N1; TYPE HELP\r\n");
}

void serial_console_poll(const console_status_t *status)
{
    while (HostUART_GetRxBufferSize() != 0u)
    {
        char value = (char)HostUART_ReadRxData();
        if ((value == '\r') || (value == '\n'))
        {
            if (command_length != 0u)
            {
                execute_command(status);
                command_length = 0u;
            }
        }
        else if (command_length < (COMMAND_BUFFER_BYTES - 1u))
        {
            command_buffer[command_length++] = value;
        }
        else
        {
            command_length = 0u;
            write_text("ERROR COMMAND TOO LONG\r\n");
        }
    }
}

void serial_console_report_scan(const rch_demod_stats_t *stats)
{
    if (output_mode != CONSOLE_MODE_DIAGNOSTIC)
    {
        return;
    }

    write_text("SCAN samples=");
    write_decimal(stats->samples_scanned);
    write_text(" preambles=");
    write_decimal(stats->preamble_candidates);
    write_text(" rejects=");
    write_decimal(stats->coding_rejects);
    write_text(" valid_candidates=");
    write_decimal(stats->valid_candidates);
    write_text(" unique=");
    write_decimal(stats->unique_packets);
    write_text("\r\n");
}

void serial_console_report_packet(const rch_demod_result_t *packet,
                                  uint32_t timestamp_qms)
{
    if (output_mode != CONSOLE_MODE_DIAGNOSTIC)
    {
        return;
    }

    write_text("PKT raw=");
    write_packet_hex(packet->telegram);
    write_text(" id=");
    write_decimal(packet->decoded.id30);
    write_text(" status=");
    write_decimal(packet->decoded.status10);
    write_text(" t_qms=");
    write_decimal(timestamp_qms);
    write_text(" q=");
    write_decimal(packet->quality_percent);
    write_text(" sample=");
    write_decimal(packet->first_sample);
    write_text(" valid=1\r\n");
}

void serial_console_emit_passage(const passage_event_t *event, void *context)
{
    (void)context;

    /* Cano: six ID hex digits + eight quarter-millisecond timestamp digits. */
    write_hex(event->id & 0x00FFFFFFUL, 6u);
    write_hex(event->timestamp_qms, 8u);
    write_text("\r\n");

    if (output_mode == CONSOLE_MODE_DIAGNOSTIC)
    {
        write_text("PASS id=");
        write_decimal(event->id);
        write_text(" t_qms=");
        write_decimal(event->timestamp_qms);
        write_text(" hits=");
        write_decimal(event->hits);
        write_text(" q=");
        write_decimal(event->quality_percent);
        write_text(" status=");
        write_decimal(event->status10);
        write_text(" raw=");
        write_packet_hex(event->telegram);
        write_text("\r\n");
    }
}

console_mode_t serial_console_mode(void)
{
    return output_mode;
}
