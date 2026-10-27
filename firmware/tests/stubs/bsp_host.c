/* Host stand-in for the board layer so core/ and services/ build on a PC. */
#include "bsp.h"

void bsp_init(void) {}
void bsp_sleep(void) {}
void bsp_irq_disable(void) {}
void bsp_irq_enable(void) {}
unsigned long bsp_millis(void) { return 0; }
