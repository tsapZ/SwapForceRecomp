# Portal of Power

How the game talks to the portal, how figures are stored, and what is special about Swap Force's
swappers. The implementation lives in `thirdparty/skylanders-portal` (paths below are relative to
its `src/`). Protocol details were measured on Giants; for Swap Force they are **unverified** until
Phase 3.

## Backends

`portal_mode` selects one, at startup:

| Mode | Backend | What it does |
|---|---|---|
| `software` | `portal::SoftwarePortal` | emulated portal with up to 16 figure slots, driven by the portal menu |
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

Both halves sit on the portal at the same time, in two slots, and the game combines them (tested
with Blast Zone + Fire Kraken and with whole swappers through the portal menu). Both halves get
their progress written back. Still to test: a swapper on the real USB portal (#42).

If anything needs changing, it goes into a clearly named branch of `skylanders-portal` so it can be
offered upstream.

## Portal menu (Guide button / F6)

`portal_rex/portal_menu.cpp` (input plumbing in `portal_menu_input.cpp`), in the
`feature/portal-menu` branch of the fork. A Skylanders-style ImGui menu that works with a
controller or the keyboard:

- **Open:** the Guide (Xbox) button, View/Back + Start together, or holding View/Back on a
  controller (`portal_menu_open_buttons`, default `guide,back+start,hold_back`; hold time
  `portal_menu_hold_ms`, chord window `portal_menu_chord_ms`), or F6 on the keyboard
  (`bind_portal_menu`). Buttons that may start an open entry are kept from the game until it's
  clear they don't open the menu: a short Back tap still reaches the game (as a short tap, a moment
  later), holding it or adding Start opens the menu without the game seeing Back. Without that,
  Back opened the game's own menu on the way (noticed with a Logitech Dual Action, whose button 9
  is Back). The Windows Game Bar also reacts to the Guide button: turn it off under Settings >
  Gaming > Xbox Game Bar if it gets in the way.
- **Per player:** the menu opens for the controller that pressed the button (controller in
  connection slot N = player N+1) and only changes that player's figure. The keyboard plays as
  `keyboard_player` (default 1; 2 tries co-op with one controller), F6 opens the menu for that
  player, and the keyboard only steers that player's menu. Opening lifts the figure off the
  emulated portal, so the game reacts like with a real portal: alone it pauses and asks for a
  Skylander, in co-op only that player drops out. A second player who hasn't joined yet can open
  the menu too (the game doesn't poll that controller, so the wrapper checks it on every poll of
  the others) and joins by placing a figure. Tested with an Xbox 360 controller as player 1 and a
  Dual Action as player 2, and with the test script's `player 2` command.
- **One collection:** all players share the same figures (level, gold and upgrades live in each
  figure file). A character another player has on the portal is shown as "Taken · P1" in that
  player's colour and can't be chosen; that also goes for recent entries and swapper halves. Only
  the "Recent" list is per player.
- **Layout:** "Recent" opens first (each player's own list, kept across sessions, in
  `<figures>/portal_menu_recent_pN.txt`; a swapper combination is one entry and each half only
  appears in its latest combination). Then one tab per game with the characters that debuted in
  it, one row per element, one card per character; last the Items tab with one row per game.
- **Controls:** D-pad / stick or arrows / WASD to move, A / Enter to choose, B / Esc to go back or
  close (the old figure goes back on the portal), LT / RT or Q / E to switch the category tab, LB /
  RB or 1 / 3 to switch the row (element, or game in the Items tab), X or L for details. A on a
  character opens its versions (Series 1-4, LightCore, variants, chase variants, Eon's Elite).
- **Swapper builder:** A on a swapper opens two lists side by side, tops and bottoms, starting
  with the chosen swapper whole. Up / down picks a half, left / right (or LB / RB) switches the
  list, the preview shows the combination and its name (top's first word + bottom's last: Blast
  Zone + Fire Kraken = "Blast Kraken"), A puts both halves on.
- **Items:** magic items and adventure packs of the games in `portal_menu_games`. There is one
  item spot, shared by all players: A puts an item on without taking the player's Skylander off
  (it stays on the portal in its own slot) and replaces the item that was on; any player takes it
  off with Y (P on the keyboard) from any tab, or with A on the item marked "On the portal".
- **Figures:** all catalog figures of the games in `portal_menu_games` (Swap Force: Spyro's
  Adventure, Giants, Swap Force), plus figures from later games that carry the id of a character
  those games know: the game goes by the id, so Eon's Elite Chop Chop (id 30) and Tidal Wave Gill
  Grunt (Series 4) are listed under their character, and Trap Team minis of the old sidekicks
  too. SuperChargers drivers have their own ids and stay out, like characters new in a later
  game. The owner's figure files under `portal_figures_dir` are used when present (level, gold,
  nickname shown); a figure without a file gets one when it is placed and keeps its progress from
  then on.
- **Look:** element colours and self-drawn element symbols (the official ones can't be used): a
  medallion with gold ring and shaded disc in the element row, preview and details, a faded
  watermark on each card; slightly see-through panel with the game visible around it; German
  texts with `language = "german"`. Portraits from the game data are planned (#46), the details
  view will show more of the figure data (#48).
- The whole input system is the SDK's default one with every driver wrapped
  (`CreatePortalMenuInputSystem`): the wrapper notices the open button, hides all controller and
  keyboard input from the game while the menu is open, and lets the menu poll each controller.
  The Guide button never reaches the game. Its device assignment is the SDK's slot assignment
  plus `keyboard_player`.
## Developer tools (F8)

`portal_rex/portal_overlay_dialog.cpp`, the former F6 overlay: **Browse** lists dumps found under
`portal_figures_dir` and places or removes them in any slot 0–15. **New Figure** creates a blank
figure from the catalog and places it. With the USB portal it shows what is on the real portal and
can dump a figure to a file.
