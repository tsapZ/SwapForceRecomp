# Menus and keys

Everything the port adds on top of the game, reachable with a controller or the keyboard. The
menus come from the `feature/portal-menu` branch of the skylanders-portal fork
(`thirdparty/skylanders-portal/src/portal_rex/`); they share one look (`menu_draw.cpp`) and hide all
input from the game while they are open.

| What | Controller | Keyboard | Setting |
|---|---|---|---|
| Portal menu: put Skylanders and items on the portal | Guide (Xbox), Back + Start, hold Back | F6 | `portal_menu_open_buttons`, `bind_portal_menu` |
| Take the item off the portal | Back + Y | F5 | `portal_item_off_buttons`, `bind_portal_item_off` |
| Player menu: which controller is which player | L3 + R3 | F9 | `player_menu_buttons`, `bind_player_menu` |
| Settings menu | Back + RB | F4 | `settings_menu_buttons`, `bind_settings_menu` |
| All settings (developer list of the SDK) | from the settings menu, last page | F10 | `bind_settings` |
| Debug overlay (FPS) | | F3 | `bind_debug_overlay` |
| Achievements | | F7 | `bind_achievements` |
| Portal developer tools | | F8 | `bind_portal_overlay` |

Button entries are comma-separated: a button (`guide`, `back`, `start`, `a`, `b`, `x`, `y`, `lb`,
`rb`, `ls`, `rs`, `up`, `down`, `left`, `right`), a chord joined with `+`, or `hold_<button>`.
System buttons that start an entry (Back, Start, Guide, the stick clicks) are kept from the game
for a moment, so Back doesn't also open the game's own menu; gameplay buttons (A, B, X, Y, the
shoulders, the d-pad) never are, so a chord like `back+y` adds no lag to Y.

## Portal menu

See [portal.md](portal.md#portal-menu-guide-button--f6).

## Player menu

See [portal.md](portal.md#player-menu-l3--r3--f9).

## Settings menu

`portal_rex/settings_menu.cpp` draws the menu; the app describes its pages in
`src/settings_pages.cpp`:

- **Graphics:** preset (Performance: scale 1 + FSR; Quality: scale 2 + CAS, recommended; Best:
  scale 3 + CAS), internal resolution (`resolution_scale`), sharpening (`present_effect`), texture
  filtering (`anisotropic_override`), the game's own edge smoothing kept sharp
  (`fxaa_follow_resolution_scale`, on), extra edge smoothing (`swap_post_effect`, with the warning
  from #45), glow (`bloom_strength`: off, weak, as in the game, strong), motion blur
  (`motion_blur_strength`: off, half, as in the game), fullscreen, window size (`resolution`).
- **Frame rate:** 60 FPS and more (`present_interval`), frame rate limit (`frame_rate_limit`),
  VSync, render path (`render_target_path_d3d12`), graphics card (`d3d12_adapter`).
- **Game:** language (English or German, what the disc has), skip the logo movies, skip the
  opening movie.
- **Portal:** virtual / USB / none (`portal_mode`), the portal menu opening by itself, only my
  figures.
- **Controls:** what opens the portal menu, keyboard and mouse (`mnk_mode`), which player the
  keyboard is (`keyboard_player`).
- **Advanced:** opens the SDK's list of every setting (F10).

LB / RB or LT / RT switch the page, up / down pick a setting, left / right (or A) change it, B or
Start close. The right side explains the setting and lists its choices. Every change is written
into `swapforcerecomp.toml` at once, only that line, so comments and everything else in the file
stay (`settings_file.cpp`). Settings that only apply at start-up (`kInitOnly` / `kRequiresRestart`,
or marked by the page) say "applies at the next start", and changed ones get a gold dot.

The SDK list's own "Save to config" button still rewrites the whole file from every setting that
differs from its default, including command-line ones, and drops the comments; use the settings
menu for everyday changes.
