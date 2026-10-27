/**
 * Bare-metal run-to-completion scheduler.
 *
 * Each module registers one event mask and one process function. The loop
 * checks modules in table order, runs the first one with a pending flag,
 * then starts again from the top. With nothing pending it sleeps.
 */
#ifndef BANDLINK_SCHEDULER_H
#define BANDLINK_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *name;        /* for logs and timing measurements        */
    uint32_t    event_mask;  /* EVT_* flags that trigger this module    */
    void      (*process)(uint32_t events); /* receives the flags taken  */
} module_t;

/** Run at most one module. Returns 1 if a module ran, 0 if idle. */
int scheduler_step(const module_t *modules, size_t count);

/** Never returns: scheduler_step() forever, sleeping via bsp_sleep() when idle. */
void scheduler_run(const module_t *modules, size_t count);

#endif
