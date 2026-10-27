/* Host tests for core/: event flags and scheduler priority order. */
#include <stdio.h>
#include <stdlib.h>
#include "events.h"
#include "scheduler.h"

static int failures;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static char order[16];
static int  n_ran;
static void mod_a(uint32_t e) { (void)e; order[n_ran++] = 'a'; }
static void mod_b(uint32_t e) { (void)e; order[n_ran++] = 'b'; }
static void mod_c(uint32_t e) { (void)e; order[n_ran++] = 'c'; events_set(EVT_RADIO); }

static void test_events(void)
{
    events_take(0xFFFFFFFFu);
    CHECK(!events_pending());
    events_set(EVT_GNSS | EVT_UI);
    CHECK(events_pending());
    CHECK(events_take(EVT_GNSS) == EVT_GNSS);
    CHECK(events_take(EVT_GNSS) == 0u);          /* already cleared */
    CHECK(events_take(EVT_UI | EVT_IMU) == EVT_UI);
    CHECK(!events_pending());
}

static void test_priority(void)
{
    const module_t mods[] = {
        { "a", EVT_RADIO, mod_a },
        { "b", EVT_GNSS,  mod_b },
        { "c", EVT_UI,    mod_c },
    };
    events_take(0xFFFFFFFFu);
    n_ran = 0;
    events_set(EVT_UI | EVT_GNSS);
    while (scheduler_step(mods, 3)) {}
    order[n_ran] = '\0';
    /* b first (higher priority), then c, which raises RADIO, so a runs next. */
    CHECK(n_ran == 3 && order[0] == 'b' && order[1] == 'c' && order[2] == 'a');
    CHECK(scheduler_step(mods, 3) == 0);
}

int main(void)
{
    test_events();
    test_priority();
    if (failures) { printf("%d check(s) failed\n", failures); return EXIT_FAILURE; }
    printf("core: all tests passed\n");
    return EXIT_SUCCESS;
}
