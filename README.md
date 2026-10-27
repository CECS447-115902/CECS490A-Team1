# BandLink — NFC GPS Bracelet

CECS 490A/B Senior Project · Team S02-T01 (Team 1)
Oscar Sandoval · Giorgia Cito · Justin Narciso · Gabriel Dao

A wrist-worn group locator for crowded events where cell networks fail. Each bracelet gets its own GNSS fix and broadcasts an AES-CCM encrypted position over LoRa (915 MHz) to bracelets it was paired with by an NFC tap. The round AMOLED shows an arrow and distance to each friend; UWB adds precise close-range distance (Stretch 2).

## Repository map

| Path | What lives there |
|---|---|
| `docs/proposal/` | Proposal drafts and revisions |
| `docs/architecture/` | Firmware architecture diagrams and design notes |
| `docs/budget/` | BOM + power budget spreadsheet |
| `docs/meetings/` | Weekly meeting notes (`YYYY-MM-DD.md`, copy `_template.md`) |
| `docs/reports/` | Milestone reports, final report, presentation decks |
| `hardware/schematic/` | Interconnect schematic exports (PDF/SVG) |
| `hardware/pcb/` | KiCad project for the custom bracelet PCB |
| `hardware/bom/` | Exported BOMs for ordering (CSV from KiCad) |
| `hardware/datasheets/` | Datasheets, one folder per subsystem |
| `firmware/bandlink/` | STM32U5 firmware (bare-metal, LVGL 9) |
| `firmware/tests/` | Host-side unit tests (run on a PC, no hardware) |
| `firmware/tools/` | Scripts: flashing, log capture, packet decoding |
| `reference/` | Third-party example code kept for reference only. Not built. |

## Firmware layout

`firmware/bandlink/` mirrors the layers in `docs/architecture/`:

```
app/         screens' logic, mode controller (Standby / Typical / Finding)
ui/          LVGL screens + port (display flush, encoder/touch input, tick)
services/    link layer, crypto, navigation, pairing, ranging, battery, haptics
core/        bare-metal scheduler: event flags, main loop, soft timers, power, store
drivers/     one folder per chip: sx1262, max_m10s, pn7150, bno085, co5300, ...
bsp/         per-board pin maps + CubeMX projects (nucleo_u5a5, bandlink_rev_a)
third_party/ LVGL, vendor driver libraries (git submodules)
```

**Dependency rule:** `app/` and `services/` never include a HAL or CubeMX header. Hardware is reached through `drivers/` and `bsp/` interfaces, so the same service code builds for the Nucleo, the custom PCB, and the host test build.

## Getting started

1. Clone with submodules: `git clone --recurse-submodules <repo-url>`
2. Firmware toolchain: STM32CubeIDE or STM32CubeMX + CMake + Arm GNU Toolchain
3. Host tests: `cd firmware/tests && make test` (needs `gcc` and `make`)

## Workflow

- `main` is always buildable. Work on a branch: `fw/<topic>`, `hw/<topic>`, or `docs/<topic>`.
- Open a pull request; one teammate reviews before merging.
- Commit messages: imperative, scoped, e.g. `fw/radio: add TDMA slot timer`.
- Large binaries (PDF, images, firmware `.bin`) are fine up to a few MB. Don't commit build output.

## Subsystem owners

| Subsystem | Owner |
|---|---|
| Radio link (LoRa, TDMA, crypto, relay) | TBD |
| Positioning (GNSS, BNO085, navigation) | TBD |
| NFC pairing + UWB ranging | TBD |
| UI (LVGL, AMOLED, crown, haptics) | TBD |
| Power + PCB (charging, regulators, layout) | TBD |
