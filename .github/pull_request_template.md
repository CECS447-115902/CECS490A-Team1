## What changed

## Why

## How it was tested

- [ ] Ran on hardware (board: )

## Checklist
- [ ] No HAL/CubeMX includes in `app/` or `services/`
- [ ] ISRs only notify tasks; LVGL called only from the ui task; shared buses use their mutex
- [ ] Docs updated if behavior or pin assignment changed
