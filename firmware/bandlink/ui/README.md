# ui/

LVGL 9 screens and the LVGL port.

- `screens/`: finder, group, pairing, watch face, settings. Screens read state from services; they never call drivers.
- `port/`: `lv_conf.h`, display flush callback (OCTOSPI DMA, TE sync, `lv_display_flush_ready()` from the DMA-complete ISR), encoder input from the crown, touch input from the CST820, `lv_tick_inc(1)` from the LPTIM1 tick.

Panel notes (Osptek AM173Q466466FLS, CO5300, see `reference/osptek-amoled-1.73-co5300/`):
- Vendor init sets the column window to 0x0006–0x01D7, so add a 6-pixel column offset in the flush callback.
- Round invalidated areas to even coordinates before flushing.
