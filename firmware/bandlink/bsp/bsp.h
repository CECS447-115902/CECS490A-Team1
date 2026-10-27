/**
 * Board support interface. Implemented once per board in bsp/<board>/.
 * Services and app code never call the HAL directly; they go through here
 * or through a driver.
 */
#ifndef BANDLINK_BSP_H
#define BANDLINK_BSP_H

/** Clocks, GPIO, buses, DMA, EXTI, LPTIM1 1 ms tick. Called once from main(). */
void bsp_init(void);

/** Enter the deepest allowed low-power mode (Stop 2 on target). Returns on wake. */
void bsp_sleep(void);

void bsp_irq_disable(void);
void bsp_irq_enable(void);

/** Milliseconds since boot from the LPTIM1 tick (keeps counting in Stop 2). */
unsigned long bsp_millis(void);

#endif
