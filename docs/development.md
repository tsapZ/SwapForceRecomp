# Development

Building SwapForceRecomp from source. There is no playable build yet; see [STATUS.md](STATUS.md).

## What you need

1. **Windows 10 or 11, 64-bit,** with a DirectX 12 capable graphics card. The CMake presets for
   Linux and macOS come from the template and are untested here.
2. **Your own copy of the game:** the Xbox 360 disc of Skylanders: Swap Force, as an ISO or an
   extracted folder. Which exact version is supported is decided in Phase 1, see
   [game/swap-force.md](game/swap-force.md).
3. **A controller.** An Xbox controller works. Keyboard and mouse via `mnk_mode`.
4. **Build tools:** Visual Studio 2022 with "Desktop development with C++" and "C++ Clang tools for
   Windows" (clang 18 or newer), CMake 3.25+, Ninja, Git, [just](https://github.com/casey/just)
   and Python 3.

## Setup

**1. Turn on symlinks (once).** One of the libraries uses symbolic links. Turn on **Developer Mode**
(Settings → System → For developers), then:

```
git config --global core.symlinks true
git config --global core.longpaths true
```

**2. Get the code with its submodules:**

```
git clone --recursive https://github.com/tsapZ/SwapForceRecomp.git
cd SwapForceRecomp
```

Already cloned without `--recursive`? Run `just submodules`.

**3. Open the right prompt.** From now on, run everything in the **x64 Native Tools Command Prompt
for VS 2022** (Start menu). Elsewhere CMake does not find the compiler.

**4. Put the game files in place.** `rom\default.xex` must exist, next to the rest of the disc:

```
extract-xiso -d rom "path\to\Swap Force.iso"
```

Options must come **before** the file name. A `-d` after it is ignored, and the disc lands in a new
folder named after the ISO in the current directory, which `.gitignore` does not cover.

An already extracted folder is copied as is. GOD containers (folders with hex names) are not
supported directly. `rom/` is git-ignored.

**5. Build.** The first build takes a long time because it also builds the ReXGlue SDK.

```
just build-release      # configure on first run, then build
just regen-release      # codegen from rom/default.xex, reconfigure, rebuild
just play-release
```

Without `just`:

```
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release --target swapforcerecomp_codegen
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release
```

The codegen step turns your `default.xex` into C++ on your machine; that code is never uploaded.
Use the `debug` presets/recipes when chasing a crash, `release` for playing.

Without a ROM, configuring still works (about 2 minutes, tested): CMake prints "No recompiled code
yet" with the codegen steps and writes the build files. There is just no game code to compile until
codegen has run.

Keep the checkout path short. In a deep folder, CMake warns that FidelityFX object paths may exceed
its 250-character limit.

## Everyday commands

| Command | What it does |
|---|---|
| `just` | list all recipes |
| `just build-debug` / `build-release` | configure if needed, build |
| `just regen-debug` / `regen-release` | rerun codegen after changing `config/default.toml` or the dump |
| `just play-debug` / `play-release` | build and start; extra arguments are passed on |
| `just test-debug` / `test-release` | unit tests (ctest) |
| `just logs-debug -f` | follow the newest log file |
| `just clean` | remove build output |

## Testing without touching your saves

Automated or experimental runs write saves and figure progress. Point them at a copy:

    just play-release --user_data_root=C:\path\to\test-userdata

Copy `Documents\swapforcerecomp` (saves under `B13EBABEBABEBABE\`, figures under `figures\`) into
that folder first. Never test against the real user folder.

### Scripted input

`--input_script=<file>` plays a fixed controller timeline as an extra input device, independent of
the window focus (the keyboard emulation only reads keys while the game window has the focus, so a
system dialog in the foreground breaks keyboard-driven test runs). One command per line or `;`:

    wait 18
    tap start            # title screen
    wait 4
    tap a                # story mode
    hold right 600       # d-pad, in milliseconds
    stick 0 1 1500       # left stick forward for 1.5 s
    fps 10               # log the guest frame rate over 10 s
    renderdoc 2          # capture the next 2 guest frames with an attached RenderDoc
    log reached level    # mark the point in the log
    quit

Buttons: `a b x y start back up down left right lb rb ls rs lt rt`. Full syntax in
`src/dev/input_script.h`. For RenderDoc, start the game through `renderdoccmd capture`; the capture
spans whole guest frames (RenderDoc's own triggers capture host frames, and one guest frame's GPU
work is spread over several of them).

## Troubleshooting

- **CMake can't find `clang`/`clang++`:** use the `just` recipes instead of calling `cmake`
  directly; they put the VS Clang tools on `PATH`. Or run `just clean-debug` and try again.
- **"Administrator rights required" on symlinks:** Developer Mode is off, or `core.symlinks` is
  not set. Git uses Developer Mode; Windows PowerShell 5.1's `New-Item -ItemType SymbolicLink`
  does not, so don't use it as a test.
- **The game refuses to start because of the XEX check:** expected until Phase 1 records the
  real hash; after that it means the dump is a different version.
- **Paths too long:** turn on Windows long paths and `git config --global core.longpaths true`.
