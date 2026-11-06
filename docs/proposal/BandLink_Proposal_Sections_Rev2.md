# NFC GPS Bracelet (BandLink) — Proposal Sections, Rev 2

CECS 490A · Team S02-T01 · Replaces the Scope, Architecture, Requirements, Risks, and Materials sections of the Sep 15 draft and adds a two-semester schedule.
Numbers come from `BandLink_BOM_Power_Budget.xlsx` (Oct 2026). Many datasheet duty cycles and most prices are still estimates; they'll be replaced with measured values during 490A.

---

## 1. Scope

The project runs across two semesters. **490A is prototype-only:** the MVP is built from dev boards and breakout modules. **The custom PCB is designed and built in 490B.** Work is delivered in three tiers, and each tier is a working system on its own, so a slip in a later tier doesn't take down the demo.

| Tier | Contents | Target |
|---|---|---|
| **MVP** | GNSS fix, SX1262 LoRa link, NFC tap-to-pair, AES-encrypted location packets, BNO085 9-DOF fusion bearing, Osptek AM173Q466466FLS 1.73" round AMOLED (466×466, CO5300 QSPI, CST820 touch), ERM haptic. Two portable units built from NUCLEO-U5A5ZJ-Q boards and breakout modules, ≥500 m apart. | End of 490A |
| **Stretch 1** | Custom PCB (Rev A) carrying the same 1.73" AMOLED with the full LVGL UI, PEC06 rotary crown, DRV2605L + LRA haptics, fuel gauge, magnetic pogo charging, 3D-printed enclosure. | 490B |
| **Stretch 2** | DW3000 UWB close-range ranging (distance only), multi-hop LoRa relay. | 490B, if time allows |

**Out of scope:** heart rate / SpO2, speaker or voice, Qi wireless charging, offline maps, voice over LoRa.

### 1.1 Optional features (candidates)

These are not committed scope. Each one is considered only after the current tier is complete, using the process below.

| ID | Feature | What it does | New hardware? | Effort | Earliest tier | Decide by |
|---|---|---|---|---|---|---|
| OPT-01 | SOS alert | Long-press the crown to broadcast an alert + location to the whole group; receivers vibrate and show an arrow to the sender | No | Low | Stretch 1 | 490B week 13 |
| OPT-02 | Quick messages | Preset short texts sent over LoRa ("Meet at stage," "Leaving," "Need help"), shown on the group screen | No | Low–medium | Stretch 1 | 490B week 13 |
| OPT-03 | Meet-up point | Drop a GPS pin and share it; every bracelet gets an arrow and distance to it | No | Low | Stretch 1 | 490B week 13 |
| OPT-04 | Drift alert | Vibrate when a paired friend moves past a set distance | No | Low | Stretch 1 | 490B week 13 |
| OPT-05 | Breadcrumb trail | Keep a friend's last known positions when they go out of range and point to the most recent one | No | Medium | Stretch 1 | 490B week 13 |
| OPT-06 | Fall / impact detection | Use BNO085 motion reports to detect a hard fall and offer to send an SOS | No | Medium | Stretch 2 | 490B week 13 |
| OPT-07 | Phone companion app | BLE link to a phone for a map view and group setup | **Yes**, BLE chip (e.g. nRF52-series) | High | Stretch 2 | **Before the Rev A schematic is ordered (490B week 3–4)** |
| OPT-08 | Auto-brightness | Ambient light sensor adjusts AMOLED brightness to save power | **Yes**, small I2C light sensor | Low | Stretch 1 | **Before the Rev A schematic is ordered (490B week 3–4)** |

**Adoption process:** a new idea goes on the hub Backlog as an `IDEA-` card. At the weekly meeting the team checks four things:
1. Is the current tier done?
2. Does the idea need hardware?
3. Does it fit the power budget and schedule?
4. Does it put any requirement (R-01 to R-10) at risk?

If adopted, it becomes a normal card with an owner, done-when and due date, and the decision is logged in `docs/decisions.md`.

**Design provisions that keep these options open:**
- **LoRa packet:** a message-type field (see 2.6), so OPT-01 to OPT-05 can add new packet types without breaking compatibility.
- **MCU headroom:** spare SRAM, flash and pins on the U5G9.
- **Rev A PCB:** a small expansion header (spare GPIO, I2C, SPI), test pads on unused MCU pins, and unpopulated footprints for a second button and an LED.

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
   │ BNO085 9-DOF  │◄──────────────►│   FreeRTOS + LVGL         │◄────────►│ 1.73" AMOLED     │
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

- **Prototype board: NUCLEO-U5A5ZJ-Q (×3: two units + one spare / third test node).** Same STM32U5 family and HAL as the final MCU, 3.3 V I/O, Arduino + Morpho headers. Breakouts wire in directly: LoRa on SPI, GNSS on a UART, BNO085 / NFC / haptics on I2C, and the ERM motor on a GPIO.
- **Display: Osptek AM173Q466466FLS (×3), the same panel as the final bracelet.** 1.73" 466×466 round AMOLED on its 24-pin FPC adapter board: CO5300 driver on OCTOSPI1 in quad mode, CST820 touch on I2C. Panel VBAT from the Nucleo's 5 V pin (needs 3.4–5.5 V), VCI_EN from a GPIO. The OCTOSPI1 pins must reach the Nucleo-144 Morpho headers without conflicts (confirm in CubeMX); single-line SPI is the fallback. The U5A5 has no NeoChrom GPU, so LVGL renders in software with DMA2D, using partial redraws.

The STM32U5G9J-DK1 is not part of the 490A setup; it is an optional 490B purchase if UI or GPU work needs the exact final MCU before the custom PCB arrives. Drivers and services move from the Nucleo to the U5G9 by regenerating the CubeMX project and updating the board pin map. The custom PCB moves the BNO085 to SPI; the display and its OCTOSPI wiring stay the same.

### 2.3 Interface table

| Peripheral | Bus | Signals | Notes |
|---|---|---|---|
| MAX-M10S GNSS | USART | TX, RX, PPS, RESET_N | PPS goes to a timer input-capture pin and disciplines the TDMA slot clock |
| SX1262 LoRa | SPI1 | SCK, MOSI, MISO, NSS, BUSY, DIO1, NRESET, RXEN/TXEN | Poll BUSY before every command. 3.3 V rail only (VBAT max 3.7 V) |
| PN7150 NFC | I2C1 | SDA, SCL, IRQ, VEN | Held in hard power-down via VEN except during pairing |
| BNO085 9-DOF IMU | SPI3 (PCB) / I2C 0x4A (dev breakout) | SCK, MOSI, MISO, CS, H_INTN, NRST, PS0/WAKE, BOOTN test pad | 3.3 V VDD. On-chip fusion outputs a rotation-vector quaternion. In I2C mode the host must support clock stretching. Placed away from the LRA magnet and battery |
| DRV2605L haptic | I2C2 | SDA, SCL, EN | |
| MAX17048 gauge | I2C2 | SDA, SCL, ALRT | Low-battery interrupt |
| AMOLED (Osptek AM173Q466466FLS) | OCTOSPI1 (quad) | CLK, NCS, IO0–3, TE, RESET, VCI_EN + touch SDA, SCL, TP_INT, TP_RST | CO5300 driver, CST820 touch. Frame start synced to TE; 6-pixel column offset (vendor window 0x0006–0x01D7); even-aligned redraw regions |
| DWM3000 UWB (S2) | SPI2 | SCK, MOSI, MISO, CS, IRQ, WAKEUP, RSTn | Channel 5, deep sleep between ranging bursts |
| PEC06 crown | TIM (encoder mode) | A, B, SW | RC debounce per Bourns app circuit |
| Pogo dock | USART | VBUS, GND, TX, RX | System bootloader and debug console |

### 2.4 Firmware architecture

**FreeRTOS** (X-CUBE-FREERTOS pack in STM32CubeMX) with preemptive priority scheduling and **tickless idle**: the kernel tick runs from LPTIM1, and when every task is blocked the core enters Stop 2 and wakes on the next EXTI, LPTIM, or UART interrupt. Interrupt handlers do the minimum: they timestamp, start DMA, and wake the owning task with `vTaskNotifyGiveFromISR()`. Data moves between tasks through queues; each bus is owned by one task or guarded by a mutex.

Tasks, highest priority first:

1. **radio:** SX1262 TX/RX on GPS-synced TDMA slots, AES-128-CCM encryption and authentication, relay queue (S2). The slot-timer ISR issues the SetTx command so transmit timing never waits on the scheduler.
2. **ranging (S2):** DW3000 ranging; responses use the DW3000's hardware delayed-TX.
3. **position:** UBX parsing from USART2 DMA, peer position table, bearing and distance computation.
4. **sensor:** BNO085 SH-2/SHTP driver. Enables the rotation-vector report and converts the quaternion to heading. Saves the dynamic calibration (DCD) to the sensor's flash. Uses the significant-motion and tap reports for wrist-raise wake.
5. **pairing:** PN7150 NCI host, NFC exchange of the group key and device ID.
6. **ui:** the only task that calls LVGL. Runs `lv_timer_handler()`, receives display updates from services through a queue; the flush callback starts DMA, and the DMA-complete ISR calls `lv_display_flush_ready()`.
7. **system:** battery service, watchdog kick, log flush, and the power manager, which enforces load interlocks (e.g., no NFC field while LoRa TX or UWB is active).

Stack sizes start at 2–6 KB per task (about 20 KB total) and are checked with `uxTaskGetStackHighWaterMark()`; stack-overflow checking and `configASSERT` stay enabled in debug builds. CubeMX is set to use a timer other than SysTick (e.g. TIM6) for the HAL timebase, as FreeRTOS requires.

### 2.5 MCU selection

The MCU is the **STM32U5G9VJT6Q** (Cortex-M33, 160 MHz, 4 MB flash, ~3 MB SRAM, LQFP-100).

| | STM32U5G9 (selected) | STM32U5A9 | ESP32-S3 |
|---|---|---|---|
| SRAM | ~3 MB | 2.5 MB | 512 KB + external PSRAM |
| Stop 2, all SRAM retained | 15.5 µA | 17.5 µA | ~0.2–0.25 mA light sleep |
| Graphics | NeoChrom VG GPU, DMA2D, JPEG, GFXTIM | NeoChrom GPU, DMA2D | None (software) |
| SPI buses | SPI1–3 + 2× OCTOSPI + HSPI | SPI1–3 + OCTOSPI + HSPI | 2 general-purpose |
| FreeRTOS support | X-CUBE-FREERTOS in CubeMX | X-CUBE-FREERTOS in CubeMX | Built into ESP-IDF |
| Matching Discovery kit | STM32U5G9J-DK1 (active) | STM32U5A9J-DK (not recommended for new designs) | Waveshare AMOLED boards |
| Unit price (qty 1) | ~$21 | ~$16 | ~$3–4 module |

We chose the U5G9 over the U5A9 because it has an active Discovery kit (STM32U5G9J-DK1) we can buy in 490B if UI or GPU work needs the exact final MCU before the custom PCB arrives. All 490A prototyping uses the NUCLEO-U5A5ZJ-Q (same family and HAL, 3.3 V I/O, lower cost). It also retains more SRAM in Stop 2 at lower current, and adds a vector-graphics GPU and graphics timer that suit the round AMOLED. We chose the STM32U5 family over the ESP32-S3 because BandLink's radios (LoRa, NFC, UWB) are all external, so the ESP32's Wi-Fi/BLE adds no value while costing pins and power, and its sleep current is roughly ten times higher. The U5 also provides independent SPI buses for the LoRa, UWB and IMU, plus a separate OCTOSPI for the display.

### 2.6 LoRa link parameters

SF9 / 125 kHz / CR 4/5, 8-symbol preamble, 24-byte payload (ID, sequence, lat/lon, altitude, time, flags, 4-byte MIC). One flags byte carries a **message type** (position, SOS, quick message, meet-up point, relayed), so optional features in 1.1 reuse the same encrypted packet format. Time on air is **0.206 s per packet**, under the FCC 15.247 limit of 0.4 s dwell per channel. Packet interval is 10 s in standby, 5 s at a typical event, and 2 s in finding mode.

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
| K-05 | AMOLED panel VBAT needs 3.4–5.5 V, but the LiPo runs 4.2 V down to 3.0 V, so the screen cuts out in the last ~10–15% of charge if fed from the battery | Medium | Medium | Decide before the Rev A schematic: add a small 5 V boost for VBAT, or set the low-battery shutdown at ~3.4 V and accept the lost capacity. The 490A prototype powers VBAT from the Nucleo's 5 V pin, so this only affects the PCB |
| K-06 | LoRa range below 500 m at wrist height | Medium | High | Step SF up to 10–11 (re-check dwell), use 22 dBm output, raise the antenna; the relay (S2) adds range |
| K-07 | BNO085 magnetometer interference from the LRA, battery, or display, or I2C issues with the dev breakout | Medium | Medium | Place it far from the LRA, run the sensor's calibration and save it (DCD), gate heading on the reported accuracy status. Use SPI on the custom PCB. During dev, keep the BNO085 alone on its I2C bus with no multiplexer |
| K-08 | Part lead times or lifecycle (DWM3000 37 wk factory, PN7150 successor drops P2P, U5A9 Discovery board not recommended for new designs, addressed by moving to the U5G9) | Medium | Medium | Buy from distributor stock now. Fall back to NFC card-emulation pairing if moving to PN7160. Use the STM32U5G9J-DK1 for UI development |
| K-09 | Four-person team spreads too thin across subsystems | Medium | High | Each subsystem has a single owner. Tier gates: no Stretch 1 work until the MVP demo passes; no Stretch 2 work until Stretch 1 is done |
| K-10 | RF regulatory compliance | Low | Medium | 915 MHz under FCC Part 15.247 with ≤0.4 s dwell; hop channels; test-only use of non-certified modules |
| K-11 | Code written on the U5A5 Nucleo behaves differently on the U5G9 (pin mapping, memory map, missing GPU on the U5A5); with no DK1 in 490A, the first run on the exact final MCU is Rev A bring-up | Medium | Medium | Keep chip code in drivers/ and pins in the board file; regenerate CubeMX for the U5G9 early in 490B. Buy an STM32U5G9J-DK1 in 490B if UI or GPU work stalls, and rerun each driver's smoke test on it before the PCB arrives |
| K-12 | RTOS concurrency bugs: races on shared buses, priority inversion, stack overflow, LVGL called from the wrong task | Medium | Medium | One owning task per bus or a mutex (priority inheritance); only `...FromISR()` APIs in interrupts; LVGL called only from the ui task; stack-overflow checking, `configASSERT` and stack high-water checks in debug builds; FreeRTOS-aware debugging in CubeIDE |

---

## 5. Materials and Sourcing

### 5.1 Budget summary

| Category | Per unit | Qty | Total |
|---|---|---|---|
| Dev prototype set, MVP parts (490A) | $217.50 | 3 (2 units + 1 spare / 3-board tests) | $653 |
| Dev stretch add-ons (490B, before PCB) | $58 | 2 | $116 |
| Custom PCB bracelet (490B) | $184.25 | 3 (2 + spare) | $553 |
| Team tools (PPK2, STLINK-V3MINIE, TC2030) | — | 1 | $151 |
| **Project total** | | | **≈ $1,473** |
| MVP-only spend (3 MVP sets + PPK2) | | | ≈ $752 |
| Optional: STM32U5G9J-DK1 (490B, only if needed) | | | $90 |

The AMOLED panel + adapter is budgeted at ~$30 per set until Osptek confirms price and lead time. Most line items are still estimates. Verify prices at DigiKey/Mouser before ordering.

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
| Display | Osptek AM173Q466466FLS 1.73" AMOLED w/ CST820 touch (price TBD) | S1 | 30 |
| Display power | 5 V boost for panel VBAT, if chosen (K-05) | S1 | 3 |
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
| 6 | Osptek AM173Q466466FLS 1.73" round AMOLED + FPC adapter board | Arrow + distance display, touch | OCTOSPI1 (quad) + TE, RESET, VCI_EN; touch on I2C + INT, RST; VBAT from 5 V | ~$30 (TBD) | ~$90 |
| 7 | Coin ERM vibration motor + 2N7002 MOSFET + flyback diode | Haptic alert | 1 GPIO | $3 | $9 |
| 8 | USB power bank (small) | Powers the unit for the 500 m walk test | USB | $15 | $45 |
| | **Per unit** | | | **~$217.50** | **~$653** |

**Team items: order 1 each**

| # | Part | Purpose | ~$ |
|---|---|---|---|
| 9 | Nordic PPK2 power profiler | Current measurements for R-03 and the power budget | $99 |

**490A order total: about $752.**

**Small parts, if the team doesn't already have them (~$20–30 total):** jumper wires (M-F, F-F), Qwiic/STEMMA QT cables for the BNO085, a half-size breadboard or perfboard + standoffs per unit so it can be carried, spare headers.

**Check before ordering:**
- **GNSS antenna:** confirm the MAX-M10S breakout has an on-board antenna. If it only has a u.FL connector, add a small passive or active L1 antenna (~$10 each).
- **LoRa frequency:** every SX1262 board and antenna must be the **915 MHz** (US) version, not 868 MHz.
- **NFC Click board:** SparkFun has retired its listing; order from MIKROE or another distributor.

### 5.4 490B items

**Stretch add-ons for 2 units, early 490B (~$58 per unit, ~$116):** DRV2605L + LRA, PEC06 encoder, MAX17048 fuel-gauge breakout, DWM3000EVB. Optional: one STM32U5G9J-DK1 (~$90) if UI or GPU work needs the exact final MCU before the PCB.

**Custom PCB tools (~$52):** STLINK-V3MINIE and Tag-Connect TC2030 cable. The Nucleo boards have on-board ST-LINKs, so no separate debugger is needed in 490A.

---

## 6. Schedule

### 6.1 490A: prototype only (now → mid-December)

| Weeks | Work | Hub cards |
|---|---|---|
| 10/28–11/03 | MCU decision recorded (STM32U5G9 final, NUCLEO-U5A5ZJ-Q prototypes), display chosen (Osptek AM173Q466466FLS), 3× Nucleo + 3× AMOLED + MVP parts ordered, STM32 toolchain with FreeRTOS set up on the Nucleo, IMU smoke test | MISC-004, HW-03, HW-04, SW-000 |
| 11/04–11/10 | Each chip brought up alone on the Nucleo: LoRa point-to-point, GNSS fix; Nucleo prototype pin map (including OCTOSPI1 for the AMOLED); LVGL running on the AMOLED | SW-001, SW-002, HW-05 |
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
