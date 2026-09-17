/* Compile/link behavioral checks for the PSoC DMA wrapper. SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>

#include "capture_dma.h"
#include "project.h"

DWT_Type stub_dwt;
CoreDebug_Type stub_core_debug;

static uint8 allocated_td = 10u;
static unsigned stop_calls;
static unsigned start_calls;
static unsigned configure_calls;
static unsigned address_calls;
static unsigned disable_calls;
static unsigned status_calls;
static unsigned clear_calls;
static unsigned initial_td_calls;
static unsigned enable_calls;
static unsigned interrupt_mode_calls;
static uint8 last_preserve;
static uint8 interrupt_mode;
static uint8 force_status_failure;
static void (*edge_handler)(void);
static int failures;

#define CHECK(expression) do {                                                \
    if (!(expression)) {                                                      \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
        ++failures;                                                           \
    }                                                                         \
} while (0)

void SampleClock_Stop(void) { ++stop_calls; }
void SampleClock_Start(void) { ++start_calls; }
uint8 SampleDMA_DmaInitialize(uint8 burst, uint8 request, uint16 source,
                              uint16 destination)
{
    CHECK(burst == 1u);
    CHECK(request == 1u);
    CHECK(source == HI16(CYDEV_PERIPH_BASE));
    (void)destination;
    return 3u;
}
uint8 CyDmaTdAllocate(void) { return allocated_td++; }
cystatus CyDmaTdSetConfiguration(uint8 td, uint16 count, uint8 next,
                                 uint8 configuration)
{
    CHECK((td == 10u) || (td == 11u));
    CHECK(count == CAPTURE_DMA_HALF);
    CHECK((next == 10u) || (next == 11u));
    CHECK(configuration == CY_DMA_TD_INC_DST_ADR);
    ++configure_calls;
    return CYRET_SUCCESS;
}
cystatus CyDmaTdSetAddress(uint8 td, uint16 source, uint16 destination)
{
    CHECK((td == 10u) || (td == 11u));
    CHECK(source == LO16((uint32)LoopIn__PS));
    (void)destination;
    ++address_calls;
    return CYRET_SUCCESS;
}
cystatus CyDmaChDisable(uint8 channel)
{
    CHECK(channel == 3u);
    ++disable_calls;
    return CYRET_SUCCESS;
}
cystatus CyDmaChStatus(uint8 channel, uint8 *current_td, uint8 *state)
{
    CHECK(channel == 3u);
    (void)current_td;
    ++status_calls;
    if (force_status_failure != 0u)
    {
        return -1;
    }
    *state = 0u;
    return CYRET_SUCCESS;
}
cystatus CyDmaClearPendingDrq(uint8 channel)
{
    CHECK(channel == 3u);
    ++clear_calls;
    return CYRET_SUCCESS;
}
cystatus CyDmaChSetInitialTd(uint8 channel, uint8 td)
{
    CHECK(channel == 3u);
    CHECK(td == 10u);
    ++initial_td_calls;
    return CYRET_SUCCESS;
}
cystatus CyDmaChEnable(uint8 channel, uint8 preserve)
{
    CHECK(channel == 3u);
    ++enable_calls;
    last_preserve = preserve;
    return CYRET_SUCCESS;
}
void LoopEdgeISR_StartEx(void (*handler)(void)) { edge_handler = handler; }
void LoopIn_SetInterruptMode(uint16 pin, uint16 mode)
{
    CHECK(pin == LoopIn_0_INTR);
    ++interrupt_mode_calls;
    interrupt_mode = (uint8)mode;
}
uint8 LoopIn_ClearInterrupt(void) { return 0u; }

int main(void)
{
    const capture_counters_t *counters;

    CHECK(capture_dma_init());
    CHECK(edge_handler != NULL);
    CHECK(stop_calls >= 2u);
    CHECK(start_calls == 1u);
    CHECK(configure_calls == 4u);
    CHECK(address_calls == 4u);
    CHECK(disable_calls == 1u);
    CHECK(status_calls == 1u);
    CHECK(clear_calls == 1u);
    CHECK(initial_td_calls == 1u);
    CHECK(enable_calls == 1u);
    CHECK(last_preserve == 1u);
    CHECK(interrupt_mode == LoopIn_INTR_RISING);

    stub_dwt.CYCCNT = 100u;
    edge_handler();
    counters = capture_dma_counters();
    CHECK(counters->triggers == 1u);
    CHECK(interrupt_mode == LoopIn_INTR_NONE);
    stub_dwt.CYCCNT = 8099u;
    CHECK(!capture_dma_window_due());
    stub_dwt.CYCCNT = 8100u;
    CHECK(capture_dma_window_due());

    capture_dma_freeze();
    CHECK(capture_dma_counters()->windows == 1u);
    CHECK(capture_dma_arm());
    CHECK(configure_calls == 6u); /* TD state is restored on every re-arm. */
    CHECK(start_calls == 2u);

    force_status_failure = 1u;
    CHECK(!capture_dma_arm());
    CHECK(capture_dma_counters()->dma_errors == 1u);

    if (failures != 0)
    {
        fprintf(stderr, "%d capture test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("All PSoC DMA-wrapper state tests passed.");
    return EXIT_SUCCESS;
}
