# datasheets/

One folder per subsystem. Name files `<PART>_<doc>_<rev>.pdf`.

| Folder | Parts |
|---|---|
| `mcu/` | STM32U5A9 datasheet + RM0456 reference manual |
| `gnss/` | u-blox MAX-M10S data sheet + integration manual |
| `lora/` | Semtech SX1262, Ebyte E22-900M22S |
| `nfc/` | NXP PN7150, PCA9306 |
| `imu/` | CEVA BNO08X (BNO085) |
| `uwb/` | Qorvo DW3000 / DWM3000 (S2) |
| `power/` | BQ25180, TPS63802, TLV755P, MAX17048, battery |
| `haptics/` | DRV2605L, LRA |
| `input/` | Bourns PEC06 |
| `display/` | See below |

**Display:** the panel, CO5300 driver, and CST820 touch datasheets are in the vendor package at `reference/osptek-amoled-1.73-co5300/versions/AM173Q466466FLS/docs/`. They're kept there so the vendor README links still work.
