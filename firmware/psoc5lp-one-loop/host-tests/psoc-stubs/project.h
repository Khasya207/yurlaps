/* Compile-only PSoC Creator API stub. Not target firmware. */
#ifndef PROJECT_H
#define PROJECT_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int cystatus;

typedef struct { volatile uint32_t CTRL; volatile uint32_t CYCCNT; } DWT_Type;
typedef struct { volatile uint32_t DEMCR; } CoreDebug_Type;
extern DWT_Type stub_dwt;
extern CoreDebug_Type stub_core_debug;
#define DWT (&stub_dwt)
#define CoreDebug (&stub_core_debug)
#define DWT_CTRL_NOCYCCNT_Msk (1UL << 25)
#define DWT_CTRL_CYCCNTENA_Msk (1UL)
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)

#define CY_ISR(name) void name(void)
#define CyGlobalIntEnable do { } while (0)
#define CY_DMA_INVALID_CHANNEL (0xFFu)
#define CY_DMA_INVALID_TD (0xFFu)
#define CY_DMA_TD_INC_DST_ADR (0x20u)
#define CY_DMA_STATUS_TD_ACTIVE (0x02u)
#define CYRET_SUCCESS (0)
#define CYDEV_PERIPH_BASE (0x40000000UL)
#define HI16(x) ((uint16)(((uint32_t)(x)) >> 16))
#define LO16(x) ((uint16)((uint32_t)(x)))
#define LoopIn__PS (0x400051C1UL)
#define LoopIn_0_INTR (1u)
#define LoopIn_INTR_NONE (0u)
#define LoopIn_INTR_RISING (1u)

void SampleClock_Stop(void);
void SampleClock_Start(void);
uint8 SampleDMA_DmaInitialize(uint8, uint8, uint16, uint16);
uint8 CyDmaTdAllocate(void);
cystatus CyDmaTdSetConfiguration(uint8, uint16, uint8, uint8);
cystatus CyDmaTdSetAddress(uint8, uint16, uint16);
cystatus CyDmaChDisable(uint8);
cystatus CyDmaChStatus(uint8, uint8 *, uint8 *);
cystatus CyDmaClearPendingDrq(uint8);
cystatus CyDmaChSetInitialTd(uint8, uint8);
cystatus CyDmaChEnable(uint8, uint8);
void LoopEdgeISR_StartEx(void (*handler)(void));
void LoopIn_SetInterruptMode(uint16, uint16);
uint8 LoopIn_ClearInterrupt(void);

void HostUART_Start(void);
void HostUART_PutChar(uint8);
void HostUART_PutString(const char *);
uint8 HostUART_GetRxBufferSize(void);
uint8 HostUART_ReadRxData(void);
void StatusLED_Write(uint8);

#endif /* PROJECT_H */
