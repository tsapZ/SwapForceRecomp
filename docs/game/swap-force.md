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
| Release | Europe (English/German), XGD2 | matches the size below; *region not yet checked against the XEX* |
| ISO size | 8,738,846,720 bytes | measured |
| Disc content | 2,474 files, 8,448,047,409 bytes | `extract-xiso -l`, measured |
| `default.xex` size | 18,563,072 bytes, in the disc root | `extract-xiso -l`, measured |
| `default.xex` SHA-256 | not recorded yet | Phase 1 → `default.xex.sha256` |
| Title ID, version | not recorded yet | Phase 1 |

Only the `default.xex` from the disc is used. A title update from a console hard drive is a
different executable with different addresses and is not supported.

`default.xex.sha256` is an all-zero placeholder until Phase 1. Until then the startup check
rejects every XEX, which is intended.

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
| Exact version and region of `default.xex` | XEX header (Phase 1) | |
| Does the game import `XamInputNonControllerGetRaw/SetRaw` (1185/1186)? | Import table / Ghidra | |
| Which functions wrap the portal read/write calls? | Ghidra, callers of the imports | |
| Does the boot get past the first Bink 2 video? | First runs in Phase 1 | |
| Does it need `gpu_allow_invalid_fetch_constants` like Giants? | Phase 2, with and without | |
| Where is the projection-matrix function (ultrawide)? | Ghidra, Phase 4 | |
