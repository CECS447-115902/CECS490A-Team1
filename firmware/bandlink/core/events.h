/**
 * Event flags: the only thing an ISR is allowed to touch.
 *
 * ISRs call events_set(); the main loop calls events_take() to claim and
 * clear a flag, then runs that module's *_process() to completion.
 */
#ifndef BANDLINK_EVENTS_H
#define BANDLINK_EVENTS_H

#include <stdbool.h>
#include <stdint.h>

/* Bit order is also the main-loop priority order (lowest bit runs first). */
enum {
    EVT_RADIO   = 1u << 0,  /* SX1262 DIO1, TDMA slot timer      */
    EVT_RANGING = 1u << 1,  /* DW3000 IRQ, ranging timer   (S2)  */
    EVT_GNSS    = 1u << 2,  /* USART2 idle-line DMA, PPS capture */
    EVT_IMU     = 1u << 3,  /* BNO085 H_INTN                     */
    EVT_NFC     = 1u << 4,  /* PN7150 IRQ, pairing request       */
    EVT_UI      = 1u << 5,  /* LVGL timer, TE, crown, new data   */
    EVT_SYSTEM  = 1u << 6,  /* 1 s timer, fuel-gauge ALRT, CHG   */
};

/** Set one or more flags. Safe to call from any ISR or from the main loop. */
void events_set(uint32_t mask);

/** Atomically clear the flags in mask; return which of them were set. */
uint32_t events_take(uint32_t mask);

/** True if any flag is pending. Call with interrupts masked before sleeping. */
bool events_pending(void);

#endif
