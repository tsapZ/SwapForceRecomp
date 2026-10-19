# Hybrid portal: the real USB portal and the portal menu together

Status: built 2026-10-19 (`portal_mode = "hybrid"`, settings menu > Portal > "Both"), unit-tested
(`skylanders_real_portal_bridge_test`) and run in the game without a portal plugged in; **not yet
tried with a real portal** (none here). Owner's request: "USB portal + portal in game, for the
figures you don't own".

## How it works

The game talks only to the emulated portal, as in `software` mode, so the portal menu, the item
and sidekick rules and the portal check keep working. Next to it a bridge thread drives the real
USB portal itself (`portal/hybrid/real_portal_bridge.cpp`, glue in `portal_rex.cpp`): it watches
which figures stand on it, reads each new one completely, puts a copy into a free slot of the
emulated portal (from the top slot down; the menu fills from the bottom), and writes every change
the game makes to that copy back onto the real figure. Lifting a figure off the real portal takes
the copy off.

```
 game ── XAM ── SoftwarePortal (16 slots) ──┬── menu figures (files in figures/)
                                            └── copies of real figures ◄──► bridge ◄──► USB portal
```

## Cases

| # | What happens | What the bridge and the menu do |
|---|---|---|
| 1 | A figure is put on the real portal | all 64 blocks are read; its 1 KB is saved to `figures/real/<name> <UID>.dump`, which is also the copy's source; the copy goes into a free emulated slot. A Skylander belongs to the player whose portal menu is open (the game waits for that player; the menu closes without putting the lifted figure back), else for a swapper half to the player with the other half, else to the first player without a figure |
| 2 | Reading fails (figure moved, half on the portal) | each block is asked for up to 3 times; then the figure is left out (logged) until it is lifted and put on again |
| 3 | The game writes to a copy (XP, gold, about every 30 s) | the copy changes at once (the game reads back what it wrote) and is saved to the backup file; the changed blocks go to the real figure in order, each confirmed by the portal's reply. Block 0, block 1 and the sector trailers (every fourth block: the tag's keys) are never written |
| 4 | Writing to the real figure fails | retried up to 3 times; then it is logged and the block is in the backup file |
| 5 | The figure is lifted off the real portal | the copy leaves the emulated portal; writes still queued are dropped (they are in the backup); the game reacts as to a lifted figure |
| 6 | A figure file with the UID of a figure on the real portal is chosen in the menu | refused: "This figure is on the real portal." Two copies of one figure would overwrite each other's progress |
| 7 | A sidekick or an item stands on the real portal and another is brought from the menu | refused: "A sidekick / an item is on the real portal: lift it off there first." (the game takes one of each) |
| 8 | Swapper on the real portal | top and bottom are two figures, both get copies, the game puts them together as on the console |
| 9 | Too many figures anyway | the game's error screen opens the portal check; figures from the real portal are marked "on the real portal"; taking one off there takes only the copy off (lift the real one afterwards) |
| 10 | The USB portal is unplugged | like lifting every real figure (case 5); the bridge looks for the portal once a second and starts over when it is back |
| 11 | Figures already on the real portal at start-up | read like case 1 as soon as the bridge has started |
| 12 | Portal light | the game's colour commands go to the emulated portal and, at most ten times a second, to the real one, so it glows as on the console |
| 13 | Portal speaker (Swap Force's portal has none) | PC speakers, as in `software` mode |
| 14 | The portal menu lifts a player's copy (opening the menu) | the copy is followed by its source file: put back, its writes go to the real figure again; replaced by a menu figure, the real one stays unused until it is lifted and put on again |
| 15 | A real figure's backup is played from the menu while the figure is off the portal, then the figure is put on again | the backup file with the newer progress is kept as `... before.dump`, the real figure's own data becomes the backup; nothing is lost, but the menu progress is not copied onto the real figure |

## What is not covered

- Traps and creation crystals (later games, not Swap Force).
- The portal developer tools' figure dump (F8) are only offered in `usb` mode; in `hybrid` every
  real figure is backed up anyway.
- One figure on two PCs at once (see docs/online-coop.md for playing over the network).
