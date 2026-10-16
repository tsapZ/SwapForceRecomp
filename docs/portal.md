# Portal of Power

How the game talks to the portal, how figures are stored, and what is special about Swap Force's
swappers. The implementation lives in `thirdparty/skylanders-portal` (paths below are relative to
its `src/`). Protocol details were measured on Giants; for Swap Force they are **unverified** until
Phase 3.

## Backends

`portal_mode` selects one, at startup:

| Mode | Backend | What it does |
|---|---|---|
| `software` | `portal::SoftwarePortal` | emulated portal with up to 16 figure slots, driven by the F6 overlay |
| `usb` | `portal::UsbPortal` | a real portal via hidapi; commands go out as HID `SET_REPORT`, no driver swap (Zadig) needed |
| `none` | – | no handler; the game shows its own "Can't find the Portal of Power" |

Both backends implement `portal::PortalDevice`. `portal::XamBridge` adds and strips the Xbox 360
frame header between the game and the device.

## Wire protocol

Every frame is 32 bytes. On the Xbox side that is the header `0B 14` plus 30 bytes of payload; the
raw device side has no header. The first payload byte is the command letter.

| Command | Payload | Meaning | Reply |
|---|---|---|---|
| `R` | `52` | ready poll, about every 300 ms until answered | `52 02 1B` |
| `A` | `41 <n>` | activate (`1`) / deactivate (`0`) | `41 <n> FF 77` |
| `S` | `53` | status poll | status frame |
| `C` | `43 R G B` | set LED colour | none |
| `Q` | `51 <slot> <block>` | read one 16-byte block | command byte + 16 data bytes |
| `W` | `57 <slot> <block> <16 bytes>` | write one 16-byte block | none |

A read with no queued reply returns a status frame:

```
0B 14 53 <4-byte little-endian slot states> <counter> <active>
```

Slot states use 2 bits per slot, slot 0 lowest: `0` empty, `1` present, `2` removing, `3` added. The
game polls continuously, whether a portal answers or not.

## Figures

- A figure is 64 blocks of 16 bytes (1024 bytes), like a real tag. Files are flat 1024-byte dumps
  (`portal/figure_file.*`), saved atomically (write to a temp file, then rename).
- `CreateBlankFigure(id, variant)` builds a valid empty figure: id and variant at `0x10`/`0x1C`,
  CRC-16/CCITT checksum, sector-trailer access bits. No existing dump needed.
- The game rejects invalid data with "A toy on the Portal of Power has a problem." A valid figure
  is played normally, including progress written back during play.
- Writes go back to the file the figure came from.
- Figure dumps are personal data and are git-ignored (`*.dump`, `*.sky`, `figures/**/*.bin`).

## Swappers (Swap Force)

Each swapper half is its own figure with its own id. The built-in catalog
(`portal/skylander_catalog_data.h`) already lists all 16 swappers: **bottom halves 1000–1015, top
halves 2000–2015**, game "Swap Force". `portal/figure_stats.cpp` already contains Swap Force's
level table.

Open for Phase 3:

- Both halves sit on the portal at the same time, in two slots. Does the software portal's slot
  handling and the overlay make placing and swapping a pair practical?
- When a half is swapped, the game sees one slot go `removing` and another `added`. Does the
  timing match what a real portal sends?
- Where does the game write progress for a combined character, to one half or both?

If anything needs changing, it goes into a clearly named branch of `skylanders-portal` so it can be
offered upstream.

## Overlay (F6)

`portal_rex/portal_overlay_dialog.cpp`, an ImGui dialog. **Browse** lists dumps found under
`portal_figures_dir` and places or removes them in slots 0–15. **New Figure** creates a blank figure
from the catalog and places it. It only uses the control API, so it works the same with every
backend.
