# SwapForceRecomp documentation

This folder is the project wiki. Pages, roughly in reading order:

| Page | What it covers |
|---|---|
| [roadmap.md](roadmap.md) | Phases 0 to 5: goals, done criteria, risks |
| [STATUS.md](STATUS.md) | What works right now; updated at the end of every session |
| [game/swap-force.md](game/swap-force.md) | The game, the target dump, how it differs from Giants, open questions |
| [architecture.md](architecture.md) | Repository layout, recompile pipeline, app lifecycle, settings |
| [development.md](development.md) | Building from source on Windows |
| [build.md](build.md) | Codegen fixes (`config/default.toml`), kernel stubs, the fix loop, testing |
| [reverse-engineering.md](reverse-engineering.md) | Headless Ghidra: identify and name functions |
| [portal.md](portal.md) | Portal of Power: protocol, figures, swappers, overlay |
| [releasing.md](releasing.md) | Packaging the installer and publishing a release |

Most of the tooling and several of these pages are adapted from
[GiantsRecomp](https://github.com/TheBiemGamer/GiantsRecomp). Anything that was measured on
Skylanders Giants and has not been confirmed on Swap Force yet is marked as such.

## Glossary

- **Static recompilation:** translating the game's PowerPC machine code into C++ once, ahead of
  time, and compiling that into a native Windows program. No emulation of the CPU at runtime.
- **ReXGlue:** the SDK that does the translation (codegen) and provides the runtime: Xbox 360
  kernel/XAM functions, the Xenos GPU on Direct3D 12/Vulkan, audio, input. Lives in
  `thirdparty/rexglue-sdk`.
- **`default.xex`:** the game's main executable on the disc. Everything in `config/default.toml`
  is keyed to addresses inside exactly one version of this file.
- **Codegen:** the build step that turns `rom/default.xex` into C++ under `generated/`. Runs
  locally; its output is never committed.
- **Guest address:** an address in the original Xbox 360 program (e.g. `0x82153BE8`). Functions
  without a known purpose are named `sub_<address>`.
- **Midasm hook:** a call into our own C++ inserted at a guest address, declared in
  `config/default.toml` and implemented in `src/hooks/`.
- **Kernel stub:** a placeholder for a kernel or XAM import the runtime does not implement,
  in `src/game/kernel_stubs.cpp`.
- **XAM ordinal:** Xbox system-library functions are imported by number. The portal goes through
  `xam.xex` ordinals 1185/1186.
- **Cvar:** a runtime setting (`REXCVAR_DEFINE_*`), set in `swapforcerecomp.toml`, by `--flag`
  or by a `REX_*` environment variable.
- **Swapper:** a Swap Force figure whose top and bottom halves come apart; each half is its own
  figure on the portal.
