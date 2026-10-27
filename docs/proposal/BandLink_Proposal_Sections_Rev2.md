# NFC GPS Bracelet (BandLink) — Proposal Sections, Rev 2

CECS 490A · Team S02-T01 · Replaces the Architecture, Requirements, Risks, and Materials sections of the Sep 15 draft.
Numbers come from `BandLink_BOM_Power_Budget.xlsx` (Oct 2026). Many datasheet duty cycles and most prices are still estimates; they'll be replaced with measured values during 490A.

---

## 1. Scope

The project is delivered in three tiers. Each tier is a working system on its own, so a slip in a later tier doesn't take down the demo.

| Tier | Contents | Target |
|---|---|---|
| **MVP** | GNSS fix, SX1262 LoRa link, NFC tap-to-pair, AES-encrypted location packets, BNO085 9-DOF fusion bearing, simple round TFT, ERM haptic. Two units on dev boards, ≥500 m apart. | 490A approval demo |
| **Stretch 1** | 1.43" 466×466 AMOLED with LVGL UI, PEC06 rotary crown, DRV2605L + LRA haptics, fuel gauge, magnetic pogo charging. Custom wearable PCB. | 490B |
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
   ┌───────────────┐   I2C1         │   STM32U5A9VJT6Q          │   SPI2   ┌──────────────────┐
   │ PN7150 NFC    │◄──────────────►│   Cortex-M33 160 MHz      │◄────────►│ DWM3000 UWB (S2) │
   └───────────────┘                │   4 MB flash / 2.5 MB SRAM│          └──────────────────┘
   ┌───────────────┐   SPI3         │   NeoChrom GPU, AES/PKA   │  OCTOSPI ┌──────────────────┐
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

The MVP uses the same topology on dev boards: a NUCLEO-U5A5ZJ-Q, the Adafruit BNO085 breakout on I2C (STEMMA QT), a GC9A01 round TFT on SPI in place of the AMOLED, and an ERM motor on a GPIO. The custom PCB moves the BNO085 to SPI.

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

### 2.5 LoRa link parameters

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
| K-01 | Custom wearable PCB doesn't work on the first spin (RF layout, small form factor) | High | High | MVP runs on dev boards, so the demo doesn't depend on the PCB. Use pre-built LoRa and UWB modules instead of chip-down RF. Board spin 1 is a flat test board, and miniaturization comes in spin 2 |
| K-02 | Body and wrist detune the GNSS, LoRa, and NFC antennas | High | Medium | Prototype antennas on-wrist early, include tuning pads in the matching network, put a ferrite sheet under the NFC coil |
| K-03 | Battery life misses the 8 h target | Medium | High | Budget shows 8.4 h margin. Levers: GNSS power-save mode, TDMA receive slots, AMOLED timeout, BNO085 report rate. Validate with PPK2 in 490A |
| K-04 | Peak current browns out the cell or regulator | Medium | High | TPS63802 buck-boost, bulk capacitance on the 3.3 V rail, firmware load interlocks, confirm the cell's C-rating |
| K-05 | AMOLED needs an external ELVDD/ELVSS supply, or the driver IC differs from the listing | Medium | Medium | Buy the panel early, confirm the driver IC, add a display power chip if needed. The GC9A01 TFT is the fallback |
| K-06 | LoRa range below 500 m at wrist height | Medium | High | Step SF up to 10–11 (re-check dwell), use 22 dBm output, raise the antenna; the relay (S2) adds range |
| K-07 | BNO085 magnetometer interference from the LRA, battery, or display, or I2C issues with the dev breakout | Medium | Medium | Place it far from the LRA, run the sensor's calibration and save it (DCD), gate heading on the reported accuracy status. Use SPI on the custom PCB. During dev, keep the BNO085 alone on its I2C bus with no multiplexer |
| K-08 | Part lead times or lifecycle (DWM3000 37 wk factory, PN7150 successor drops P2P, U5A9 Discovery board not recommended for new designs) | Medium | Medium | Buy from distributor stock now. Fall back to NFC card-emulation pairing if moving to PN7160. Use the STM32U5G9J-DK1 for UI development |
| K-09 | Four-person team spreads too thin across subsystems | Medium | High | Each subsystem has a single owner. Tier gates: no Stretch 1 work until the MVP demo passes |
| K-10 | RF regulatory compliance | Low | Medium | 915 MHz under FCC Part 15.247 with ≤0.4 s dwell; hop channels; test-only use of non-certified modules |

---

## 5. Materials and Sourcing

### 5.1 Budget summary

| Category | Per unit | Qty | Total |
|---|---|---|---|
| Dev bring-up set (490A) | $270.50 | 2 | $541 |
| Custom PCB bracelet (490B) | $166.25 | 3 (2 + spare) | $499 |
| Team tools (one-time) | — | 1 | $241 |
| **Project total** | | | **≈ $1,281** |
| MVP-only spend | | | ≈ $486 |

Most line items are still estimates. Verify prices at DigiKey/Mouser before ordering.

### 5.2 Custom PCB parts (per bracelet)

| Subsystem | Part | Tier | ~$ |
|---|---|---|---|
| MCU | STM32U5A9VJT6Q (LQFP100) + crystals | MVP | 17.50 |
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

### 5.3 Dev bring-up set (per bracelet, 490A)

NUCLEO-U5A5ZJ-Q, SparkFun MAX-M10S breakout, SX1262 915 MHz breakout, MIKROE NFC 2 Click (PN7150), Adafruit BNO085 9-DOF breakout (#4754, $29.50), GC9A01 round TFT, ERM motor, USB power bank. Stretch add-ons: Waveshare 1.43" AMOLED module, DRV2605L + LRA, PEC06, MAX17048 breakout, DWM3000EVB.

### 5.4 Team tools

STM32U5G9J-DK1 Discovery (LVGL/GPU development), STLINK-V3MINIE, Tag-Connect TC2030 cable, Nordic PPK2 power profiler.

---

## References (to add to the proposal's reference list)

- STMicroelectronics, STM32U599/5A9 product page and datasheet — st.com
- u-blox, MAX-M10S Data Sheet, UBX-20035208
- Semtech, SX1261/2 Datasheet and product page — semtech.com
- Qorvo, DW3000 Datasheet v1.3; DWM3000 module listing (DigiKey)
- NXP, PN7150 Datasheet
- CEVA, BNO08X Datasheet (BNO080/BNO085); Adafruit 9-DOF Orientation IMU Fusion Breakout – BNO085 guide
- Texas Instruments, BQ25120A, TPS63802, DRV2605L datasheets
- Analog Devices, MAX17048/MAX17049 datasheet
- Bourns, PEC06 datasheet
- FCC 47 CFR §15.247 (frequency-hopping systems, 902–928 MHz)
