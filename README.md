# EasyFSK / TinyFSK (OK2ZAW mods)

Arduino/PlatformIO firmware based on TinyFSK by Andrew T. Flowers, K0SM, with
OK2ZAW's PTT sequencer modifications. See [src/TinyFSK_ZAW_01.cpp](src/TinyFSK_ZAW_01.cpp).

## PTT/PA Timing Configuration

While in the `~` configuration menu:

- `L` — PTT lead time, ms (before first bit)
- `T` — PTT tail time, ms (after last bit)
- `l` — PA lead time, ms (before PTT)
- `t` — PA tail time, ms (after PTT)

Type the letter, then digits, then Enter to set a new value (saved to EEPROM
immediately). Press Enter alone to view the current value without changing
it. `~` cancels mid-entry. Defaults (150/25/80/80 ms) are restored if EEPROM
is blank or holds an out-of-range value.
