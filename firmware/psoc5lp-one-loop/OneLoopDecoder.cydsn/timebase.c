/* Stable ECO/PLL-derived decoder timebase. SPDX-License-Identifier: BSD-3-Clause */
#include "timebase.h"

#include "project.h"

static uint32_t previous_low;
static uint64_t cycle_epoch;
static uint8_t cycle_counter_available;

int timebase_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    if ((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) != 0u)
    {
        cycle_counter_available = 0u;
        return 0;
    }

    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    previous_low = 0u;
    cycle_epoch = 0u;
    cycle_counter_available = 1u;
    return 1;
}

uint64_t timebase_now_cycles(void)
{
    uint32_t low;

    if (cycle_counter_available == 0u)
    {
        return 0u;
    }

    low = DWT->CYCCNT;
    if (low < previous_low)
    {
        cycle_epoch += (1ULL << 32);
    }
    previous_low = low;
    return cycle_epoch | low;
}

uint32_t timebase_now_qms(void)
{
    return (uint32_t)(timebase_now_cycles() / DECODER_CYCLES_PER_QMS);
}

uint64_t timebase_extend_recent_low(uint32_t low_cycles)
{
    uint64_t now = timebase_now_cycles();
    uint64_t candidate = (now & 0xFFFFFFFF00000000ULL) | low_cycles;

    /* A capture is at most milliseconds old. Correct the rare wrap boundary. */
    if ((low_cycles > (uint32_t)now) &&
        ((low_cycles - (uint32_t)now) > 0x80000000UL))
    {
        candidate -= (1ULL << 32);
    }
    return candidate;
}

uint32_t timebase_recent_low_to_qms(uint32_t low_cycles)
{
    return (uint32_t)(timebase_extend_recent_low(low_cycles) /
                      DECODER_CYCLES_PER_QMS);
}
