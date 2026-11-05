# bsp/

`bsp.h` (planned) will be the board interface used by `core/` and the drivers, including the pre/post-sleep hooks FreeRTOS tickless idle calls around Stop 2. Each board folder implements it.

| Board | Use |
|---|---|
| `nucleo_u5a5/` | MVP bring-up on NUCLEO-U5A5ZJ-Q with breakout boards |
| `bandlink_rev_a/` | First custom PCB (STM32U5A9VJT6Q) |

Each board folder holds:
- `cubemx/`: the `.ioc` file and the code CubeMX generates. Regenerate freely; only edit inside `USER CODE` blocks. Call `app_main()` from `USER CODE BEGIN 2` in the generated `main.c` (it starts the scheduler), and set the HAL timebase to TIM6.
- `board.h`: pin and peripheral assignments (which SPI/I2C/EXTI each chip uses).
- `bsp_<board>.c`: implements `bsp.h` (sleep, tick, bus wrappers).
