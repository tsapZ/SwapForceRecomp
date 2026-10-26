# Menus and keys

Everything the port adds on top of the game, reachable with a controller or the keyboard. The
menus come from the `feature/portal-menu` branch of the skylanders-portal fork
(`thirdparty/skylanders-portal/src/portal_rex/`); they share one look (`menu_draw.cpp`) and hide all
input from the game while they are open.

| What | Controller | Keyboard | Setting |
|---|---|---|---|
| Portal menu: put Skylanders and items on the portal | Guide (Xbox), Back + Start, hold Back | F6 | `portal_menu_open_buttons`, `bind_portal_menu` |
| Portal overview: what is on the portal, taken off one by one (the item too) without lifting the Skylander | Back + Y | F5 | `portal_item_off_buttons`, `bind_portal_item_off` |
| Player menu: which controller is which player | L3 + R3, or F4 > Portal > "Open the player menu" | F9 | `player_menu_buttons`, `bind_player_menu` |
| Settings menu (also over the portal or player menu, which then closes like with B) | Back + RB | F4 | `settings_menu_buttons`, `bind_settings_menu` |
| Achievements | F4 > Game > Achievements | F7 | `bind_achievements` |
| Report a problem: keeps the picture of the moment and the kind of problem (and a few typed words) in `logs\problem-<date>.txt` / `.png`; Core of Light's report takes them along | F4 > Game > "Report a problem" | | |
| Quit or restart the game (both ask first, "No" chosen) | F4 > Game > "Quit the game" / "Restart the game" | Alt+F4 | |
| All settings (developer list of the SDK) | from the settings menu, last page | F10 | `bind_settings` |
| Debug overlay (FPS) | | F3 | `bind_debug_overlay` |
| Portal developer tools | | F8 | `bind_portal_overlay` |

Button entries are comma-separated: a button (`guide`, `back`, `start`, `a`, `b`, `x`, `y`, `lb`,
`rb`, `ls`, `rs`, `up`, `down`, `left`, `right`), a chord joined with `+`, or `hold_<button>`.
System buttons that start an entry (Back, Start, Guide, the stick clicks) are kept from the game
for a moment, so Back doesn't also open the game's own menu; gameplay buttons (A, B, X, Y, the
shoulders, the d-pad) never are, so a chord like `back+y` adds no lag to Y.

## Portal menu

See [portal.md](portal.md#portal-menu-guide-button--f6).

## Portal check

Opens by itself when the game shows its portal error screen (`CMagicMoment` state 16, `eMMS_Error`:
"too many Skylanders / items on the portal", a figure it can't read; found 2026-10-19 by putting a
second sidekick on). It lists everything on the emulated portal: each player's Skylander, the item,
the sidekick and any other figure, marks what there is too much of (more than one sidekick or
item, a Skylander no player has, every Skylander for "too many Skylanders"), and A marks a figure
to take off; Continue takes the marked ones off, and the game sees them leave. B closes it without
changes; while the error stays, it comes back after 8 s. The menu itself never puts a second
sidekick or item on, so this is the fail-safe for figures put on some other way.

## Player menu

See [portal.md](portal.md#player-menu-l3--r3--f9).

## Settings menu

`portal_rex/settings_menu.cpp` draws the menu; the app describes its pages in
`src/settings_pages.cpp`:

- **Graphics:** preset (Performance: scale 1 + FSR; Quality: scale 2 + CAS, recommended; Best:
  scale 3 + CAS), internal resolution (`resolution_scale`), sharpening (`present_effect`), texture
  filtering (`anisotropic_override`), HD textures on or off (`hd_textures`, with a pack installed), the game's own edge smoothing kept sharp
  (`fxaa_follow_resolution_scale`, on), extra edge smoothing (`swap_post_effect`, with the warning
  from #45), glow (`bloom_strength`: off, weak, as in the game, strong), motion blur
  (`motion_blur_strength`: off by default, half, as in the game), native 3D picture
  (`scene_size`: off = 1040x620 like the Xbox 360, or 1280x720; experimental, D3D12, restart),
  fullscreen, window size (`resolution`).
- **Frame rate:** 60 FPS and more (`present_interval`), frame rate limit (`frame_rate_limit`),
  VSync, render path (`render_target_path_d3d12`), graphics card (`d3d12_adapter`), graphics
  interface (`gpu_backend`: automatic = D3D12, D3D12, Vulkan).
- **Game:** language (English or German, what the disc has), skip the logo movies, skip the
  opening movie, Nightmare from the start (`nightmare_from_start`; SwapForceRecomp #74: the
  hardest difficulty without finishing Story Mode first, the save itself unchanged).
- **Portal:** virtual / USB / none (`portal_mode`), the portal menu opening by itself, only my
  figures.
- **Controls:** an overview, then the menu buttons: portal menu (`portal_menu_open_buttons`),
  the portal overview (`portal_item_off_buttons`), player menu (`player_menu_buttons`),
  settings menu (`settings_menu_buttons`), each with a few choices that don't clash and "off";
  keyboard and mouse (`mnk_mode`), which player the keyboard is (`keyboard_player`). The info
  panel shows the chosen buttons on the game's own Xbox 360 controller picture (its Controls
  screen's, with the callout lines painted out), a coloured ring per entry; the overview shows
  every menu's buttons at once. The buttons count for every controller and apply at once.
- **Keyboard, Controller:** the keyboard's key for each controller button; what each controller
  button does in the game (`pad_map_a` ... `pad_map_rs`, e.g. a shoulder button doing B's job) and
  how strongly each player's controller vibrates (`pad_rumble_p1` / `_p2`, 10 % steps, the
  controller buzzes at each change; SwapForceRecomp #71, #73).
- **Advanced:** opens the SDK's list of every setting (F10).

LB / RB or LT / RT switch the page, up / down pick a setting, left / right (or A) change it, B or
Start close. The right side explains the setting and lists its choices. A change applies at once
where the setting allows it, so it can be seen; **Y saves** the changes into `swapforcerecomp.toml`
(the footer says "Saved"), only their lines, so comments and everything else in the file stay
(`settings_file.cpp`). Closing with unsaved changes asks: save, don't save (the old values come
back), or back to the settings. Settings that only apply at start-up (`kInitOnly` /
`kRequiresRestart`, or marked by the page) say "applies at the next start", and changed ones get a
gold dot. For test runs `--config_file=<path>` makes the app read and write another settings file,
so the real one stays untouched.

The SDK list's own "Save to config" button still rewrites the whole file from every setting that
differs from its default, including command-line ones, and drops the comments; use the settings
menu for everyday changes.
