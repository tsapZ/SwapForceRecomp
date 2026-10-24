# CLAUDE.md: SwapForceRecomp

## Goal

Native Windows port of Skylanders: Swap Force (Xbox 360, `rom/default.xex`) via ReXGlue static
recompilation: 4K/ultrawide, software + USB portal, settings overlay. Online co-op and profiles
belong to the hub repo, Skylanders: Core of Light (`tsapZ/SkylandersCoreOfLight`), not to this
game; this game must stay complete without the hub.

Template: GiantsRecomp (Skylanders Giants, same engine generation and toolchain). Reference clone
with submodules at `../_ref/GiantsRecomp`; read its README.md and docs/*.md before changing build or
RE tooling. The phase plan lives in `../PROJEKT.md` (outside the repo, never committed).

## Rules

- Analyse first, then build. When unclear, ask instead of guessing.
- Small commits, Conventional Commits (`feat:`, `fix:`, `build:`, `docs:`).
- Never commit game data, secrets or tokens. Before EVERY commit check `git status`: no `rom/`
  content, `*.xex`, `*.iso` or figure dumps.
- No personal information about the owner in files, commit messages or docs.
- Every `config/default.toml` entry and every kernel stub gets a comment: what is at the address,
  how it was found, why the fix or stub is safe (same convention as GiantsRecomp).
- Do not modify submodules (`thirdparty/`) except on clearly named own branches.
- If Swap Force differs technically from Giants in a significant way, report it immediately and
  adapt the plan.
- Git identity is repo-local (`git config user.name/user.email`), never global.
- Scripted or experimental game runs use a copied `--user_data_root`, never the user's real
  `Documents\Skylanders\Swap Force` or the old `Documents\swapforcerecomp` (saves), nor
  `Documents\Skylanders\Figures` (figure dumps). See docs/development.md.

## Build

Always from the **x64 Native Tools Command Prompt for VS 2022**, otherwise CMake does not find the
compiler.

    just submodules        # git submodule update --init --recursive
    just build-release     # configures on first run, then builds (also: build-debug)
    just regen-release     # rerun codegen from rom/default.xex, reconfigure, rebuild
    just test-release
    just play-release
    just --list            # everything else (package, logs, clean)

Codegen needs `rom/default.xex` (Phase 1). `tools/ghidra_re.ps1 setup` downloads Ghidra, JDK and
the XEX loader into `logs/toolchain/` (gitignored); run it in Phase 1, not earlier.

## Hooks ported from the Giants scaffold

- `intro_skip_hook.cpp` and `ultrawide_hook.cpp` (3D camera only, the HUD is still stretched) are
  ported to Swap Force; no Giants addresses are left in `src/hooks/`.

## Status convention

`docs/STATUS.md` is updated at the end of every session: what works, what doesn't, known bugs.
Only record what the build actually did.
`docs/roadmap.md` is the overview for the owner (phase table, "now / next / later", checklists with
issue numbers): update it whenever an issue is closed or the plan changes, and keep the README status
table in step with it.
