#include "settings_pages.h"

#include <string>
#include <utility>

#include <rex/cvar.h>

namespace swapforcerecomp {
namespace {

using skylanders::Setting;
using skylanders::SettingOption;
using skylanders::SettingsPage;

// UTF-8 is written as escapes, split where a hex digit follows ("n\xC3\xA4" "chsten").

// Resolution scale and present effect together. The FPS are the hub measurements of 2026-10-19
// (#50: 2560x1440 output, RTV, no frame rate limit): scale 1 + FSR 169, scale 2 + CAS 83,
// scale 3 + CAS 40.
Setting PresetSetting() {
  Setting s;
  s.en = "Preset";
  s.de = "Voreinstellung";
  s.help_en =
      "Sets the internal resolution and sharpening together. Performance: the game's own 720p "
      "picture, upscaled with FSR, for slower PCs (about 170 FPS in the hub in the tests). "
      "Quality (recommended): twice the resolution, sharpened; sharp on a 1440p screen, about "
      "85 FPS in the hub. Best: three times the resolution, sharp in 4K, needs a strong graphics "
      "card (about 40 FPS in the hub).";
  s.help_de =
      "Stellt Aufl\xC3\xB6sung und Sch\xC3\xA4rfe zusammen ein. Leistung: das 720p-Bild des "
      "Spiels, mit FSR hochskaliert, f\xC3\xBCr schw\xC3\xA4" "chere PCs (in den Tests rund 170 "
      "FPS im Hub). Qualit\xC3\xA4t (empfohlen): doppelte Aufl\xC3\xB6sung, gesch\xC3\xA4rft; "
      "scharf auf einem 1440p-Bildschirm, rund 85 FPS im Hub. Beste: dreifache Aufl\xC3\xB6sung, "
      "scharf in 4K, braucht eine starke Grafikkarte (um 40 FPS im Hub).";
  s.options = {{"performance", "Performance", "Leistung"},
               {"quality", "Quality", "Qualit\xC3\xA4t"},
               {"best", "Best", "Beste"}};
  s.restart = true;
  s.current = [] {
    const std::string scale = rex::cvar::GetFlagByName("resolution_scale");
    const std::string effect = rex::cvar::GetFlagByName("present_effect");
    if (scale == "1" && effect == "fsr") return std::string("performance");
    if (scale == "2" && effect == "cas") return std::string("quality");
    if (scale == "3" && effect == "cas") return std::string("best");
    return std::string();
  };
  s.preset = [](const std::string& value) -> std::vector<std::pair<std::string, std::string>> {
    if (value == "performance") return {{"resolution_scale", "1"}, {"present_effect", "fsr"}};
    if (value == "best") return {{"resolution_scale", "3"}, {"present_effect", "cas"}};
    return {{"resolution_scale", "2"}, {"present_effect", "cas"}};
  };
  return s;
}

SettingsPage GraphicsPage() {
  SettingsPage page{"Graphics", "Grafik", {}};
  page.settings.push_back(PresetSetting());
  page.settings.push_back(
      {"resolution_scale", "Internal resolution", "Interne Aufl\xC3\xB6sung",
       "How finely the game draws the 3D scene: a multiple of its own 1280x720. 2x is exactly "
       "1440p, 3x exactly 4K. Every step costs a lot of GPU time.",
       "Wie fein das Spiel die 3D-Szene zeichnet: ein Vielfaches seiner eigenen 1280x720. 2x ist "
       "genau 1440p, 3x genau 4K. Jede Stufe kostet viel Grafikleistung.",
       {{"1", "1x (720p)", "1x (720p)"},
        {"2", "2x (1440p)", "2x (1440p)"},
        {"3", "3x (4K)", "3x (4K)"},
        {"4", "4x", "4x"}}});
  page.settings.push_back(
      {"present_effect", "Sharpening", "Sch\xC3\xA4rfe",
       "How the picture is scaled to the window. Sharpened (CAS) is best when the internal "
       "resolution matches the screen; FSR upscales a smaller picture; soft is plain bilinear.",
       "Wie das Bild aufs Fenster skaliert wird. Gesch\xC3\xA4rft (CAS) ist am besten, wenn die "
       "interne Aufl\xC3\xB6sung zum Bildschirm passt; FSR skaliert ein kleineres Bild hoch; "
       "weich ist einfaches bilineares Skalieren.",
       {{"bilinear", "Soft", "Weich"},
        {"cas", "Sharpened (CAS)", "Gesch\xC3\xA4rft (CAS)"},
        {"fsr", "FSR 1 upscaling", "FSR 1 (hochskaliert)"}}});
  page.settings.push_back(
      {"anisotropic_override", "Texture filtering", "Texturfilter",
       "Keeps floors and walls sharp at a shallow angle. 16x costs next to nothing (83 vs 84 FPS "
       "against 4x in the tests).",
       "H\xC3\xA4lt B\xC3\xB6" "den und W\xC3\xA4nde auch schr\xC3\xA4g betrachtet scharf. 16x "
       "kostet so gut wie nichts (in den Tests 83 statt 84 FPS gegen\xC3\xBC" "ber 4x).",
       {{"0", "Off", "Aus"}, {"3", "4x", "4x"}, {"4", "8x", "8x"}, {"5", "16x", "16x"}}});
  page.settings.push_back(
      {"fxaa_follow_resolution_scale", "Game's edge smoothing, sharp", "Kantengl\xC3\xA4ttung des "
       "Spiels, scharf",
       "The game smooths edges itself (FXAA). On: it works at the internal resolution, so the "
       "picture stays sharp at 2x and 3x. Off: as on the console, which blurs at 2x and up. Costs "
       "nothing.",
       "Das Spiel gl\xC3\xA4ttet Kanten selbst (FXAA). An: das passiert in der internen "
       "Aufl\xC3\xB6sung, so bleibt das Bild bei 2x und 3x scharf. Aus: wie auf der Konsole, das "
       "verwischt ab 2x. Kostet nichts."});
  page.settings.push_back(
      {"swap_post_effect", "Extra edge smoothing", "Zus\xC3\xA4tzliche Kantengl\xC3\xA4ttung",
       "A second FXAA over the whole picture, on top of the game's own. For now it also makes the "
       "picture darker and paler (#45). Recommended: off.",
       "Ein zweites FXAA \xC3\xBC" "ber das ganze Bild, zus\xC3\xA4tzlich zu dem des Spiels. Macht "
       "das Bild zurzeit auch dunkler und blasser (#45). Empfohlen: aus.",
       {{"none", "Off", "Aus"}, {"fxaa", "FXAA", "FXAA"}, {"fxaa_extreme", "FXAA strong",
                                                         "FXAA stark"}}});
  Setting fullscreen{"fullscreen", "Fullscreen", "Vollbild",
                     "Start in fullscreen or in a window.",
                     "Im Vollbild oder im Fenster starten."};
  fullscreen.restart = true;
  page.settings.push_back(fullscreen);
  page.settings.push_back(
      {"resolution", "Window size", "Fenstergr\xC3\xB6\xC3\x9F" "e",
       "The window's size when not in fullscreen, and the picture size the game is told about.",
       "Die Gr\xC3\xB6\xC3\x9F" "e des Fensters au\xC3\x9F" "erhalb des Vollbilds und die "
       "Bildgr\xC3\xB6\xC3\x9F" "e, die das Spiel bekommt.",
       {{"", "Automatic", "Automatisch"},
        {"1280x720", "1280x720", "1280x720"},
        {"1920x1080", "1920x1080", "1920x1080"},
        {"2560x1440", "2560x1440", "2560x1440"},
        {"3840x2160", "3840x2160", "3840x2160"}}});
  return page;
}

SettingsPage FrameRatePage() {
  SettingsPage page{"Frame rate", "Bildrate", {}};
  page.settings.push_back(
      {"present_interval", "60 FPS and more", "60 FPS und mehr",
       "The game was made for 30 FPS. On: as many frames as the limit below allows. The game "
       "speed was checked in the tutorial at 60 FPS.",
       "Das Spiel ist f\xC3\xBCr 30 FPS gemacht. An: so viele Bilder, wie die Grenze darunter "
       "erlaubt. Die Spielgeschwindigkeit wurde im Tutorial mit 60 FPS gepr\xC3\xBC" "ft.",
       {{"0", "Off (30 FPS)", "Aus (30 FPS)"}, {"1", "On", "An"}}});
  page.settings.push_back(
      {"frame_rate_limit", "Frame rate limit", "FPS-Grenze",
       "The most frames per second with 60 FPS and more on. Best at your screen's refresh rate.",
       "H\xC3\xB6" "chstens so viele Bilder pro Sekunde, wenn 60 FPS und mehr an ist. Am besten "
       "die Bildwiederholrate deines Bildschirms.",
       {{"30", "30", "30"},
        {"60", "60", "60"},
        {"120", "120", "120"},
        {"144", "144", "144"},
        {"165", "165", "165"},
        {"200", "200", "200"},
        {"240", "240", "240"}}});
  page.settings.push_back({"vsync", "VSync", "VSync",
                           "Waits for the screen between frames: no tearing.",
                           "Wartet zwischen den Bildern auf den Bildschirm: kein Zerrei\xC3\x9F" "en."});
  page.settings.push_back(
      {"render_target_path_d3d12", "Render path", "Renderpfad",
       "Fast (RTV) is the one to use. Exact (ROV) emulates the Xbox 360's memory bit for bit and "
       "is much slower; only for comparing graphics errors.",
       "Schnell (RTV) ist der richtige. Genau (ROV) bildet den Speicher der Xbox 360 Bit f\xC3\xBCr "
       "Bit nach und ist viel langsamer; nur zum Vergleichen bei Grafikfehlern.",
       {{"rtv", "Fast (RTV)", "Schnell (RTV)"}, {"rov", "Exact (ROV)", "Genau (ROV)"}}});
  page.settings.push_back(
      {"d3d12_adapter", "Graphics card", "Grafikkarte",
       "Which graphics card draws the game, for PCs with more than one.",
       "Welche Grafikkarte das Spiel zeichnet, f\xC3\xBCr PCs mit mehreren.",
       {{"-1", "Automatic", "Automatisch"}, {"0", "GPU 1", "GPU 1"}, {"1", "GPU 2", "GPU 2"}}});
  return page;
}

SettingsPage GamePage() {
  SettingsPage page{"Game", "Spiel", {}};
  Setting language{"language", "Language", "Sprache",
                   "Text and voices. The disc has English and German.",
                   "Texte und Stimmen. Die Disc hat Englisch und Deutsch.",
                   {{"english", "English", "English"}, {"german", "Deutsch", "Deutsch"}}};
  language.restart = true;
  page.settings.push_back(language);
  Setting intro{"skip_intro", "Skip logo movies", "Logo-Videos \xC3\xBC" "berspringen",
                "The Activision and Vicarious Visions logos at start-up.",
                "Die Logos von Activision und Vicarious Visions beim Start."};
  intro.restart = true;
  page.settings.push_back(intro);
  page.settings.push_back({"skip_opening_movie", "Skip the opening movie",
                           "Er\xC3\xB6" "ffnungsfilm \xC3\xBC" "berspringen",
                           "The long movie when a new game starts.",
                           "Der lange Film beim Start eines neuen Spiels."});
  return page;
}

SettingsPage PortalPage() {
  SettingsPage page{"Portal", "Portal", {}};
  Setting mode{"portal_mode", "Portal", "Portal",
               "Virtual: the portal menu (Xbox button) puts figures on. USB: a real Portal of "
               "Power, also the ones from Spyro's Adventure and Giants.",
               "Virtuell: das Portal-Men\xC3\xBC (Xbox-Taste) stellt Figuren auf. USB: ein echtes "
               "Portal of Power, auch die von Spyro's Adventure und Giants.",
               {{"software", "Virtual (menu)", "Virtuell (Men\xC3\xBC)"},
                {"usb", "Real USB portal", "Echtes USB-Portal"},
                {"none", "None", "Keins"}}};
  mode.restart = true;
  page.settings.push_back(mode);
  page.settings.push_back(
      {"portal_menu_auto_open", "Menu opens by itself", "Men\xC3\xBC \xC3\xB6" "ffnet sich selbst",
       "When the game asks for a Skylander, the portal menu comes up for that player.",
       "Wenn das Spiel nach einem Skylander fragt, \xC3\xB6" "ffnet sich das Portal-Men\xC3\xBC "
       "f\xC3\xBCr diesen Spieler."});
  page.settings.push_back(
      {"portal_menu_owned_only", "Only my figures", "Nur eigene Figuren",
       "The portal menu lists only figures that have a figure file.",
       "Das Portal-Men\xC3\xBC zeigt nur Figuren, f\xC3\xBCr die es eine Figurendatei gibt."});
  return page;
}

SettingsPage ControlsPage() {
  SettingsPage page{"Controls", "Steuerung", {}};
  page.settings.push_back(
      {"portal_menu_open_buttons", "Portal menu button", "Portal-Men\xC3\xBC-Taste",
       "What opens the portal menu on a controller. Turn off the Xbox Game Bar in Windows if the "
       "Xbox button opens it too.",
       "Womit sich das Portal-Men\xC3\xBC am Controller \xC3\xB6" "ffnet. Wenn die Xbox-Taste "
       "auch die Xbox Game Bar \xC3\xB6" "ffnet, die Game Bar in Windows ausschalten.",
       {{"guide,back+start,hold_back", "Xbox, Back+Start, hold Back",
         "Xbox, Back+Start, Back halten"},
        {"guide", "Xbox button only", "Nur Xbox-Taste"},
        {"back+start,hold_back", "Back+Start, hold Back", "Back+Start, Back halten"}}});
  page.settings.push_back(
      {"mnk_mode", "Keyboard and mouse", "Tastatur und Maus",
       "Play with the keyboard as a controller.", "Mit der Tastatur als Controller spielen."});
  page.settings.push_back(
      {"keyboard_player", "Keyboard plays as", "Tastatur spielt als",
       "Which player the keyboard is. Controllers are moved in the player menu (L3 + R3).",
       "Welcher Spieler die Tastatur ist. Controller verschiebt das Spieler-Men\xC3\xBC (L3 + R3).",
       {{"1", "Player 1", "Spieler 1"},
        {"2", "Player 2", "Spieler 2"},
        {"3", "Player 3", "Spieler 3"},
        {"4", "Player 4", "Spieler 4"}}});
  return page;
}

}  // namespace

std::vector<SettingsPage> BuildSettingsPages(std::function<void()> open_all_settings) {
  std::vector<SettingsPage> pages = {GraphicsPage(), FrameRatePage(), GamePage(), PortalPage(),
                                     ControlsPage()};
  SettingsPage advanced{"Advanced", "Erweitert", {}};
  Setting all;
  all.en = "All settings (developers)";
  all.de = "Alle Einstellungen (Entwickler)";
  all.help_en =
      "The SDK's full list of settings (F10). Its \"Save to config\" button rewrites the whole "
      "settings file; changes made here are saved one by one.";
  all.help_de =
      "Die vollst\xC3\xA4ndige Liste aller Einstellungen des SDK (F10). Sein \"Save to config\" "
      "schreibt die ganze Einstellungsdatei neu; was hier ge\xC3\xA4ndert wird, speichert einzeln.";
  all.action = std::move(open_all_settings);
  advanced.settings.push_back(std::move(all));
  pages.push_back(std::move(advanced));
  return pages;
}

}  // namespace swapforcerecomp
