# Releasing

How to build the installer and publish a release. Not relevant before Phase 2 (playable). No CI:
every step runs on a maintainer's machine against their own copy of the game. The installer never
contains game data; it extracts it from the user's own ISO or folder.

## Build and package

```
just package-installer
```

Builds the release binary, stages it with the installer's other inputs, and compiles
`installer/SwapForceRecomp.iss` with Inno Setup 6 into `installer/Output/SwapForceRecompSetup-<version>.exe`.
Inno Setup: `winget install JRSoftware.InnoSetup`. The recipe looks in both common install locations
for `ISCC.exe`.

The installer has its own `AppId`, separate from GiantsRecomp, so both can be installed side by side.

## Before publishing, verify

- Clean Windows VM without Visual Studio, CMake or Ninja: install once from a real ISO and once from
  an extracted folder; the game launches both times.
- A wrong `default.xex` is rejected with a clear message: before copying for a folder, after
  extraction for an ISO (extract-xiso can't check the version before extracting).
- Installing over an existing install keeps `rom/`, `swapforcerecomp.toml` and saves, and only
  replaces the binary.
- Uninstalling keeps `rom/` and the toml unless the opt-in prompt is answered "Yes";
  `unins000.exe /VERYSILENT` never prompts and keeps them.
- Start menu shortcut works; "Uninstall SwapForceRecomp" appears in the Start menu and in Add/Remove
  Programs.

## Publish

Bump `MyAppVersion` in `installer/SwapForceRecomp.iss` to match the tag. Upload
`installer/Output/SwapForceRecompSetup-<version>.exe` to a GitHub release, and state which exact game version
and region it was built and tested against.

The repository is private for now. Making it public is a separate decision; check the history for
game data and personal information first.
