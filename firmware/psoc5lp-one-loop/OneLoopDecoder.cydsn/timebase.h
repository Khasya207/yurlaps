/* Stable ECO/PLL-derived decoder timebase. SPDX-License-Identifier: BSD-3-Clause */
#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DECODER_CPU_CLOCK_HZ       (80000000UL)
#define DECODER_QMS_PER_SECOND     (4000UL)
#define DECODER_CYCLES_PER_QMS     (20000UL)

int timebase_init(void);
uint64_t timebase_now_cycles(void);
uint32_t timebase_now_qms(void);
uint64_t timebase_extend_recent_low(uint32_t low_cycles);
uint32_t timebase_recent_low_to_qms(uint32_t low_cycles);

#ifdef __cplusplus
}
#endif

#endif /* TIMEBASE_H */
