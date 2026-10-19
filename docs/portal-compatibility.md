# Portal compatibility

Which real Portal of Power works with which Skylanders game, on the consoles and with this PC port.
It also shows what each portal reports about itself, so a launcher can tell them apart.

Each statement has a basis tag:

- **[S]** a source says so (numbers point to the list at the end)
- **[T]** tested in this project
- **[I]** inference: it fits the sources, but nobody has checked it
- **[C]** the sources disagree

## Short answer

- For SwapForceRecomp, use a wired PS3, PS4, Wii, Wii U or PC portal. Any generation works.
  Swap Force or newer is best. [T][I]
- A Giants portal works. It has been tested with a PS3 Giants portal. [T]
- Spyro's Adventure portals should work the same way. Nobody has tested one yet. [I]
- Xbox 360 portals do not work yet. They are a different USB device. [S][I]
- Xbox One portals do not work. No emulator supports them either. [S]
- No driver install is needed. Do not use Zadig on the portal for this port. [T][I]

## 1. Portal generations

All non-Xbox portals use USB ID `1430:0150` and are plain USB HID devices. [S 4, 5, 6, 7, 12]
The Xbox 360 portals use `1430:1F17`. [S 6, 9, 10] The ID column is the reply to the `R` command
(section 2). [S 1]

| Portal | Connection | USB ID | `R` ID | Pieces | Lights | Speaker | Trap slot |
|---|---|---|---|---|---|---|---|
| Spyro's Adventure, Wii/PS3 | wireless, AA batteries, USB dongle | `1430:0150` (the dongle) [S 13] | `01 29` | 4 [S 4, 8] | yes [S 3] | no | no |
| Spyro's Adventure, Xbox 360 | wired | `1430:1F17` [S 6] | `01 28` | 4 | yes | no | no |
| Spyro's Adventure, PC/Mac | wired | `1430:0150` [I, see note 1] | `01 2C` | 4 | yes | no | no |
| Spyro's Adventure / Giants, 3DS | wireless; has a USB port | `1430:0150` [I] | `01 34` | 4 | ? | no | no |
| Giants (PS3/Wii) | wired | `1430:0150` [T] | `01 3D` (also `01 3C`) | 4 [S 3] | yes | no | no |
| Giants, Xbox 360 | wired | `1430:1F17` [I] | `01 48` | 4 | yes | no | no |
| Swap Force (PS3/Wii/Wii U/PS4) | wired | `1430:0150` | `02 03` | more than 4 [S 3] | yes [S 3] | no | no |
| Swap Force, Xbox 360 | wired | `1430:1F17` [I] | `02 03` | more than 4 | yes | no | no |
| Swap Force, 3DS | no USB port | – | `02 02` | – | – | – | – |
| Trap Team "Traptanium" | wired | `1430:0150` [S 12] | `02 18` (also `02 1B`) | more than 4 | colour rims, left/right [S 14] | yes [S 14] | yes, with a white light [S 4] |
| Trap Team, Xbox 360 | wired | `1430:1F17` [I] | `02 1B` | more than 4 | yes | yes [S 10] | yes |
| Trap Team, Xbox One | wired | unknown | `02 27` | | | yes | yes |
| Trap Team, mobile | Bluetooth LE | – | `02 19` | | | no [S 14] | yes |
| SuperChargers | wired | `1430:0150` | `02 0A 03 02` | more than 4 | no built-in lights [S 3] | no [S 3] | no [C, see note 2] |
| SuperChargers, Xbox 360 | wired | `1430:1F17` [I] | `02 0A 03 02` | | no | no | no |
| SuperChargers, Xbox One | wired | unknown | `02 14 03 02` | | no | no | no |
| Imaginators | wired | `1430:0150` | `02 0A 05 08` | | ? | ? | covered, see below |
| Imaginators, Xbox 360 (Swap Force shape, grey cord) | wired | `1430:1F17` [I] | `02 0A 05 08` | | | | no |

All `R` IDs come from one community table, built by people who own these portals. [S 1]

Notes:

1. The Spyro's Adventure PC game itself only opens `1430:0150` over HID. [S 12] So the PC portal is
   almost certainly `0150`. One older write-up says PC portals are `1F17`. [S 6] That does not fit
   the game code. [C]
2. One RPCS3 comment says the SuperChargers portal has a trap slot. [S 15] The fan wiki says no,
   and says it does not work with Trap Team. [S 3] [C]
3. Imaginators came in two shapes. In some regions it is a SuperChargers-style portal, with a panel
   over a trap slot. The trap parts are still inside. In other regions it is a Swap Force portal with
   a grey cord. [S 3]
4. The Wii wireless portal sometimes answers `R` with `90 00`. The table author thinks this is a bug.
   [S 1]
5. Every model inside one generation reports a different ID. A launcher can therefore name the exact
   model. [S 1]

## 2. What the game checks: the `R` reply

The game sends `R` first. The portal answers `52` plus 2 ID bytes (4 bytes on SuperChargers and
Imaginators). [S 1, 2, 11] The game uses this ID to decide if the portal is supported. Unsupported
IDs give a warning. [S 11] Trap Team uses it to reject non-Traptanium portals. [S 1, 15]

- The first ID byte is `01` for Spyro's Adventure, Giants and Battlegrounds. It is `02` for Swap
  Force and everything after. [S 1]
- Swap Force on Xbox 360 rejects `01 3D` (a PS3 Giants portal) with "Wrong Portal version". [T]
- It accepts `52 02 1B` (an Xbox 360 Traptanium ID). [T]
- Activision says portals are backward compatible. It only warns about swappers on older portals.
  [S 8] No source says outright that an Xbox 360 Giants portal works in Swap Force on an Xbox 360.
  If it does, the game must accept `01 48` (and maybe `01 28`). Then it checks a list of known
  Xbox 360 IDs and does not reject every `01` portal. [I] One forum answer says the Spyro's
  Adventure portal does not work with Swap Force at all. [C, S 16]

Emulated portals answer with these IDs (good references):

| Emulator | Emulated `R` reply | Which real portal that is |
|---|---|---|
| Dolphin (Wii) | `52 02 1B` [S 2] | Traptanium (alternate, and Xbox 360) |
| Cemu (Wii U) | `52 02 1B` [S 5] | same |
| shadPS4 (PS4) | `52 02 1B` (same code as Dolphin) [S 17] | same |
| RPCS3 (PS3), since April 2021 | `52 02 18` [S 7] | Traptanium |
| RPCS3 before that | `52 02 0A 03 02` [S 7] | SuperChargers. Trap Team got stuck on "use the Traptanium Portal". [S 15] |
| SwapForceRecomp / GiantsRecomp emulated portal | `52 02 1B` [T] | Traptanium |
| Xenia (Xbox 360) | no emulated portal; real portal only [S 9] | – |

## 3. Which portal works with which game

Official statements and community reports:

| Game | Works with | Basis |
|---|---|---|
| Spyro's Adventure, Giants | any portal of the same console family | [S 3] |
| Swap Force | Swap Force and newer portals of the same console family [S 3]. Older portals "might" have trouble with swappers [S 8]. One forum answer says the Spyro's Adventure portal does not work [S 16]. | [S][C] |
| Trap Team | Traptanium portal only. Spyro's Adventure, Giants and Swap Force portals do not work at all, not just the traps. | [S 15, 18] |
| SuperChargers | Giants, Swap Force, Trap Team portals. Activision says the Spyro's Adventure portal does **not** work. The wiki says it does, with 4 pieces. | [S 19] vs [S 3], [C] |
| Imaginators | all older portals. Traps need a portal with a trap slot. | [S 8] |
| Imaginators portal in older games | Activision says it works with every earlier game. The RPCS3 report says Trap Team rejects it. | [S 8] vs [S 15], [C] |

Console families (real consoles):

- Xbox 360 portals only work on an Xbox 360. [S 3, 20]
- Xbox One portals work on Xbox One and Series X/S. [S 3]
- PS3, PS4, Wii, Wii U and PC portals can be swapped freely. [S 3]
- The Wii and PS3 wireless portals are identical. Either dongle works with either portal. [S 3]
- Mobile Bluetooth Traptanium portals only work on mobile devices. [S 3]
- Without a trap slot, a portal can still read a trap held near the ring. This is not practical.
  [S 3]

## 4. In SwapForceRecomp

How the fork handles real portals (`thirdparty/skylanders-portal`, branch
`swapforce/old-portal-support`, file `src/portal/usb/usb_portal.cpp`):

- It opens the portal through hidapi with the stock Windows HID driver. [T]
- Commands go out as HID SET_REPORT. Speaker audio goes out on the interrupt OUT endpoint. [T]
- Any `52 01 xx` reply is rewritten to `52 02 1B`. Replies from newer portals pass unchanged. [T]
- It reads every queued HID report, because the portal sends status reports faster than the game
  reads them. Without that, the game reset the portal about every 20 s. [T]
- It looks for `1430:0150` first, then `1430:1F17`. [T]

What that means per portal:

| Portal | Status in this port |
|---|---|
| Giants (PS3, `01 3D`) | works, tested [T] |
| Spyro's Adventure wired PC, 3DS (with cable) | should work through the ID rewrite [I] |
| Spyro's Adventure wireless (Wii/PS3 dongle) | should work through the ID rewrite. It may drop answers. Only 4 pieces. [I, S 6] |
| Swap Force, Traptanium, SuperChargers, Imaginators (non-Xbox) | should work without the rewrite [I]. The upstream library was tested with a Wii U Traptanium portal in Giants. [S 21] |
| Any Xbox 360 portal (`1F17`) | most likely not found at all, see below [I] |
| Xbox One portals | not supported [S 9] |

Swappers: one swapper uses two pieces. Two players with swappers plus a magic item need 5 pieces. Spyro's
Adventure and Giants portals only read 4. [S 3, 4] Use a Swap Force or newer portal for that. [I]

The ID rewrite does not add hardware. A Giants portal still has no speaker, no trap slot and only 4
pieces. Swap Force never sent the speaker command (`M`) in our traces, so this does not matter here.
[T]

## 5. Practical notes for PC

- **Normal portals (PS3, PS4, Wii, Wii U, PC):** Windows loads its own HID driver. Nothing to install.
  [T, S 21]
- **Zadig:** Emulators (Dolphin, RPCS3, Cemu with libusb) often need a WinUSB or libusbK driver from
  Zadig. [S 22, 23] After that switch, the portal is no longer an HID device. This port will then
  not find it. Undo it in Device Manager: uninstall the device and delete the driver, then plug it in
  again. [I]
- **Wireless Spyro's Adventure portals:** plug in the dongle, not the portal. Put in fresh batteries.
  Wireless portals answer the activate command `A` differently (third byte counts down, fourth is
  `00`; wired ones answer `FF 77`). [S 2, 4] A launcher can use this to spot a wireless portal. [I]
- **Xbox 360 portals:** they are not plain HID. Xenia needs a WinUSB driver from Zadig for them.
  [S 9, 10] Cemu talks to them over libusb. [S 5] They expect a `0B 14` header in front of each
  command and send audio as G.721 with a `0B 17` header. [S 5, 6, 10] They have a security chip for
  the Xbox 360. [S 6] hidapi (used by this port) only sees HID devices. So the `1F17` entry in the
  fork most likely never opens anything. [I] Good news: the game already sends `0B 14` frames. A
  small libusb backend could pass them through unchanged, like Xenia does. [I]
- **Xbox One portals:** they show up as "PORTAL" in Zadig and work very differently. Nobody supports
  them. [S 10]
- **Linux:** the Traptanium portal shows in `lsusb`, but some games did not see it through Proton.
  [S 24] Not tested with this port.

## 6. For a future launcher

- Read the `R` reply once. Look it up in the table in section 1 to show the exact model. [S 1]
- Look at the USB ID first. `0150` means HID, so use hidapi. `1F17` means Xbox 360, so use libusb with
  WinUSB. [S 5, 9] [I]
- Keep a per-game list of IDs to rewrite. Swap Force on Xbox 360: `01 xx` to `02 1B`. [T]
- Trap Team needs a Traptanium ID (`02 18`, `02 19`, `02 1B` or `02 27`) and a real trap slot.
  Rewriting the ID cannot add a trap slot or speaker. [S 1, 15] [I]
- Warn when a 4-piece portal is used for co-op with swappers. [S 3] [I]

## 7. Open questions

- Which IDs does Swap Force on Xbox 360 accept? Find the "Wrong Portal version" check in Ghidra.
- Test a Spyro's Adventure wireless portal (`01 29`) and a Swap Force portal (`02 03`).
- Get the USB descriptor of an Xbox 360 portal (`lsusb -v` or USBView) to confirm it is not HID.
- Do all Xbox 360 portal generations use `1430:1F17`? Xenia and Cemu only list that one.

## Sources

1. Texthead1, "Skylanders Portal IDs" (table of `R` IDs per model with FCC IDs, 2025):
   https://github.com/Texthead1/Skylanders-Portal-IDs (mirror:
   https://github.com/skylandersNFC/Docs/tree/main/Skylanders_IDs/Skylanders-Portal-IDs)
2. Dolphin, emulated portal source:
   https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/IOS/USB/Emulated/Skylanders/Skylander.cpp
3. Skylanders Wiki, "Portal of Power": https://skylanderswiki.com/wiki/Portal_of_Power
4. tresni, PoweredPortals wiki, "USB Protocols": https://github.com/tresni/PoweredPortals/wiki/USB-Protocols
5. Cemu, nsyshid source (`Skylander.cpp`, `Whitelist.cpp`) and PR #1550 "Add Skylander Xbox 360
   Portal support" (May 2025): https://github.com/cemu-project/Cemu/tree/main/src/Cafe/OS/libs/nsyshid ,
   https://github.com/cemu-project/Cemu/pull/1550
6. "The Skylanders Portal Demystified": https://skylandersnfc.github.io/Docs/Skylanders_Portal_Demystified/
   (same text: https://gist.github.com/parkerlreed/a19d50deccaedbe15517a19bc70ff2e5)
7. RPCS3, `Skylander.cpp` and PR #10061 "Switch emulation to the Traptanium portal" (April 2021):
   https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Io/Skylander.cpp ,
   https://github.com/RPCS3/rpcs3/pull/10061 ; passthrough list in
   https://github.com/RPCS3/rpcs3/blob/master/rpcs3/Emu/Cell/lv2/sys_usbd.cpp
8. Activision, "Skylanders Imaginators Portal of Power FAQ":
   https://support.activision.com/skylanders-imaginators/articles/skylanders-imaginators-portal-of-power-faq
9. Xenia Canary, `src/xenia/hid/portal/hardware_portal.{h,cc}` and commit 7298536d46 "[HID] Added
   support for Xbox 360 Skylanders Portal - This requires Zadig for installation of WinUSB driver"
   (April 2025): https://github.com/xenia-canary/xenia-canary/tree/canary_experimental/src/xenia/hid/portal
10. Xenia Canary PR #581 discussion: https://github.com/xenia-canary/xenia-canary/pull/581
11. mandar1jn (Marijn Kneppers), "Reverse engineering Skylanders toys-to-life mechanics":
    https://marijnkneppers.dev/posts/reverse-engineering-skylanders-toys-to-life-mechanics/
12. ssa-improved (Spyro's Adventure PC mod), `src/portal/*`: https://github.com/j4ceee/ssa-improved
13. USB ID database, vendor 1430: https://usb-ids.gowdy.us/read/UD/1430
14. Skylanders Wiki, "Traptanium Portal": https://skylanderswiki.com/wiki/Traptanium_Portal
15. RPCS3 issue #9449 "Trap Team expects Traptanium Portal": https://github.com/RPCS3/rpcs3/issues/9449
16. SuperCheats user answer (weak source):
    https://www.supercheats.com/xbox360/questions/skylandersspyrosadventure/325995/will-one-portal-work-all-three.htm
17. shadPS4, emulated portal: https://github.com/shadps4-emu/shadPS4/blob/main/src/core/libraries/usbd/emulated/skylander.cpp
18. Game Informer, "Eleven things you need to know about Skylanders Trap Team" (2014):
    https://gameinformer.com/b/features/archive/2014/04/23/eleven-things-you-need-to-know-about-skylanders-trap-team
19. Activision, SuperChargers "Portal of Power and hardware issues" FAQ (Spanish page):
    https://support.activision.com/es/skylanders-superchargers/articles/skylanders-superchargers-portal-of-power-and-hardware-issues-faq
20. Wikipedia, "Skylanders: Spyro's Adventure": https://en.wikipedia.org/wiki/Skylanders:_Spyro%27s_Adventure
21. GiantsRecomp README ("Tested with a Wii U Traptanium portal", no driver changes):
    https://github.com/TheBiemGamer/GiantsRecomp
22. Dolphin forum, Xbox 360 portal with Swap Force (2017; "Xbox 360 portal is different than the
    PS3/Wii portals and will not work in Dolphin"):
    https://forums.dolphin-emu.org/Thread-portal-of-power-not-detected-on-skylanders-swap-force
23. Dolphin bug #6665 (Zadig driver reports): https://bugs.dolphin-emu.org/issues/6665
24. Cemu issue #275 (Linux, Traptanium portal not seen): https://github.com/cemu-project/Cemu/issues/275

Local sources: `thirdparty/skylanders-portal/src/portal/usb/usb_portal.{h,cpp}`,
`src/portal/software/software_portal.cpp`, `docs/portal.md`, `docs/STATUS.md`, issue #47.
