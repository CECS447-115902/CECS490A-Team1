# drivers/

One folder per chip. Each driver owns its chip's registers/protocol and nothing else.

```
drivers/
  sx1262/     LoRa radio      SPI1   (wraps Semtech sx126x driver in third_party/)
  max_m10s/   GNSS            USART2 + PPS (UBX config + parser)
  pn7150/     NFC             I2C1   (NCI host state machine)
  bno085/     IMU             SPI3   (wraps CEVA sh2 in third_party/)
  co5300/     AMOLED          OCTOSPI (init sequence, window set, flush)
  cst820/     Touch           I2C2
  dw3000/     UWB (S2)        SPI2   (wraps Qorvo DW3000 API)
  drv2605l/   Haptics         I2C2
  max17048/   Fuel gauge      I2C2
  bq25180/    Charger         I2C2
  pec06/      Crown encoder   TIM3 encoder mode
```

Rules:
- No blocking waits. Multi-step operations are state machines advanced from the module's `*_process()`.
- Bus access goes through the `bsp/` bus functions, never directly through HAL handles, so the driver builds for both boards.
- Each driver has `<chip>.h` (public API) and `<chip>.c`. Keep vendor code in `third_party/`, unmodified where possible.
