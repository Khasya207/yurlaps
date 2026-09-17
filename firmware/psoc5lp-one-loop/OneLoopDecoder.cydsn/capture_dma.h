/* 20 MS/s circular GPIO capture for one loop. SPDX-License-Identifier: BSD-3-Clause */
#ifndef CAPTURE_DMA_H
#define CAPTURE_DMA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAPTURE_SAMPLE_COUNT          (4096u)
#define CAPTURE_DMA_HALF              (2048u)
#define CAPTURE_POST_TRIGGER_US       (100u)
#define CAPTURE_POST_TRIGGER_CYCLES   (8000UL)
#define CAPTURE_LOOP_PIN_MASK         (0x04u) /* P12[2] */

typedef struct
{
    uint32_t triggers;
    uint32_t windows;
    uint32_t dma_errors;
} capture_counters_t;

int capture_dma_init(void);
int capture_dma_arm(void);
int capture_dma_window_due(void);
void capture_dma_freeze(void);
const uint8_t *capture_dma_samples(void);
uint32_t capture_dma_trigger_low_cycles(void);
const capture_counters_t *capture_dma_counters(void);

#ifdef __cplusplus
}
#endif

#endif /* CAPTURE_DMA_H */
