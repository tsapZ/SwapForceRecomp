# Skylanders: Swap Force (Xbox 360)

What we know about the game and the dump this project targets, and what still has to be
checked. Facts marked *unverified* come from other sources and have not been measured here.

## The game

- Third Skylanders game, released in 2013, published by Activision and developed by
  **Vicarious Visions**. Giants (2012) and Trap Team (2014) are by Toys for Bob.
- New with Swap Force: **swappers**, figures whose top and bottom halves come apart and combine
  with other swappers' halves. Each half is its own figure on the portal. Also new: jumping.

## Target dump

| Property | Value | Source |
|---|---|---|
| Release | Europe (English/German), XGD2 | size matches; region flags below |
| ISO size | 8,738,846,720 bytes | measured |
| Disc content | 2,474 files, 8,448,047,409 bytes | `extract-xiso`, measured |
| `default.xex` size | 18,563,072 bytes, in the disc root | measured |
| `default.xex` SHA-256 | `dc58d3851bbd89935e9ecf2ee84b158ec400f049da46c937a4713477a50c79d7` | measured, in `default.xex.sha256` |
| Title ID | `415608E9` (media ID `236350A9`) | XEX execution info |
| Version | `0.0.0.26` (base version the same), disc 1 of 1 | XEX execution info |
| Region flags | `0x00FF0000`: all PAL regions | XEX security info |

Only the `default.xex` from the disc is used. A title update from a console hard drive is a
different executable with different addresses and is not supported. The startup check accepts only
this exact file.

## Differences from Giants that matter

- **Different studio.** The plan assumes the same engine generation as Giants. That is plausible
  (same publisher, one year apart, same portal hardware) but not proven. If the code looks very
  different, report it and adjust the plan before investing in Giants-specific approaches.
- **Swappers.** The game has to handle two figures that together form one character. The portal
  catalog in `thirdparty/skylanders-portal` already lists all 16 swappers as separate top and bottom
  figures; whether the protocol side needs anything extra is a Phase 3 question.
- **Videos.** Swap Force plays Bink 2 videos. The earlier attempt below stalls on one.
- **Every address is new.** Nothing in GiantsRecomp's `config/default.toml` or hooks applies.

## Prior art

[swapforce-pc](https://github.com/AngelLopezTorres/swapforce-pc) (since 2026-10-11, in Spanish)
takes another route: XenonRecomp plus its own runtime instead of ReXGlue. According to its own
README it is not playable: the game starts and the main loop runs, but the picture stays black
because a Bink 2 video does not advance; 7–9 fps; no sound, input or portal. GitHub detects no
license, so it is **read-only for us: nothing is copied**. Its `analysis/` and `PROGRESS.md` are
useful as a list of stumbling blocks.

## Open questions

| Question | How to check | Answer |
|---|---|---|
| Exact version and region of `default.xex` | XEX header | `0.0.0.26`, PAL, title `415608E9` |
| Does the game import `XamInputNonControllerGetRaw/SetRaw` (1185/1186)? | Import table / Ghidra | |
| Which functions wrap the portal read/write calls? | Ghidra, callers of the imports | |
| Does the boot get past the first Bink 2 video? | First runs in Phase 1 | |
| Does it need `gpu_allow_invalid_fetch_constants` like Giants? | Phase 2, with and without | |
| Where is the projection-matrix function (ultrawide)? | Ghidra, Phase 4 | |
