#include "scheduler.h"
#include "events.h"
#include "bsp.h"

int scheduler_step(const module_t *modules, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        uint32_t taken = events_take(modules[i].event_mask);
        if (taken != 0u) {
            modules[i].process(taken);
            return 1;   /* restart from the top: radio always gets first look */
        }
    }
    return 0;
}

void scheduler_run(const module_t *modules, size_t count)
{
    for (;;) {
        if (scheduler_step(modules, count)) {
            continue;
        }
        /* Mask interrupts, re-check, then sleep. A flag set between the
         * check and WFI still wakes the core because the IRQ stays pending. */
        bsp_irq_disable();
        if (!events_pending()) {
            bsp_sleep();
        }
        bsp_irq_enable();
    }
}
