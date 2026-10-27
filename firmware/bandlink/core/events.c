#include "events.h"

/* LDREX/STREX on Cortex-M33; plain atomics on the host test build. */
static volatile uint32_t s_flags;

void events_set(uint32_t mask)
{
    __atomic_fetch_or(&s_flags, mask, __ATOMIC_RELEASE);
}

uint32_t events_take(uint32_t mask)
{
    return __atomic_fetch_and(&s_flags, ~mask, __ATOMIC_ACQUIRE) & mask;
}

bool events_pending(void)
{
    return __atomic_load_n(&s_flags, __ATOMIC_ACQUIRE) != 0u;
}
