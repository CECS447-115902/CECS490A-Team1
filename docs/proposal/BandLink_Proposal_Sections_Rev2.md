# NFC GPS Bracelet (BandLink) — Proposal Sections, Rev 2

CECS 490A · Team S02-T01 · Replaces the Scope, Architecture, Requirements, Risks, and Materials sections of the Sep 15 draft and adds a two-semester schedule.
Numbers come from `BandLink_BOM_Power_Budget.xlsx` (Oct 2026). Many datasheet duty cycles and most prices are still estimates; they'll be replaced with measured values during 490A.

---

## 1. Scope

The project runs across two semesters. **490A is prototype-only:** the MVP is built from dev boards and breakout modules. **The custom PCB is designed and built in 490B.** Work is delivered in three tiers, and each tier is a working system on its own, so a slip in a later tier doesn't take down the demo.

| Tier | Contents | Target |
|---|---|---|
| **MVP** | GNSS fix, SX1262 LoRa link, NFC tap-to-pair, AES-encrypted location packets, BNO085 9-DOF fusion bearing, GC9A01 round TFT, ERM haptic. Two portable units built from NUCLEO-U5A5ZJ-Q boards and breakout modules, ≥500 m apart; LVGL UI developed on an STM32U5G9J-DK1. | End of 490A |
| **Stretch 1** | Custom PCB (Rev A), 466×466 AMOLED with LVGL UI, PEC06 rotary crown, DRV2605L + LRA haptics, fuel gauge, magnetic pogo charging, 3D-printed enclosure. | 490B |
| **Stretch 2** | DW3000 UWB close-range ranging (distance only), multi-hop LoRa relay. | 490B, if time allows |

**Out of scope:** heart rate / SpO2, speaker or voice, Qi wireless charging, offline maps, voice over LoRa.

---

## 2. System Architecture

### 2.1 Concept of operation

Each bracelet computes its own GNSS position and broadcasts it in an encrypted LoRa packet on 915 MHz. Only bracelets that have been paired by an NFC tap hold the group key, so only they can decrypt it. Each unit combines a peer's position with its own position and the BNO085's fused heading to show a bearing arrow and distance. Inside roughly 30 m (Stretch 2), UWB two-way ranging replaces GPS for the distance reading. The direction still comes from GPS bearing plus compass heading, because a single-antenna UWB radio can't measure angle of arrival. With multi-hop relay enabled (Stretch 2), a bracelet re-broadcasts peer packets it hears, which extends group range past a single LoRa hop. Relaying doesn't improve position accuracy.

### 2.2 Per-bracelet block diagram

```
                    ┌──────────────────────────────────────────────┐
  LiPo 500 mAh ──►  │ BQ25180 charger (power path) ◄── pogo VBUS   │
       │            │ MAX17048 fuel gauge (I2C)                    │
       ▼            │ TPS63802 buck-boost → 3.3 V rail             │
                    │ TLV75518P LDO → 1.8 V (PN7150 pads)          │
                    └───────────────┬──────────────────────────────┘
                                    │ 3.3 V
   ┌───────────────┐   UART + PPS   ┌───────────────────────────┐   SPI1   ┌──────────────────┐
   │ MAX-M10S GNSS │◄──────────────►│                           │◄────────►│ SX1262 LoRa 915  │
   └───────────────┘                │                           │          └──────────────────┘
   ┌───────────────┐   I2C1         │   STM32U5G9VJT6Q          │   SPI2   ┌──────────────────┐
   │ PN7150 NFC    │◄──────────────►│   Cortex-M33 160 MHz      │◄────────►│ DWM3000 UWB (S2) │
   └───────────────┘                │   4 MB flash / 3 MB SRAM  │          └──────────────────┘
   ┌───────────────┐   SPI3         │   NeoChrom VG GPU, AES/PKA│  OCTOSPI ┌──────────────────┐
   │ BNO085 9-DOF  │◄──────────────►│   Bare-metal loop + LVGL  │◄────────►│ 1.43" AMOLED (S1)│
   │ fusion IMU    │  + INT/RST/WAKE│                           │  + TE    └──────────────────┘
   └───────────────┘                │                           │   I2C2   ┌──────────────────┐
                                    │                           │◄────────►│ DRV2605L + LRA   │
                                    └──────┬─────────────┬──────┘          └──────────────────┘
                                 TIM enc.  │             │ USART (pogo)
                                 ┌─────────▼───┐   ┌─────▼──────────────┐
                                 │ PEC06 crown │   │ Dock: bootloader / │
                                 └─────────────┘   │ debug console      │
                                                   └────────────────────┘
```

The 490A MVP uses the same topology on dev boards, split by job:

- **Portable units: NUCLEO-U5A5ZJ-Q (×3: two units + one spare / third test node).** Same STM32U5 family and HAL as the final MCU, 3.3 V I/O, Arduino + Morpho headers. Breakouts wire in directly: LoRa on SPI, GNSS on a UART, BNO085 / NFC / haptics on I2C, a GC9A01 round TFT on SPI for the arrow, and the ERM motor on a GPIO.
- **UI board: STM32U5G9J-DK1 (×1).** The exact final MCU (STM32U5G9) with its 2.47" 480×480 round touch TFT, for LVGL screens and NeoChrom GPU work. Its I/O runs at 1.8 V, so breakouts are not wired to it.

Drivers and services move from the Nucleo to the U5G9 by regenerating the CubeMX project and updating the board pin map. The custom PCB moves the BNO085 to SPI and the display to the QSPI AMOLED.

### 2.3 Interface table

| Peripheral | Bus | Signals | Notes |
|---|---|---|---|
| MAX-M10S GNSS | USART | TX, RX, PPS, RESET_N | PPS goes to a timer input-capture pin and disciplines the TDMA slot clock |
| SX1262 LoRa | SPI1 | SCK, MOSI, MISO, NSS, BUSY, DIO1, NRESET, RXEN/TXEN | Poll BUSY before every command. 3.3 V rail only (VBAT max 3.7 V) |
| PN7150 NFC | I2C1 | SDA, SCL, IRQ, VEN | Held in hard power-down via VEN except during pairing |
| BNO085 9-DOF IMU | SPI3 (PCB) / I2C 0x4A (dev breakout) | SCK, MOSI, MISO, CS, H_INTN, NRST, PS0/WAKE, BOOTN test pad | 3.3 V VDD. On-chip fusion outputs a rotation-vector quaternion. In I2C mode the host must support clock stretching. Placed away from the LRA magnet and battery |
| DRV2605L haptic | I2C2 | SDA, SCL, EN | |
| MAX17048 gauge | I2C2 | SDA, SCL, ALRT | Low-battery interrupt |
| AMOLED (S1) | OCTOSPI (quad) | CLK, NCS, IO0–3, TE, RESET + I2C touch | Frame start synced to TE |
| DWM3000 UWB (S2) | SPI2 | SCK, MOSI, MISO, CS, IRQ, WAKEUP, RSTn | Channel 5, deep sleep between ranging bursts |
| PEC06 crown | TIM (encoder mode) | A, B, SW | RC debounce per Bourns app circuit |
| Pogo dock | USART | VBUS, GND, TX, RX | System bootloader and debug console |

### 2.4 Firmware architecture

Bare-metal, no RTOS. Interrupt handlers only set an event flag, timestamp, or start a DMA transfer. A single main loop checks the flags in fixed priority order and calls each module's `*_process()` function to completion, restarting from the top after each one. When no flags are pending, the core enters Stop 2 (WFI) and wakes on the next EXTI, LPTIM, or UART interrupt. Multi-step exchanges are non-blocking state machines.

Main-loop modules, highest priority first:

1. **radio_process():** SX1262 TX/RX state machine on GPS-synced TDMA slots, AES-128-CCM encryption and authentication, relay queue (S2). The slot-timer ISR issues the SetTx command so transmit timing doesn't depend on the loop.
2. **ranging_process() (S2):** DW3000 ranging state machine; responses use the DW3000's hardware delayed-TX.
3. **gnss_process():** UBX parsing from USART2 DMA, peer position table, bearing and distance computation.
4. **imu_process():** BNO085 SH-2/SHTP driver. Enables the rotation-vector report and converts the quaternion to heading. Saves the dynamic calibration (DCD) to the sensor's flash. Uses the significant-motion and tap reports for wrist-raise wake.
5. **nfc_process():** PN7150 NCI state machine, NFC exchange of the group key and device ID.
6. **ui_process():** `lv_timer_handler()` with LVGL ticked from LPTIM1; the flush callback starts DMA, and the DMA-complete ISR calls `lv_display_flush_ready()`.
7. **system_process():** battery service, watchdog kick, log flush, and the power manager, which enforces load interlocks (e.g., no NFC field while LoRa TX or UWB is active).

Each module has a worst-case run budget (radio and ranging under 2 ms, UI under 5 ms), verified with a GPIO toggle on a scope.

### 2.5 MCU selection

The MCU is the **STM32U5G9VJT6Q** (Cortex-M33, 160 MHz, 4 MB flash, ~3 MB SRAM, LQFP-100).

| | STM32U5G9 (selected) | STM32U5A9 | ESP32-S3 |
|---|---|---|---|
| SRAM | ~3 MB | 2.5 MB | 512 KB + external PSRAM |
| Stop 2, all SRAM retained | 15.5 µA | 17.5 µA | ~0.2–0.25 mA light sleep |
| Graphics | NeoChrom VG GPU, DMA2D, JPEG, GFXTIM | NeoChrom GPU, DMA2D | None (software) |
| SPI buses | SPI1–3 + 2× OCTOSPI + HSPI | SPI1–3 + OCTOSPI + HSPI | 2 general-purpose |
| Bare-metal firmware | Standard (CubeU5) | Standard (CubeU5) | SDK built on FreeRTOS |
| Matching Discovery kit | STM32U5G9J-DK1 (active) | STM32U5A9J-DK (not recommended for new designs) | Waveshare AMOLED boards |
| Unit price (qty 1) | ~$21 | ~$16 | ~$3–4 module |

We chose the U5G9 over the U5A9 because it is the MCU on the STM32U5G9J-DK1 we use for UI development, so UI and GPU code brought up on the kit runs unchanged on the custom PCB. The portable 490A units use the NUCLEO-U5A5ZJ-Q (same family and HAL, 3.3 V I/O, lower cost). It also retains more SRAM in Stop 2 at lower current, and adds a vector-graphics GPU and graphics timer that suit the round AMOLED. We chose the STM32U5 family over the ESP32-S3 because BandLink's radios (LoRa, NFC, UWB) are all external, so the ESP32's Wi-Fi/BLE adds no value, while its RTOS-based SDK conflicts with our bare-metal design and its sleep current is roughly ten times higher. The U5 also provides independent SPI buses for the LoRa, UWB and IMU, plus a separate OCTOSPI for the display.

### 2.6 LoRa link parameters

SF9 / 125 kHz / CR 4/5, 8-symbol preamble, 24-byte payload (ID, sequence, lat/lon, altitude, time, flags, 4-byte MIC). Time on air is **0.206 s per packet**, under the FCC 15.247 limit of 0.4 s dwell per channel. Packet interval is 10 s in standby, 5 s at a typical event, and 2 s in finding mode.

---

## 3. Requirements

| ID | Requirement | Target | Verification |
|---|---|---|---|
| R-01 | LoRa range, line of sight | ≥500 m (goal 1 km) at ≥90% packet delivery | Walk test with two units, log RSSI/SNR and packet delivery rate vs distance |
| R-02 | GNSS accuracy, outdoors | ≤10 m CEP50 | 30 min static logs compared against a surveyed point |
| R-03 | Battery life, mixed 8 h event day | ≥8 h (budget: 16.4 h) | Full-day run from 100% charge on the duty profile below, measured with PPK2 and fuel gauge |
| R-04 | NFC pairing | ≤2 s for ≥95% of trials | 50 timed taps |
| R-05 | Display update latency | ≤10 s after a peer packet is received | Timestamped packet RX vs UI refresh in logs |
| R-06 | Payload confidentiality | Unreadable and unforgeable without the pairing key | Sniff with a third SX1262; replay and bit-flip packets, confirm the MIC rejects them |
| R-07 | Bearing accuracy | ≤15° heading error after BNO085 calibration | Compare rotation-vector heading against a reference compass at 8 headings, check the accuracy status the sensor reports with each reading |
| R-08 | Peak current | Within the cell's pulse rating (budget: 453 mA ≈ 0.9C) | PPK2 capture during worst-case concurrent TX |
| R-09 (S2) | UWB ranging | ±0.3 m at ≤30 m | Tape-measure comparison at 1, 5, 10, 20, 30 m |
| R-10 (S2) | Relay | A packet reaches a peer out of direct range through one relay | Three-unit test with a relay in the middle |

**Duty profile for R-03:** 25% standby tracking (16.6 mA → 25.6 h), 65% typical event (24.4 mA → 17.5 h), 10% active finding (60.0 mA → 7.1 h). The blended average is 26.0 mA, which gives 16.4 h on a 500 mAh cell (Adafruit #1578, 29 × 36 × 4.75 mm) at 85% usable capacity.

---

## 4. Risks

| ID | Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|---|
| K-01 | Custom PCB doesn't work on the first spin, and the 490B start leaves time for only one full spin | High | High | 490A is prototype-only, so the MVP never depends on the PCB. Rev A is a flat, larger test board copied from the working prototype wiring: same radio modules (no chip-down RF), test points, rail-cut jumpers, SWD header. Team design review + ERC/DRC before ordering by 490B week 3–4. A fixes-only Rev B is optional, ordered by week 9; otherwise Rev A is bodge-wired. Wrist-sized layout only if Rev A works cleanly |
| K-02 | Body and wrist detune the GNSS, LoRa, and NFC antennas | High | Medium | Prototype antennas on-wrist early, include tuning pads in the matching network, put a ferrite sheet under the NFC coil |
| K-03 | Battery life misses the 8 h target | Medium | High | Budget shows 8.4 h margin. Levers: GNSS power-save mode, TDMA receive slots, AMOLED timeout, BNO085 report rate. Validate with PPK2 in 490A |
| K-04 | Peak current browns out the cell or regulator | Medium | High | TPS63802 buck-boost, bulk capacitance on the 3.3 V rail, firmware load interlocks, confirm the cell's C-rating |
| K-05 | AMOLED needs a supply the PCB doesn't provide, or the driver IC differs from the listing | Medium | Medium | The 490A MVP uses a GC9A01 round TFT on the Nucleo units and the DK1's built-in round TFT for UI work, so the AMOLED is off the 490A critical path. Buy the panel early in 490B, confirm driver IC, supply voltage and enable pin, and design the PCB supply to match |
| K-06 | LoRa range below 500 m at wrist height | Medium | High | Step SF up to 10–11 (re-check dwell), use 22 dBm output, raise the antenna; the relay (S2) adds range |
| K-07 | BNO085 magnetometer interference from the LRA, battery, or display, or I2C issues with the dev breakout | Medium | Medium | Place it far from the LRA, run the sensor's calibration and save it (DCD), gate heading on the reported accuracy status. Use SPI on the custom PCB. During dev, keep the BNO085 alone on its I2C bus with no multiplexer |
| K-08 | Part lead times or lifecycle (DWM3000 37 wk factory, PN7150 successor drops P2P, U5A9 Discovery board not recommended for new designs, addressed by moving to the U5G9) | Medium | Medium | Buy from distributor stock now. Fall back to NFC card-emulation pairing if moving to PN7160. Use the STM32U5G9J-DK1 for UI development |
| K-09 | Four-person team spreads too thin across subsystems | Medium | High | Each subsystem has a single owner. Tier gates: no Stretch 1 work until the MVP demo passes; no Stretch 2 work until Stretch 1 is done |
| K-10 | RF regulatory compliance | Low | Medium | 915 MHz under FCC Part 15.247 with ≤0.4 s dwell; hop channels; test-only use of non-certified modules |
| K-11 | Code written on the U5A5 Nucleo behaves differently on the U5G9 (pin mapping, memory map, missing GPU on the U5A5) | Medium | Medium | Keep chip code in drivers/ and pins in the board file; regenerate CubeMX for the U5G9 and rerun each driver's smoke test on the DK1 before 490B PCB bring-up. GPU-dependent UI work happens only on the DK1 |

---

## 5. Materials and Sourcing

### 5.1 Budget summary

| Category | Per unit | Qty | Total |
|---|---|---|---|
| Dev prototype set, MVP parts (490A) | $197.50 | 3 (2 units + 1 spare / 3-board tests) | $593 |
| Dev stretch add-ons (490B, before PCB) | $83 | 2 | $166 |
| Custom PCB bracelet (490B) | $171.25 | 3 (2 + spare) | $514 |
| Team tools (DK1, PPK2, STLINK-V3MINIE, TC2030) | — | 1 | $241 |
| **Project total** | | | **≈ $1,514** |
| MVP-only spend (3 MVP sets + DK1 + PPK2) | | | ≈ $782 |

Most line items are still estimates. Verify prices at DigiKey/Mouser before ordering.

### 5.2 Custom PCB parts (per bracelet)

| Subsystem | Part | Tier | ~$ |
|---|---|---|---|
| MCU | STM32U5G9VJT6Q (LQFP100) + crystals | MVP | 22.50 |
| GNSS | u-blox MAX-M10S + L1 chip antenna | MVP | 19 |
| LoRa | SX1262 module (e.g., Ebyte E22-900M22S) + 915 MHz antenna | MVP | 11 |
| NFC | NXP PN7150 + coil/matching network | MVP | 12 |
| Motion | CEVA BNO085 9-DOF fusion IMU + 32.768 kHz crystal | MVP | 18 |
| Power | TPS63802, TLV75518P, 500 mAh LiPo w/ PCM (Adafruit #1578) | MVP | 10.25 |
| Board/mech | 4-layer PCB, passives/ESD, enclosure + strap | MVP | 20 |
| Display | 1.43" 466×466 AMOLED w/ touch (CO5300/SH8601) | S1 | 20 |
| Haptics | DRV2605L + 8 mm LRA | S1 | 4.50 |
| Input | Bourns PEC06 encoder | S1 | 2.50 |
| Charging | BQ25180, MAX17048, 4-pin magnetic pogo connector | S1 | 8.50 |
| UWB | Qorvo DWM3000 | S2 | 23 |

### 5.3 490A prototype order

Prices are approximate (USD, qty 1–10). Items marked * come from distributor listings; the rest are estimates to confirm before ordering.

**Per portable unit: order 3 (2 units + 1 spare / third test node)**

| # | Part | Purpose | Connects via | ~Unit $ | Total for 3 |
|---|---|---|---|---|---|
| 1 | NUCLEO-U5A5ZJ-Q | MCU board (STM32U5, 3.3 V, on-board ST-LINK) | USB | $39.53* | $119 |
| 2 | SparkFun GNSS Breakout MAX-M10S (GPS-18037) | GPS position | UART + PPS pin | $45 | $135 |
| 3 | SX1262 915 MHz breakout + 915 MHz antenna (e.g. Waveshare Core1262-HF) | LoRa radio | SPI + BUSY, DIO1, RESET | $20 | $60 |
| 4 | MIKROE NFC 2 Click (PN7150) | NFC tap-to-pair | I2C + IRQ, VEN | $35* | $105 |
| 5 | Adafruit BNO085 9-DOF IMU (#4754) | Heading for the arrow | I2C (STEMMA QT) + INT, RST | $29.50* | $89 |
| 6 | GC9A01 1.28" round TFT 240×240 | Arrow + distance display | SPI + DC, RST, backlight | $10 | $30 |
| 7 | Coin ERM vibration motor + 2N7002 MOSFET + flyback diode | Haptic alert | 1 GPIO | $3 | $9 |
| 8 | USB power bank (small) | Powers the unit for the 500 m walk test | USB | $15 | $45 |
| | **Per unit** | | | **~$197.50** | **~$593** |

**Team items: order 1 each**

| # | Part | Purpose | ~$ |
|---|---|---|---|
| 9 | STM32U5G9J-DK1 Discovery kit | Same MCU as the final PCB; LVGL/GPU UI work on its 2.47" round touch TFT | $90* |
| 10 | Nordic PPK2 power profiler | Current measurements for R-03 and the power budget | $99 |
| | **Team items** | | **~$189** |

**490A order total: about $782.**

**Small parts, if the team doesn't already have them (~$20–30 total):** jumper wires (M-F, F-F), Qwiic/STEMMA QT cables for the BNO085, a half-size breadboard or perfboard + standoffs per unit so it can be carried, spare headers.

**Check before ordering:**
- **GNSS antenna:** confirm the MAX-M10S breakout has an on-board antenna. If it only has a u.FL connector, add a small passive or active L1 antenna (~$10 each).
- **LoRa frequency:** every SX1262 board and antenna must be the **915 MHz** (US) version, not 868 MHz.
- **NFC Click board:** SparkFun has retired its listing; order from MIKROE or another distributor.

### 5.4 490B items

**Stretch add-ons for 2 units, early 490B (~$83 per unit, ~$166):** AMOLED module, DRV2605L + LRA, PEC06 encoder, MAX17048 fuel-gauge breakout, DWM3000EVB.

**Custom PCB tools (~$52):** STLINK-V3MINIE and Tag-Connect TC2030 cable. The Nucleo and DK1 boards have on-board ST-LINKs, so no separate debugger is needed in 490A.

---

## 6. Schedule

### 6.1 490A: prototype only (now → mid-December)

| Weeks | Work | Hub cards |
|---|---|---|
| 10/28–11/03 | MCU decision recorded (STM32U5: Nucleo-U5A5 units + one DK1 for UI), 3× Nucleo + 1× DK1 + MVP parts ordered, STM32 toolchain set up (CubeMX, blink + UART on Nucleo and DK1), IMU smoke test | MISC-004, HW-04, SW-000 |
| 11/04–11/10 | Each chip brought up alone on the Nucleo: LoRa point-to-point, GNSS fix; Nucleo prototype pin map; LVGL running on the DK1 display | SW-001, SW-002, HW-05 |
| 11/11–11/17 | NFC tap-to-pair, AES-CCM packet encryption, GNSS → LoRa end to end | SW-003, SW-004 |
| 11/18–11/24 | Bearing arrow on display, 500 m walk test, PPK2 power measurement | SW-005, TEST-001, TEST-002 |
| 11/25–12/01 | Buffer (Thanksgiving week): fixes, retests, proposal updates | DOC-03 |
| 12/02–12/07 | Two portable MVP units, demo rehearsal, handoff notes | Build and Integrate MVP Prototype (490A) |

**490A exit criteria:** two portable units pair by NFC, exchange encrypted positions over LoRa at ≥500 m, and each shows the other's direction and distance. Release tagged `v1.0-mvp`.

**Carried into 490B:** the prototype wiring pin map, measured current per subsystem (sizes the PCB regulators and battery), range-test antenna and placement findings, a list of parts that didn't work, and a one-page handoff per subsystem.

### 6.2 490B: custom PCB and stretch goals (late January → May)

| Weeks | Work |
|---|---|
| 1–3 | Rev A schematic and layout from the prototype wiring; team review, ERC/DRC; **order by week 3–4**. If the final MCU differs from the 490A prototype board, port the drivers to its dev kit during this window. |
| 3–6 | While boards are in fab: Stretch 1 firmware on dev boards (LVGL UI on the AMOLED, crown, LRA haptics), enclosure design |
| 5–8 | Rev A bring-up: power rails first, then MCU + SWD, then one chip at a time; fixes logged in `hardware/pcb/rev_a_errata.md` |
| 8–9 | Optional fixes-only Rev B, ordered by week 9 at the latest |
| 8–11 | Charging dock, enclosure fit; Stretch 2 (UWB, relay) only if Stretch 1 is complete |
| 11–13 | Verification of R-01 to R-10, full-day battery test, field test at a crowded campus event |
| 13–15 | Feature freeze, final report, demo, poster |

Release tags: `v2.0-rev-a` when Rev A runs the MVP firmware, `v3.0-final` for the 490B demo.

---

## References (to add to the proposal's reference list)

- STMicroelectronics, STM32U5F9/5G9 product page and datasheet (STM32U5G9VJ) — st.com
- u-blox, MAX-M10S Data Sheet, UBX-20035208
- Semtech, SX1261/2 Datasheet and product page — semtech.com
- Qorvo, DW3000 Datasheet v1.3; DWM3000 module listing (DigiKey)
- NXP, PN7150 Datasheet
- CEVA, BNO08X Datasheet (BNO080/BNO085); Adafruit 9-DOF Orientation IMU Fusion Breakout – BNO085 guide
- Texas Instruments, BQ25120A, TPS63802, DRV2605L datasheets
- Analog Devices, MAX17048/MAX17049 datasheet
- Bourns, PEC06 datasheet
- FCC 47 CFR §15.247 (frequency-hopping systems, 902–928 MHz)
