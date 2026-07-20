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

### Examples

Connect a serial terminal at 9600 baud, 8-N-1. Each example is what you
type/send, including the leading `~`; your terminal must send a CR or LF for
"Enter" (most terminal apps do this automatically).

- Set PTT lead to 200 ms: `~L200` then Enter
- Set PTT tail to 30 ms: `~T30` then Enter
- Set PA lead to 100 ms: `~l100` then Enter
- Set PA tail to 50 ms: `~t50` then Enter
- Check the current PTT tail without changing it: `~T` then Enter
- Start typing a value but cancel instead of committing it: `~L` then `~`

Each set/get prints the full current configuration afterward so you can
confirm the change.
