# core/

The bare-metal scheduler. See `docs/architecture/` for the full design.

- `events.[ch]`: event flags. ISRs only call `events_set()`.
- `scheduler.[ch]`: run-to-completion main loop; sleeps when no flags are pending.
- To add: `softtimer` (LPTIM1 1 ms tick list), `power` (load interlocks, stop-mode selection), `store` (wear-levelled flash for keys and calibration).
