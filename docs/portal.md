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
  Gaming > Xbox Game Bar if it gets in the way. Gameplay buttons (A, B, X, Y, the shoulders, the
  d-pad) are never held back, so a chord that uses one (`back+y`) adds no lag to it.
- **With the game:** opened with a button, the menu lifts the figure at once but stays hidden
  while the game plays its own figure-leaving animation and zooms the camera out to the portal
  (`CMagicMoment` state 3, about 0.8 s, measured 2026-10-19); 0.6 s into that it fades in, just
  before the game's "place your Skylander" text (0.9 s after the button in the test, the text at
  about 1.2 s). Without a zoom-out it comes up once the game is clearly not busy with the figure,
  at the latest after `portal_menu_open_delay_ms` (2500).
- **Per player:** the menu opens for the controller that pressed the button (controller in
  connection slot N = player N+1) and only changes that player's figure. The keyboard plays as
  `keyboard_player` (default 1; 2 tries co-op with one controller), F6 opens the menu for that
  player, and the keyboard only steers that player's menu. Opening lifts the figure off the
  emulated portal, so the game reacts like with a real portal and shows its portal screen for that
  player ("Player 2: place your Skylander", in co-op with "B leave the game"). A second player who
  hasn't joined yet can open the menu too (the game doesn't poll that controller, so the wrapper
  checks it on every poll of the others) and joins by placing a figure.
- **Leaving:** Start > "Leave the game" (player 2 and up) closes the menu with the figure off and
  presses the B of the game's own portal screen for that player (`menu_input::TapButton`), so the
  game lets that player go; verified with the test script as player 2 (2026-10-19). Tested with an Xbox 360 controller as player 1 and a
  Dual Action as player 2, and with the test script's `player 2` command.
- **Opens by itself:** when the game shows its "Player N: place your Skylander on the Portal of
  Power" prompt (player 1 at the start or after losing the figure, player 2 right after pressing
  A to join), the menu opens for that player, steered by their controller(s); if the game stops
  waiting first, it closes again, and after B it stays closed until the next prompt. Off with
  `portal_menu_auto_open = false`. The app reads the game's `CMagicMoment` object for this
  (`src/game/magic_moment_watch.cpp`: instance pointer at `0x83131BE0`, state +0x20 = 5 while
  the prompt is up, player +0x148), found statically in Ghidra through the `MM_pleaseplaceText`
  UI string and the class's reflection tables; the object lives in physical memory.
- **All controllers of a player** steer that player's menu, not only the one that opened it.
- **One collection:** all players share the same figures (level, gold and upgrades live in each
  figure file). A character another player has on the portal is shown as "Taken · P1" in that
  player's colour and can't be chosen; that also goes for recent entries and swapper halves. Only
  the "Recent" list is per player.
- **Layout:** "Recent" opens first (each player's own list, kept across sessions, in
  `<figures>/portal_menu_recent_pN.txt`; a swapper combination is one entry and each half only
  appears in its latest combination). Then one tab per game with every character that has a
  version from that game (#49: Drobot is in Spyro's Adventure with Series 1 and in Giants with
  Series 2 and LightCore), one row per element, one card per character; last the Items tab with
  one row per game. In a game tab the card stands for that game's version: the one played last,
  else the first, with its badge under the name (S1, S2, LightCore, Giant, Sidekick, Variant), and
  A lists that game's versions first. Within an element come characters with an owned version from
  that game first, then the ones new in that game, then the returning ones.
- **Options (Start, O on the keyboard):** which games are listed (`portal_menu_hidden_games`, the
  last one can't be hidden) and "only my figures" (`portal_menu_owned_only`, only versions with a
  figure file). The menu writes just these lines into the settings file and leaves the rest of it
  as it is.
- **Controls:** D-pad / stick or arrows / WASD to move, A / Enter to choose, B / Esc to go back or
  close (the old figure goes back on the portal), LT / RT or Q / E to switch the category tab, LB /
  RB or 1 / 3 to switch the row (element, or game in the Items tab), X or L for details, Start or
  O for options. A on a character opens its versions (Series 1-4, LightCore, variants, chase
  variants, Eon's Elite).
- **Swapper builder:** A on a swapper opens two lists side by side, tops and bottoms, starting
  with the chosen swapper whole. Up / down picks a half, left / right (or LB / RB) switches the
  list, the preview shows the combination and its name (top's first word + bottom's last: Blast
  Zone + Fire Kraken = "Blast Kraken"), A puts both halves on.
- **Items:** magic items and adventure packs of the games in `portal_menu_games`. There is one
  item spot, shared by all players: A puts an item on without taking the player's Skylander off
  (it stays on the portal in its own slot) and replaces the item that was on; any player takes it
  off with Y (P on the keyboard) from any tab, or with A on the item marked "On the portal".
  The item changes only after the player's Skylander is back in the game (the menu puts the
  figure back first, then waits until the game's figure sequence is over, at least 0.7 s, at most
  10 s): changing both at once made the game ask for a Skylander again. Without the menu, Back + Y
  together (`portal_item_off_buttons`) or F5 (`bind_portal_item_off`) take the item off at once,
  without touching anyone's Skylander, and say so at the top of the screen.
- **Figures:** all catalog figures of the games in `portal_menu_games` (Swap Force: Spyro's
  Adventure, Giants, Swap Force), plus figures from later games that carry the id of a character
  those games know: the game goes by the id, so Eon's Elite Chop Chop (id 30) and Tidal Wave Gill
  Grunt (Series 4) are listed under their character, and Trap Team minis of the old sidekicks
  too. SuperChargers drivers have their own ids and stay out, like characters new in a later
  game. The owner's figure files under `portal_figures_dir` are used when present (level, gold,
  nickname shown); a figure without a file gets one when it is placed and keeps its progress from
  then on.
- **Pictures and names from the game's own data (#46):** every card shows the figure's toy
  collection icon, the same art as the game's collection screen, for every Skylander, variant,
  swapper and magic item; magic items and adventure packs get the names the game shows in its
  language (`Ambossregen` in German). The app reads them at runtime from the owner's game files
  (`src/game/toy_images.cpp`): the game's toy type table in the loaded executable (its values
  are the catalog ids), `toydata.pak` (toy -> collection material per variant, names in English
  and German) and `toycollectionimages.pak` (material -> 192x192 DXT5 texture). Sidekicks name no
  material in their toy data (`CPetToyData`); their icons go by the character name
  (`Pet_Terrabite` -> `collection_pet_terrabite_normal`, the Giants sidekicks with "lil":
  `collection_pet_liltreerex_normal` for Barkley). Only UFO Hat has no icon. Readers for the
  Alchemy formats are in `src/game/alchemy/` (IGA archives with LZMA blocks, IGZ textures, IGX
  text). Indexing takes about 150 ms in the background right after start-up; pictures are decoded
  when they first show. Nothing of the game's data goes into the repository. The menu
  library only sees a picture/name provider (`SetFigureImageProvider`, `SetFigureNameProvider`).
- **Look:** collection tiles (picture on top, name below), the preview figure standing on a drawn
  Portal of Power (dark base, silver rim, glowing top, rising light), self-drawn element symbols (the
  official ones can't be used) as medallions in the element row and band; slightly see-through
  panel with the game visible around it; German texts with `language = "german"`. The details
  view will show more of the figure data (#48).
- The whole input system is the SDK's default one with every driver wrapped
  (`CreatePortalMenuInputSystem`): the wrapper notices the open button, hides all controller and
  keyboard input from the game while the menu is open, and lets the menu poll each controller.
  The Guide button never reaches the game. Its device assignment is the SDK's slot assignment
  plus `keyboard_player`, and the player menu below can move any device to another player.

## Player menu (L3 + R3 / F9)

`portal_rex/player_menu.cpp`: which controller plays as which player. L3 + R3 together on any
controller (`player_menu_buttons`, default `ls+rs`) or F9 (`bind_player_menu`) open it; four columns,
one per player, with the controllers (and the keyboard) that play as that player. Every controller
moves itself with left / right, all at the same time, the keyboard with the arrow keys; Y (R on the
keyboard) puts every device back in connection order, A / B / Start / Enter / Esc close it. The
game sees no input while it is open. Meant for a Bluetooth controller that dropped out and came back
in another slot, or to swap who is player 1. The assignment lasts for the session; controllers get
new ids when they reconnect. Moved devices have a gold frame.

## Developer tools (F8)

`portal_rex/portal_overlay_dialog.cpp`, the former F6 overlay: **Browse** lists dumps found under
`portal_figures_dir` and places or removes them in any slot 0–15. **New Figure** creates a blank
figure from the catalog and places it. With the USB portal it shows what is on the real portal and
can dump a figure to a file.
