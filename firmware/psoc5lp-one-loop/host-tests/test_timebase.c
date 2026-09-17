/* Host state/wrap checks for the PSoC DWT timebase. SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>

#include "project.h"
#include "timebase.h"

DWT_Type stub_dwt;
CoreDebug_Type stub_core_debug;
static int failures;

#define CHECK(expression) do {                                                \
    if (!(expression)) {                                                      \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression); \
        ++failures;                                                           \
    }                                                                         \
} while (0)

int main(void)
{
    uint64_t cycles;

    stub_dwt.CTRL = DWT_CTRL_NOCYCCNT_Msk;
    CHECK(!timebase_init());
    CHECK(timebase_now_cycles() == 0u);

    stub_dwt.CTRL = 0u;
    CHECK(timebase_init());
    CHECK((stub_core_debug.DEMCR & CoreDebug_DEMCR_TRCENA_Msk) != 0u);
    CHECK((stub_dwt.CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0u);

    stub_dwt.CYCCNT = DECODER_CYCLES_PER_QMS;
    CHECK(timebase_now_qms() == 1u);

    stub_dwt.CYCCNT = 0xFFFFFFF0u;
    CHECK(timebase_now_cycles() == 0xFFFFFFF0ULL);
    stub_dwt.CYCCNT = 0x00000014u;
    cycles = timebase_now_cycles();
    CHECK(cycles == 0x100000014ULL);
    CHECK(timebase_extend_recent_low(0xFFFFFFF5u) == 0xFFFFFFF5ULL);
    CHECK(timebase_recent_low_to_qms(0xFFFFFFF5u) ==
          (uint32_t)(0xFFFFFFF5ULL / DECODER_CYCLES_PER_QMS));

    if (failures != 0)
    {
        fprintf(stderr, "%d timebase test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("All PSoC DWT timebase wrap tests passed.");
    return EXIT_SUCCESS;
}
