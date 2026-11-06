# BandLink — NFC GPS Bracelet

CECS 490A/B Senior Project · Team S02-T01 (Team 1)
Oscar Sandoval · Giorgia Cito · Justin Narciso · Gabriel Dao

A wrist-worn group locator for crowded events where cell networks fail. Each bracelet gets its own GNSS fix and broadcasts an AES-CCM encrypted position over LoRa (915 MHz) to bracelets it was paired with by an NFC tap. The round AMOLED shows an arrow and distance to each friend; UWB adds precise close-range distance (Stretch 2).

## Repository map

| Path | What lives there |
|---|---|
| `docs/proposal/` | Proposal drafts and revisions |
| `docs/architecture/` | Firmware and system architecture diagrams and design notes |
| `docs/budget/` | BOM + power budget spreadsheet |
| `docs/BandLink_Timeline.pdf` | Projected timeline, Aug 2026 to May 2027 (PNG + PDF) |
| `docs/BandLink_Task_Map.pdf` | Hub task dependency map from docs/tasks.md, critical path to the 490A demo (PNG + PDF) |
| `docs/architecture/BandLink_Module_Design.md` | Module design: layers, module table with interfaces, owner and reviewer, module test plan (TC-MOD) |
| `docs/architecture/BandLink_Module_Layers.pdf` | Layered module diagram: layers × owning FreeRTOS task (PNG + PDF) |
| `docs/traceability.csv` | Requirements R-01…R-10 mapped to modules, tests and evidence |
| `evidence/` | Test and demo evidence, named by test ID (see `evidence/README.md`) |
| `docs/tasks.md` | Task plan: hub cards with owners, due dates and planned branch names |
| `docs/meetings/` | Weekly meeting notes (`YYYY-MM-DD.md`, copy `_template.md`) |
| `docs/ui/` | UI sketches and screen designs |
| `docs/reports/` | Milestone reports, final report, presentation decks |
| `hardware/schematic/` | Interconnect schematic exports (PDF/SVG) |
| `hardware/pcb/` | KiCad project for the custom bracelet PCB |
| `hardware/bom/` | Exported BOMs for ordering (CSV from KiCad) |
| `hardware/datasheets/` | Datasheets, one folder per subsystem |
| `firmware/bandlink/` | STM32U5 firmware (FreeRTOS, LVGL 9) |
| `firmware/tests/` | Host-side unit tests (planned; run on a PC, no hardware) |
| `firmware/tools/` | Scripts: flashing, log capture, packet decoding |
| `reference/` | Third-party example code kept for reference only. Not built. |

## Firmware layout

The bracelet firmware will run on **FreeRTOS** (X-CUBE-FREERTOS via CubeMX): 7 prioritized tasks, ISRs that only notify tasks, and tickless idle into Stop 2. See `firmware/bandlink/core/README.md` for the planned rules.

`firmware/bandlink/` mirrors the layers in `docs/architecture/`:

```
app/         screens' logic, mode controller (Standby / Typical / Finding)
ui/          LVGL screens + port (display flush, encoder/touch input, tick)
services/    link layer, crypto, navigation, pairing, ranging, battery, haptics
core/        FreeRTOS glue: task table, priorities, UI queue, bus mutexes, ISR pattern
drivers/     one folder per chip: sx1262, max_m10s, pn7150, bno085, co5300, ...
bsp/         per-board pin maps + CubeMX projects (nucleo_u5a5, bandlink_rev_a)
third_party/ LVGL, vendor driver libraries (git submodules)
```

**Dependency rule:** `app/` and `services/` never include a HAL or CubeMX header. Hardware is reached through `drivers/` and `bsp/` interfaces, so the same service code builds for the Nucleo, the custom PCB, and the host test build.

## Status

System architecture is still being designed. The firmware folders and READMEs describe the planned structure; no firmware code is written yet.

## Getting started

1. Clone with submodules: `git clone --recurse-submodules <repo-url>`
2. Firmware toolchain: STM32CubeIDE or STM32CubeMX + CMake + Arm GNU Toolchain
3. Host tests: added once services exist (see `firmware/tests/README.md`)

## Git workflow

We use **GitHub Flow**: one protected `main`, short-lived branches, and every change goes through a pull request.

### Branches

| Branch | Purpose | Lifetime |
|---|---|---|
| `main` | Always works. What we'd demo or hand in. | Permanent, protected |
| `<area>/<card-id>-<topic>` | One hub task | Days, not weeks |

Area prefixes: `fw/` firmware · `hw/` schematic, PCB, BOM · `docs/` proposal, reports, architecture · `ui/` LVGL screens · `test/` test runs and logs · `app/` iOS app (if OPT-07 goes ahead)

Examples: `fw/sw-001-lora-p2p`, `hw/hw-05-pin-map`, `docs/doc-03-proposal-update`

- **One hub card = one branch = one PR.** Put the card ID in the branch name.
- Branch from the latest `main` (`git pull` first) and merge back within about a week.
- Delete the branch after merging.

### `main` branch protection (GitHub → Settings → Branches)

- Require a pull request before merging, with **1 approval** from a teammate.
- Block force-pushes and branch deletion.
- Once CI exists: require the CI check to pass.

### Commits

- Small and focused; each commit leaves the project working.
- Format `area: what changed`, imperative: `fw/radio: add TDMA slot timer`, `hw: move BNO085 to SPI3 in pin map`, `docs: update proposal for FreeRTOS`.
- Never commit build output or secrets (`.gitignore` covers CubeIDE, KiCad and build folders).

### Pull requests

- **Title:** card ID + card title, e.g. `SW-001: LoRa point-to-point link`.
- **Description:** the card's "done when," plus **evidence**: log output, photo, video, or scope capture.
- **Reviewer:** whoever knows that area best; rotate so everyone learns the whole system.
- **Merge:** squash merge, so each card is one commit on `main`.
- **After merge:** move the hub card to Done and paste the PR link into it.

### File-type rules

| Files | Rule |
|---|---|
| CubeMX `.ioc` | One owner per board config at a time; announce before editing. Regenerate after merging and only edit inside `USER CODE` blocks. |
| KiCad schematic / PCB | One person edits at a time, announced in chat. Commit exported PDFs alongside so reviewers can see changes without KiCad. |
| Markdown docs | Merge normally. One sentence per line if conflicts become a problem. |
| Binaries (PDF, xlsx, images) | Fine up to a few MB each. If the repo passes a few hundred MB, enable Git LFS for `*.pdf`, `*.mp4`, `*.dwg`. |
| Line endings | `.gitattributes` forces LF for source. On Windows, run `git config --global core.autocrlf true` once. |

### Milestone tags

Tag `main` when a milestone works, and publish it as a GitHub Release with a short "what works" note:

| Tag | Milestone |
|---|---|
| `v0.x-<name>` | 490A steps, e.g. `v0.1-lora-link`, `v0.2-gnss-in-packet` |
| `v1.0-mvp` | End of 490A: two portable units pass the 500 m test |
| `v2.0-rev-a` | Custom PCB runs the firmware |
| `v3.0-final` | 490B demo |

### Where things are tracked

- **CECS 490 hub:** the task board. `docs/tasks.md` mirrors it with planned branch names.
- **GitHub:** code, docs, pull requests, and reviews. No duplicate GitHub Issues unless the team decides otherwise.
- **`docs/decisions.md`:** one line per design decision (date, decision, why).

### Weekly rhythm

- **Before the weekly meeting:** merge what's ready; tag if a milestone was hit.
- **At the meeting:** walk through the week's merged PRs and their evidence.
- **Friday integration:** pull `main`, flash all test boards, run the end-to-end chain.

## Subsystem owners

| Subsystem | Owner |
|---|---|
| Radio link (LoRa, TDMA, crypto, relay) | TBD |
| Positioning (GNSS, BNO085, navigation) | TBD |
| NFC pairing + UWB ranging | TBD |
| UI (LVGL, AMOLED, crown, haptics) | TBD |
| Power + PCB (charging, regulators, layout) | TBD |
