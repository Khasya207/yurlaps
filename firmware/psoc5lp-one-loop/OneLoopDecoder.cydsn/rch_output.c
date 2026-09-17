/*
 * Minimal RCHourglass-compatible passage UART output.
 *
 * Record: nnnnnntttttttt-idhhqqvvtm\r\n
 * All fields are uppercase ASCII hexadecimal. Voltage and temperature are 00
 * because this one-loop decoder does not receive those measurements.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "rch_output.h"

#include "project.h"

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
        LapUART_PutChar((uint8)hex_digit((uint8_t)(value >> shift)));
    }
}

void rch_output_init(void)
{
    LapUART_Start();
}

void rch_output_emit_passage(const passage_event_t *event, void *context)
{
    (void)context;

    /* Six ID digits and eight quarter-millisecond timestamp digits. */
    write_hex(event->id, 6u);
    write_hex(event->timestamp_qms, 8u);
    LapUART_PutChar((uint8)'-');

    /* Decoder ID, hits, digital decode quality, voltage, temperature. */
    write_hex(RCH_OUTPUT_DECODER_ID, 2u);
    write_hex(event->hits, 2u);
    write_hex(event->quality_percent, 2u);
    write_hex(0u, 2u); /* Voltage measurement unavailable. */
    write_hex(0u, 2u); /* Temperature measurement unavailable. */
    LapUART_PutChar((uint8)'\r');
    LapUART_PutChar((uint8)'\n');
}

/* [] END OF FILE */
