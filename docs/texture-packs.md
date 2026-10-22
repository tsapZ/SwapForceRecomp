# HD texture packs

Most of what the games draw are textures of 512 px and smaller. At 2x or 3x internal resolution on
a 1440p screen they are stretched, which is where the picture looks soft. A texture pack puts
bigger versions in their place. This works in all six ports, through the rexglue-sdk's texture
replacement (D3D12) and skylanders-portal's pack folder.

## Using a pack

Put the pack's `.dds` files into

    Documents\Skylanders\Textures\<game>        e.g. Documents\Skylanders\Textures\Swap Force

The game uses that folder by itself when it exists; the log says `HD textures from ...` and
`Texture replacement: N DDS files`. With no folder nothing changes. `texture_replace_dir = "<folder>"`
in the settings file points it elsewhere.

## Making a pack

1. **Collect the textures.** Start the game with `--texture_dump_dir=<folder>` and play the places
   you want sharper. Each texture the game loads is written once as `<hash>.dds`, its top level,
   with the same block format as on the disc. The name is the hash of its content, so the same
   texture always gets the same name.
   - Only DXT1 / DXT3 / DXT5 (BC1-BC3) 2D textures of 32 px and more for now: the colour textures.
     Normal maps (DXN), uncompressed and render-to-texture targets are left out.
2. **Make them bigger.** Any tool that reads and writes DDS works, e.g. Microsoft's texconv:
   `texconv -ft png <folder>\*.dds` to PNG, then an upscaler (4x is a good size; an AI upscaler
   such as Real-ESRGAN adds detail, a plain resize doesn't), then back.
3. **Back to DDS in the same format.** `texconv -f BC1_UNORM` for a DXT1 file, `BC2_UNORM` for
   DXT3, `BC3_UNORM` for DXT5. Use a full mip chain (`-m 0`, the default) and keep the name.
   - DXT1 textures with holes (alpha-tested leaves, fences) need the alpha kept as on/off:
     `texconv -f BC1_UNORM -at 0.5`.
   - Resize colour and alpha separately (or use a tool that doesn't premultiply). The games use the
     colour under fully transparent texels (blend masks, water foam), and premultiplying turns it
     black, which shows as dark fringes.
4. **Put them in the pack folder** (above). A file of the wrong format or size is skipped with a
   warning in the log.

## What it costs

Measured in Swap Force's hub at 2x internal resolution: 61 textures 4x larger (Lanczos, with mip
chains, about 110 MB of DDS) kept 60 FPS. The memory is graphics memory: a 4x pack of everything a
level uses takes roughly 0.5-2 GB.

## Limits (first version)

- A replaced texture keeps its replacement even if the game later streams other content into the
  same memory with the same size and format. This hasn't been seen yet; if a wrong picture shows
  up somewhere, removing that file from the pack fixes it.
- D3D12 only (the default backend).
