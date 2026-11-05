# core/

FreeRTOS glue. The kernel itself comes from the X-CUBE-FREERTOS pack through CubeMX (`bsp/<board>/cubemx/`).

Planned contents (design in progress, nothing written yet):
- **Task table:** priorities, task handles, and the function that creates all tasks.
- **UI message format:** what services send to the ui task.
- **ISR pattern:** every interrupt notifies its owning task and yields; nothing else.
- **Power:** load interlocks and the pre/post-sleep hooks for Stop 2.
- **Store:** wear-levelled flash for keys and calibration.

## Planned FreeRTOSConfig.h settings

| Setting | Value | Why |
|---|---|---|
| `configUSE_PREEMPTION` | 1 | Radio preempts UI |
| `configUSE_TICKLESS_IDLE` | 2 (custom) | LPTIM1-based tick that keeps running in Stop 2 |
| `configUSE_MUTEXES` | 1 | Bus mutexes with priority inheritance |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 | Catch overflows in debug builds |
| `configASSERT` | defined | Trap misuse early |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | per CubeMX | ISRs that call `...FromISR()` must be at or below this priority |
| `configUSE_TICK_HOOK` | 1 | Calls `lv_tick_inc()` |

In CubeMX, set the HAL timebase to **TIM6**, not SysTick, since FreeRTOS owns SysTick.

## Rules
1. ISRs only notify a task (plus timestamp / start DMA). Only `...FromISR()` APIs.
2. One task owns each bus; shared I2C2 goes through `g_i2c2_mutex`.
3. Only the ui task calls LVGL. Everyone else sends it a message through the UI queue.
4. No busy-wait delays: use `vTaskDelay()` or block on a notification.
