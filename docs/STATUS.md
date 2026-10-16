# Status

Updated at the end of every session. Only record what a build actually did.

## 2026-10-16: Phase 0, scaffold

- **Works:** nothing has been built or run yet.
- **Done:** scaffold copied from GiantsRecomp and renamed; submodules pinned to the commits
  GiantsRecomp uses; config/default.toml starts empty.
- **Not done:** no ROM in rom/, codegen never run, no build attempted (Phase 0 step 10 is open).
- **Placeholders:** docs/game/default.xex.sha256 is all zeros, so xex_verify rejects every XEX
  until Phase 1. The hooks in src/hooks/ are disabled templates (`#if 0`) with Giants addresses.
- **Known bugs:** none recorded.
