# Roadmap

One GitHub milestone per phase. Each phase ends with a short entry in [STATUS.md](STATUS.md).
A phase is only "done" when its done criterion was observed in a real build, not inferred.

## Phase 0: repository and scaffold

Set up the repository from the GiantsRecomp template, without any game data.

- Reference clone of GiantsRecomp (outside the repo) and its docs read.
- Scaffold copied and renamed, submodules pinned, license, credits, docs.
- Issues and milestones created.
- **Done when:** `just configure-release` succeeds on a clean clone without a ROM, and the
  limits of that (codegen needs the ROM) are documented.

## Phase 1: recompile and boot

- Extract the disc into `rom/`, record the SHA-256 of `default.xex` in
  `docs/game/default.xex.sha256`, note version and region in
  [game/swap-force.md](game/swap-force.md).
- Run codegen, build, start. Then loop: crash or log line → find the cause (Ghidra tooling) →
  fix in `config/default.toml` or a kernel stub → rebuild. See [build.md](build.md).
- Early check: does Swap Force call `XamInputNonControllerGetRaw/SetRaw` (ordinals 1185/1186)?
  The portal plan depends on it. Result goes into STATUS.md.
- Early check: does the engine look like Giants (same studio tooling, same hot spots), or not?
  Swap Force is from a different studio, so this is an open assumption.
- **Done when:** the publisher and studio logos (or the intro video) play.

## Phase 2: in-game

- Menus, loading, first level. Audio, file I/O, threading, graphics glitches.
- Graphics: try the existing cvars first (`gpu_allow_invalid_fetch_constants` and friends)
  before patching anything.
- Bink 2 video playback is a known stumbling block for an earlier Swap Force attempt; see
  [game/swap-force.md](game/swap-force.md).
- **Done when:** the first level can be played through with a controller.

## Phase 3: portal

- Wire `portal_mode` software/usb/none through the submodule, as GiantsRecomp does.
- Test a normal figure, a swapper (both halves at once), and that progress is written back.
- If swapper halves don't work: fix it in `skylanders-portal` on a clearly named branch, kept
  separate so it can be offered upstream. See [portal.md](portal.md).
- **Done when:** a swapper can be placed, swapped and saved with both the software and the USB
  portal.

## Phase 4: 4K and polish

- Test `resolution`, `resolution_scale`, `present_use_actual_aspect`.
- Find Swap Force's projection-matrix function with Ghidra and port the ultrawide hook.
- Port the intro skip, settings overlay (F4), installer.
- **Done when:** 4K and 21:9 render correctly, and the installer works on a clean Windows VM.

## Phase 5 (stretch): online co-op with two real portals

- First an architecture proposal in `docs/online-coop.md`, not code: a remote client reads its
  real portal, streams figure data to a software-portal slot on the host, and writes (level,
  gold) go back to the remote client. Picture and input via Parsec or Steam Remote Play.
- **Done when:** the proposal is written and agreed; building it is a separate decision.

## Risks

| Risk | Effect | When we know |
|---|---|---|
| Swap Force's engine differs a lot from Giants | Phase 1–2 take much longer; less reuse | early Phase 1 |
| Game does not use XAM ordinals 1185/1186 for the portal | Portal needs a game-address hook instead | early Phase 1 |
| Bink 2 videos stall (seen in an earlier attempt) | Black screen at boot | Phase 1 |
| Swapper halves need protocol behaviour the portal code lacks | Fix in the submodule | Phase 3 |
