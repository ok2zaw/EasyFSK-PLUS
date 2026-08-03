# EasyFSK / TinyFSK (OK2ZAW mods)

Arduino/PlatformIO firmware based on TinyFSK by Andrew T. Flowers, K0SM, with
OK2ZAW's PTT sequencer modifications. See [src/TinyFSK_ZAW_01.cpp](src/TinyFSK_ZAW_01.cpp).

Target board: Arduino Nano (ATmega328), 9600 baud USB-serial control from the
logging/RTTY program (e.g. N1MM+, FLDIGI).

## Pinout

| Arduino pin | ATmega328 pin | Define             | Direction | Function                                                        |
|-------------|----------------|--------------------|-----------|-------------------------------------------------------------------|
| D2          | 32 (PD2)       | `PTT_USB_RTS_PIN`  | INPUT     | Hardware PTT-request input, e.g. from the RTS line of an external/alternate TNC's USB-serial adapter. External pull-up; **active LOW**. Engages the same PA/PTT lead-tail sequencer as the `[`/`]` serial commands, without going through this board's own serial port or internal FSK generator |
| D3          | 1 (PD3)        | `CPU_INH_PIN`      | INPUT     | Hardware inhibit input. Internal pull-up; **active LOW**. While asserted, blocks all PTT/FSK keying and force-stops an in-progress TX immediately; the PC is notified with an `inh:` status line |
| D4          | 2 (PD4)        | `LED_RX_PIN`       | OUTPUT    | Receive indicator LED. **Active HIGH.** Always the opposite state of `PTT_PIN` — lit while receiving, off while transmitting |
| D8          | —              | `ON_PIN`           | OUTPUT    | Reserved (configured as output at startup, not yet driven by firmware) |
| D10         | —              | `PTT_PA_PIN`       | OUTPUT    | PA/amplifier keying — first relay closed, last relay opened (outer stage of the sequence) |
| D11         | —              | `FSK_PIN`          | OUTPUT    | FSK audio/keying line — mark/space Baudot output to the transmitter |
| D13         | —              | `PTT_PIN`          | OUTPUT    | Main transceiver PTT — closed after the PA relay, opened before it (inner stage of the sequence) |
| A4 (SDA) / A5 (SCL) | —      | — (I2C)            | I2C       | Status LCD — 16x2 character display on a PCF8574 I2C backpack, address `0x27` |
| USB/Serial  | —              | —                  | —         | Serial control/data from the RTTY program, 9600 baud 8-N-1        |

`FSK_PIN`, `PTT_PIN`, `PTT_PA_PIN`, `ON_PIN`, and `LED_RX_PIN` are set as
`OUTPUT` in `setup()`. Polarity of `FSK_PIN` (mark = logical HIGH or LOW) is
configurable — see [Configuration Menu](#configuration-menu) below.
`PTT_PIN`, `PTT_PA_PIN`, and `LED_RX_PIN` are active-HIGH. `PTT_USB_RTS_PIN`
is an `INPUT` relying on an external pull-up resistor (not the internal one)
and is active-LOW. `CPU_INH_PIN` is an `INPUT` using the ATmega328's
internal pull-up, so a disconnected pin defaults to "not inhibited"; it is
also active-LOW.

Pin assignment (11 = FSK, 13 = PTT) is kept compatible with K3NG's
"nanokeyer".

## Status LCD

A 16x2 character LCD on a PCF8574 I2C backpack (address `0x27`) is wired to
the Nano's hardware I2C pins (A4/SDA, A5/SCL) and shows the board's current
state:

- Line 1, columns 1-6: the configured [callsign](#configuration-menu) (or
  `CALL` if none is set). Column 7 is a blank separator.
- Line 1, columns 8-16 (9 columns): current PTT source, with detail —
  - TNC-sourced (this board's own serial/Baudot engine, the `[`/`]`/`\`
    commands): `TNC ` + 2-digit baud rate + `b ` + FSK polarity, e.g.
    `TNC 45b H` (45.45 baud, mark = HIGH) — fills all 9 columns exactly.
  - RTS-sourced ([`PTT_USB_RTS_PIN`](#alternate-ptt-source-ptt_usb_rts_pin),
    an external TNC's RTS line): `RTS DIGI` — baud/polarity are this
    board's own internal FSK generator settings and don't apply to an
    external TNC, so a generic `DIGI` label is shown instead. Column 16 is
    blank (the string is 8 chars).
- Line 2: `RX`, `TX`, or `INHIBITED`

Requires the `marcoschwartz/LiquidCrystal_I2C` library (declared in
[platformio.ini](platformio.ini) `lib_deps`, installed automatically by
`pio run`). Exact wording/layout beyond these fields is still a placeholder
and easy to change in `lcdShowStatus()` in
[src/TinyFSK_ZAW_01.cpp](src/TinyFSK_ZAW_01.cpp).

## PTT/PA Sequencer

Many amplifiers need their PA relay/stage keyed *before* the transceiver PTT,
and released *after* it — otherwise the amp gets hot-switched under RF and
can be damaged. This firmware drives two relay outputs in a nested sequence
instead of a single PTT line:

```
Key down (TX_ON):
  PTT_PA_PIN HIGH ──► wait ptt_PA_LeadMillis ──► PTT_PIN HIGH ──► wait pttLeadMillis ──► first FSK bit sent

Key up (end of buffer / TX_END):
  last FSK bit sent ──► PTT_PA_PIN LOW ──► wait ptt_PA_TailMillis ──► PTT_PIN LOW ──► wait pttTailMillis ──► ready for next TX
```

In other words:
- **PA lead** (`l`) — PA relay closes, then this many ms later the main PTT closes.
- **PTT lead** (`L`) — main PTT closes, then this many ms later FSK data starts. Total time from key-down to first bit is `ptt_PA_LeadMillis + pttLeadMillis`.
- **PTT tail** (`T`) — after the last bit, wait this many ms, then drop main PTT.
- **PA tail** (`t`) — after dropping main PTT, wait this many ms, then drop the PA relay.

This guarantees the PA relay is always closed for the entire time the main
PTT (and therefore RF) is active, with configurable settle time on each side.
Implemented in `setPTT()` in [src/TinyFSK_ZAW_01.cpp](src/TinyFSK_ZAW_01.cpp).

Default timing: PTT lead 150 ms, PTT tail 25 ms, PA lead 80 ms, PA tail 80 ms.
All four values are independently adjustable and persisted to EEPROM (see
below), so they survive power cycles.

### Alternate PTT source: `PTT_USB_RTS_PIN`

Normally the sequencer is triggered by the `[` / `]` / `\` characters sent
over this board's own serial port (see [TX Control Characters](#tx-control-characters)),
which also drives the internal FSK/Baudot generator. If you instead use a
separate, external TNC that does its own FSK/AFSK generation — so this
board's serial data path and `FSK_PIN` aren't part of the signal chain —
that external TNC still needs amp-safe PTT sequencing.

`PTT_USB_RTS_PIN` (Arduino D2 / ATmega328 pin 32, PD2) exists for exactly
this. Wire the RTS output of the external TNC's USB-serial adapter to this
pin (with an external pull-up resistor to keep the line HIGH/idle when not
asserted). When the logging/TNC software asserts RTS, the pin is pulled
LOW, which the firmware polls every pass through `loop()` and treats exactly
like a `[` (key-down) — running the same PA-lead → PTT-lead sequence. When
RTS is released (pin goes HIGH again), the firmware treats it like a `]`
(buffered key-up), running the PTT-tail → PA-tail sequence before dropping
out.

This input is ignored while the `~` configuration menu is open, and does not
interact with the Baudot send buffer — it only drives the PA/PTT relay
sequencing, so it is not affected by anything queued for the internal FSK
generator.

### Hardware inhibit: `CPU_INH_PIN`

`CPU_INH_PIN` (Arduino D3 / ATmega328 pin 1, PD3) is a hard interlock: while
it's held LOW, the board refuses to key `PTT_PIN`/`PTT_PA_PIN` or clock any
data out `FSK_PIN`, no matter what asks it to — the `[` command, `PTT_USB_RTS_PIN`,
anything. Wire an external open-drain/relay-contact fault signal (e.g. SWR
protection, band-data lockout) to this pin; it uses the ATmega328's internal
pull-up, so an unconnected pin defaults to "not inhibited."

- If a key-up is requested while already inhibited, it's simply refused.
- If the inhibit line asserts **during** a transmission, the board force-stops
  immediately — bypassing the configured PTT/PA tail delays, since this is a
  safety cutoff rather than a normal end of TX — and clears the send buffer.
- Either way, the PC is told via a distinct status line: `inh:` (in addition
  to the usual `cmd:` line once back in RX). Watch for it if you're
  integrating with logging software, to distinguish "inhibited" from a
  normal end of transmission.

## Building & Uploading

This is a PlatformIO project (see [platformio.ini](platformio.ini)). The
`upload_port` is machine-specific — update it to match the serial device your
board enumerates as on your system before running `pio run -t upload`.

## Configuration Menu

Send `~` over the serial connection (9600 baud, 8-N-1) to enter the
configuration menu, then one of:

| Command | Effect                                  |
|---------|------------------------------------------|
| `0`     | FSK polarity: mark = logical HIGH        |
| `1`     | FSK polarity: mark = logical LOW         |
| `4`     | Set speed to 45.45 baud                  |
| `5`     | Set speed to 50.0 baud                   |
| `7`     | Set speed to 75.0 baud                   |
| `?`     | Show current configuration               |
| `L`     | Set/get PTT lead time, ms (before first bit) |
| `T`     | Set/get PTT tail time, ms (after last bit)   |
| `l`     | Set/get PA lead time, ms (before PTT)        |
| `t`     | Set/get PA tail time, ms (after PTT)         |
| `C`     | Set/get operator callsign (max 6 chars)      |

Speed and polarity apply immediately on keypress. `L`/`T`/`l`/`t`/`C` need a
value typed in next — see below.

### Browser configuration tool

[tools/config-tool.html](tools/config-tool.html) is a self-contained page
that talks to the board over the [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API) —
no install needed. Open it directly in Chrome, Edge, or Opera (double-click
the file, or drag it into the browser), click **Connect**, and pick the
board's serial port. It has controls for the PTT/PA timing values, speed,
polarity, and a raw-command box for anything not yet covered. Firefox and
Safari don't support Web Serial, so it won't work there.

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
- Set the callsign to OK2ZAW: `~COK2ZAW` then Enter (letters are
  auto-uppercased; longer than 6 characters is truncated)
- Check the current PTT tail without changing it: `~T` then Enter
- Start typing a value but cancel instead of committing it: `~L` then `~`

Each set/get prints the full current configuration afterward so you can
confirm the change.

## TX Control Characters

Sent outside the configuration menu, these control the transmitter directly
(as used by N1MM+'s `{TX}` / `{END}` / `{ESC}` macros):

| Character   | Effect                                                        |
|-------------|-----------------------------------------------------------------|
| `[`         | Key up now (`{TX}` in N1MM) — starts the PTT/PA lead sequence   |
| `]`         | Buffered key-down: finish sending buffered text, then unkey (`{END}` in N1MM) |
| `\` (backslash) | Immediate abort: unkey now and clear the send buffer (`{ESC}` in N1MM) |

Any other byte received while not in the configuration menu and not one of
the above is appended to the send buffer as text to be transmitted in Baudot
(ITA2).
