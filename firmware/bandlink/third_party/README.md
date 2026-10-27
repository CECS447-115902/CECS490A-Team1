# third_party/

External code, added as git submodules where a repository exists. Don't edit in place; wrap it in `drivers/` instead.

| Library | Used by | Source |
|---|---|---|
| LVGL 9 | `ui/` | https://github.com/lvgl/lvgl (pin a release tag) |
| Semtech SX126x driver | `drivers/sx1262` | https://github.com/Lora-net/sx126x_driver |
| CEVA SH-2 | `drivers/bno085` | https://github.com/ceva-dsp/sh2 |
| Qorvo DW3000 API (S2) | `drivers/dw3000` | Qorvo DW3000 SDK download |
| NXP PN7150 NCI examples | `drivers/pn7150` | NXP PN7150 MCU software package |

STM32CubeU5 HAL comes in through CubeMX code generation in `bsp/<board>/cubemx/`.

Example:
```
git submodule add https://github.com/lvgl/lvgl firmware/bandlink/third_party/lvgl
cd firmware/bandlink/third_party/lvgl && git checkout <release-tag>
```
