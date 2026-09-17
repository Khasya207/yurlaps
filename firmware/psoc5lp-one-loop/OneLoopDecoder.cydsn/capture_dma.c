/*
 * YurLaps one-loop decoder - 20 MS/s circular GPIO capture.
 *
 * PSoC Creator components expected by this module are documented in
 * PSoC-Creator-Setup.md. Component/API names are intentionally checked by the
 * syntax test so accidental schematic renaming is caught.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "capture_dma.h"

#include "project.h"

#include <stddef.h>

#if defined(__GNUC__)
static uint8_t sample_buffer[CAPTURE_SAMPLE_COUNT]
    __attribute__((aligned(CAPTURE_SAMPLE_COUNT)));
#else
static uint8_t sample_buffer[CAPTURE_SAMPLE_COUNT];
#endif
static uint8 dma_channel = CY_DMA_INVALID_CHANNEL;
static uint8 dma_td_first = CY_DMA_INVALID_TD;
static uint8 dma_td_second = CY_DMA_INVALID_TD;
static volatile uint8_t trigger_pending;
static volatile uint32_t trigger_low_cycles;
static capture_counters_t counters;

CY_ISR(capture_loop_edge_isr)
{
    if (trigger_pending == 0u)
    {
        trigger_low_cycles = DWT->CYCCNT;
        trigger_pending = 1u;
        ++counters.triggers;

        /* One interrupt is sufficient; the following 5 MHz edges are sampled. */
        LoopIn_SetInterruptMode(LoopIn_0_INTR, LoopIn_INTR_NONE);
    }
    (void)LoopIn_ClearInterrupt();
}

static int wait_for_dma_idle(void)
{
    uint16 attempts;

    for (attempts = 0u; attempts < 256u; ++attempts)
    {
        uint8 state = 0u;
        if (CyDmaChStatus(dma_channel, NULL, &state) != CYRET_SUCCESS)
        {
            return 0;
        }
        if ((state & CY_DMA_STATUS_TD_ACTIVE) == 0u)
        {
            return 1;
        }
    }
    return 0;
}

static int configure_descriptors(void)
{
    cystatus status;

    status = CyDmaTdSetConfiguration(dma_td_first,
                                     CAPTURE_DMA_HALF,
                                     dma_td_second,
                                     CY_DMA_TD_INC_DST_ADR);
    if (status != CYRET_SUCCESS)
    {
        return 0;
    }
    status = CyDmaTdSetConfiguration(dma_td_second,
                                     CAPTURE_DMA_HALF,
                                     dma_td_first,
                                     CY_DMA_TD_INC_DST_ADR);
    if (status != CYRET_SUCCESS)
    {
        return 0;
    }

    status = CyDmaTdSetAddress(dma_td_first,
                               LO16((uint32)LoopIn__PS),
                               LO16((uint32)(uintptr_t)&sample_buffer[0]));
    if (status != CYRET_SUCCESS)
    {
        return 0;
    }
    status = CyDmaTdSetAddress(dma_td_second,
                               LO16((uint32)LoopIn__PS),
                               LO16((uint32)(uintptr_t)&sample_buffer[CAPTURE_DMA_HALF]));
    return status == CYRET_SUCCESS;
}

int capture_dma_init(void)
{
    uint8 index;

    for (index = 0u; index < (uint8)(sizeof(counters)); ++index)
    {
        ((uint8 *)&counters)[index] = 0u;
    }

    SampleClock_Stop();

    /* A PSoC DMA channel has one upper destination-address word. Refuse a
     * linker placement that makes this buffer cross a 64 KiB boundary. */
    if (HI16((uint32)(uintptr_t)&sample_buffer[0]) !=
        HI16((uint32)(uintptr_t)&sample_buffer[CAPTURE_SAMPLE_COUNT - 1u]))
    {
        ++counters.dma_errors;
        return 0;
    }

    dma_channel = SampleDMA_DmaInitialize(1u, 1u,
                                          HI16(CYDEV_PERIPH_BASE),
                                          HI16((uint32)(uintptr_t)sample_buffer));
    dma_td_first = CyDmaTdAllocate();
    dma_td_second = CyDmaTdAllocate();

    if ((dma_channel == CY_DMA_INVALID_CHANNEL) ||
        (dma_td_first == CY_DMA_INVALID_TD) ||
        (dma_td_second == CY_DMA_INVALID_TD) ||
        !configure_descriptors())
    {
        ++counters.dma_errors;
        return 0;
    }

    LoopEdgeISR_StartEx(capture_loop_edge_isr);
    return capture_dma_arm();
}

int capture_dma_arm(void)
{
    cystatus status;

    SampleClock_Stop();
    (void)CyDmaChDisable(dma_channel);
    if (!wait_for_dma_idle())
    {
        ++counters.dma_errors;
        return 0;
    }
    (void)CyDmaClearPendingDrq(dma_channel);

    /* Disabling a channel can write its working TD state back. Restore the
     * original count/address chain before every new circular capture. */
    if (!configure_descriptors())
    {
        ++counters.dma_errors;
        return 0;
    }
    status = CyDmaChSetInitialTd(dma_channel, dma_td_first);
    if (status == CYRET_SUCCESS)
    {
        /* Preserve descriptors because the two-entry chain loops indefinitely. */
        status = CyDmaChEnable(dma_channel, 1u);
    }
    if (status != CYRET_SUCCESS)
    {
        ++counters.dma_errors;
        return 0;
    }

    trigger_pending = 0u;
    (void)LoopIn_ClearInterrupt();
    SampleClock_Start();
    LoopIn_SetInterruptMode(LoopIn_0_INTR, LoopIn_INTR_RISING);
    return 1;
}

int capture_dma_window_due(void)
{
    if (trigger_pending == 0u)
    {
        return 0;
    }
    return (uint32_t)(DWT->CYCCNT - trigger_low_cycles) >=
           CAPTURE_POST_TRIGGER_CYCLES;
}

void capture_dma_freeze(void)
{
    SampleClock_Stop();
    (void)CyDmaChDisable(dma_channel);
    if (!wait_for_dma_idle())
    {
        ++counters.dma_errors;
    }
    (void)CyDmaClearPendingDrq(dma_channel);
    ++counters.windows;
}

const uint8_t *capture_dma_samples(void)
{
    return sample_buffer;
}

uint32_t capture_dma_trigger_low_cycles(void)
{
    return trigger_low_cycles;
}

const capture_counters_t *capture_dma_counters(void)
{
    return &counters;
}
