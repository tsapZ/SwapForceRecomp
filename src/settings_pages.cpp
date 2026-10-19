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
      "picture, upscaled with FSR; for weaker PCs. Quality (recommended): twice the resolution, "
      "sharpened; sharp on a 1440p screen. Best: three times the resolution for 4K screens; needs "
      "a strong graphics card. Each step up roughly halves the frame rate.";
  s.help_de =
      "Stellt Aufl\xC3\xB6sung und Sch\xC3\xA4rfe zusammen ein. Leistung: das 720p-Bild des "
      "Spiels, mit FSR hochgerechnet; f\xC3\xBCr schw\xC3\xA4" "chere PCs. Qualit\xC3\xA4t "
      "(empfohlen): doppelte Aufl\xC3\xB6sung, nachgesch\xC3\xA4rft; scharf auf einem "
      "1440p-Bildschirm. Beste: dreifache Aufl\xC3\xB6sung f\xC3\xBCr 4K-Bildschirme; braucht eine "
      "starke Grafikkarte. Jede Stufe h\xC3\xB6her halbiert ungef\xC3\xA4hr die Bildrate.";
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
       "How finely the game draws its 3D scenes, as a multiple of its own 1280x720. 2x fits a "
       "1440p screen, 3x a 4K screen. Higher is sharper but costs a lot: each step roughly halves "
       "the frame rate.",
       "Wie fein das Spiel seine 3D-Szenen zeichnet, als Vielfaches seiner eigenen 1280x720. 2x "
       "passt zu einem 1440p-Bildschirm, 3x zu 4K. H\xC3\xB6her ist sch\xC3\xA4rfer, kostet aber "
       "viel: Jede Stufe halbiert ungef\xC3\xA4hr die Bildrate.",
       {{"1", "1x (720p)", "1x (720p)"},
        {"2", "2x (1440p)", "2x (1440p)"},
        {"3", "3x (4K)", "3x (4K)"},
        {"4", "4x", "4x"}}});
  page.settings.push_back(
      {"present_effect", "Sharpening", "Sch\xC3\xA4rfe",
       "How the finished picture is scaled to your window. Sharpened (CAS): best when the "
       "internal resolution fits your screen. FSR 1: enlarges a smaller picture and sharpens it, "
       "good with 1x. Soft: plain scaling without sharpening. Costs almost nothing.",
       "Wie das fertige Bild auf dein Fenster skaliert wird. Gesch\xC3\xA4rft (CAS): am besten, "
       "wenn die interne Aufl\xC3\xB6sung zum Bildschirm passt. FSR 1: vergr\xC3\xB6\xC3\x9F" "ert "
       "ein kleineres Bild und sch\xC3\xA4rft es nach, gut bei 1x. Weich: einfaches Skalieren "
       "ohne Sch\xC3\xA4rfen. Kostet fast nichts.",
       {{"bilinear", "Soft", "Weich"},
        {"cas", "Sharpened (CAS)", "Gesch\xC3\xA4rft (CAS)"},
        {"fsr", "FSR 1 upscaling", "FSR 1 (hochskaliert)"}}});
  page.settings.push_back(
      {"anisotropic_override", "Texture filtering", "Texturfilter",
       "Keeps floors and walls sharp when you look at them at a shallow angle; without it they "
       "blur in the distance. 16x is recommended: it costs about 1 % of the frame rate.",
       "H\xC3\xA4lt B\xC3\xB6" "den und W\xC3\xA4nde scharf, wenn man flach darauf schaut; ohne "
       "Filter verschwimmen sie in der Ferne. Empfohlen ist 16x: Das kostet etwa 1 % Bildrate.",
       {{"0", "Off", "Aus"}, {"3", "4x", "4x"}, {"4", "8x", "8x"}, {"5", "16x", "16x"}}});
  page.settings.push_back(
      {"fxaa_follow_resolution_scale", "Game's edge smoothing, sharp", "Kantengl\xC3\xA4ttung des "
       "Spiels, scharf",
       "The game smooths jagged edges itself (FXAA). On: the smoothing works at the internal "
       "resolution, so the picture stays sharp at 2x and 3x. Off: as on the console, which makes "
       "the picture blurry from 2x on. Costs nothing; recommended: on.",
       "Das Spiel gl\xC3\xA4ttet Treppenkanten selbst (FXAA). An: das passiert in der internen "
       "Aufl\xC3\xB6sung, so bleibt das Bild bei 2x und 3x scharf. Aus: wie auf der Konsole, das "
       "macht das Bild ab 2x unscharf. Kostet nichts; empfohlen: an."});
  page.settings.push_back(
      {"swap_post_effect", "Extra edge smoothing", "Zus\xC3\xA4tzliche Kantengl\xC3\xA4ttung",
       "A second edge smoothing over the whole picture, on top of the game's own. For now it also "
       "makes the picture darker and paler (a known bug). Recommended: off.",
       "Eine zweite Kantengl\xC3\xA4ttung \xC3\xBC" "ber das ganze Bild, zus\xC3\xA4tzlich zu der "
       "des Spiels. Macht das Bild zurzeit auch dunkler und blasser (bekannter Fehler). "
       "Empfohlen: aus.",
       {{"none", "Off", "Aus"}, {"fxaa", "FXAA", "FXAA"}, {"fxaa_extreme", "FXAA strong",
                                                         "FXAA stark"}}});
  page.settings.push_back(
      {"bloom_strength", "Glow (bloom)", "Leuchten (Bloom)",
       "The soft glow the game puts around bright areas and lights. Off: a clearer, slightly "
       "darker picture; strong: more glow. Costs nothing.",
       "Der weiche Schein, den das Spiel um helle Stellen und Lichter legt. Aus: ein klareres, "
       "etwas dunkleres Bild; stark: mehr Schein. Kostet nichts.",
       {{"0", "Off", "Aus"},
        {"0.5", "Weak", "Schwach"},
        {"1", "As in the game", "Wie im Spiel"},
        {"1.5", "Strong", "Stark"}}});
  page.settings.push_back(
      {"motion_blur_strength", "Motion blur", "Bewegungsunsch\xC3\xA4rfe",
       "Smears fast movement and the whole picture when the camera turns quickly. Motion blur "
       "makes 30 FPS look smoother; at higher frame rates it mostly blurs, so it is off by "
       "default. Costs nothing.",
       "Verwischt schnelle Bewegungen und das ganze Bild bei schnellen Kameradrehungen. "
       "Bewegungsunsch\xC3\xA4rfe l\xC3\xA4sst 30 FPS fl\xC3\xBCssiger wirken; bei h\xC3\xB6heren "
       "Bildraten verwischt sie vor allem, deshalb ist sie standardm\xC3\xA4\xC3\x9Fig aus. "
       "Kostet nichts.",
       {{"0", "Off", "Aus"}, {"0.5", "Half", "Halb"}, {"1", "As in the game", "Wie im Spiel"}}});
  Setting fullscreen{"fullscreen", "Fullscreen", "Vollbild",
                     "Starts the game in fullscreen or in a window.",
                     "Startet das Spiel im Vollbild oder im Fenster."};
  fullscreen.restart = true;
  page.settings.push_back(fullscreen);
  page.settings.push_back(
      {"resolution", "Window size", "Fenstergr\xC3\xB6\xC3\x9F" "e",
       "The window's size outside fullscreen, and the picture size the game works with. Wider "
       "than 16:9 (ultrawide) shows more to the sides.",
       "Die Gr\xC3\xB6\xC3\x9F" "e des Fensters au\xC3\x9F" "erhalb des Vollbilds, zugleich die "
       "Bildgr\xC3\xB6\xC3\x9F" "e, mit der das Spiel arbeitet. Breiter als 16:9 (Ultrawide) zeigt "
       "an den Seiten mehr.",
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
       "The game was made for 30 FPS. On: as many frames as the limit below allows; the game "
       "keeps its normal speed. Off: 30 FPS as on the console.",
       "Das Spiel ist f\xC3\xBCr 30 FPS gemacht. An: so viele Bilder, wie die Grenze darunter "
       "erlaubt; das Spiel l\xC3\xA4uft dabei normal schnell. Aus: 30 FPS wie auf der Konsole.",
       {{"0", "Off (30 FPS)", "Aus (30 FPS)"}, {"1", "On", "An"}}});
  page.settings.push_back(
      {"frame_rate_limit", "Frame rate limit", "FPS-Grenze",
       "The most frames per second with 60 FPS and more on. Best set to your screen's refresh "
       "rate. If your PC can't reach it, the game simply runs as fast as it can.",
       "H\xC3\xB6" "chstens so viele Bilder pro Sekunde, wenn 60 FPS und mehr an ist. Am besten "
       "die Bildwiederholrate deines Bildschirms. Schafft dein PC das nicht, l\xC3\xA4uft das "
       "Spiel einfach so schnell, wie er kann.",
       {{"30", "30", "30"},
        {"60", "60", "60"},
        {"120", "120", "120"},
        {"144", "144", "144"},
        {"165", "165", "165"},
        {"200", "200", "200"},
        {"240", "240", "240"}}});
  page.settings.push_back({"vsync", "VSync", "VSync",
                           "Waits for the screen before showing a frame, so the picture never "
                           "tears in two when the camera moves. Off reacts a tiny bit faster but "
                           "may tear.",
                           "Wartet vor jedem Bild auf den Bildschirm, so rei\xC3\x9Ft das Bild bei "
                           "Kamerabewegungen nie in zwei Teile. Aus reagiert minimal schneller, "
                           "kann aber rei\xC3\x9F" "en."});
  page.settings.push_back(
      {"render_target_path_d3d12", "Render path", "Renderpfad",
       "Fast (RTV) is the one to use. Exact (ROV) copies the Xbox 360's graphics memory bit for "
       "bit and is much slower (about a third fewer frames); only for comparing graphics errors.",
       "Schnell (RTV) ist der richtige. Genau (ROV) bildet den Grafikspeicher der Xbox 360 Bit "
       "f\xC3\xBCr Bit nach und ist viel langsamer (rund ein Drittel weniger Bilder); nur zum "
       "Vergleichen bei Grafikfehlern.",
       {{"rtv", "Fast (RTV)", "Schnell (RTV)"}, {"rov", "Exact (ROV)", "Genau (ROV)"}}});
  page.settings.push_back(
      {"d3d12_adapter", "Graphics card", "Grafikkarte",
       "Which graphics card draws the game, for PCs with more than one.",
       "Welche Grafikkarte das Spiel zeichnet, f\xC3\xBCr PCs mit mehreren.",
       {{"-1", "Automatic", "Automatisch"}, {"0", "GPU 1", "GPU 1"}, {"1", "GPU 2", "GPU 2"}}});
  page.settings.push_back(
      {"gpu_backend", "Graphics interface", "Grafikschnittstelle",
       "D3D12 is the tested one and the only one with the render path above. Vulkan runs just as "
       "fast and is there to try if D3D12 causes trouble (crashes, graphics errors).",
       "D3D12 ist die getestete und die einzige mit dem Renderpfad oben. Vulkan l\xC3\xA4uft "
       "genauso schnell und ist zum Ausprobieren da, falls D3D12 Probleme macht (Abst\xC3\xBCrze, "
       "Grafikfehler).",
       {{"any", "Automatic (D3D12)", "Automatisch (D3D12)"}, {"d3d12", "D3D12", "D3D12"},
        {"vulkan", "Vulkan", "Vulkan"}}});
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
                "Skips the Activision and Vicarious Visions logo videos at start-up.",
                "\xC3\x9C" "berspringt die Logo-Videos von Activision und Vicarious Visions beim "
                "Start."};
  intro.restart = true;
  page.settings.push_back(intro);
  page.settings.push_back({"skip_opening_movie", "Skip the opening movie",
                           "Er\xC3\xB6" "ffnungsfilm \xC3\xBC" "berspringen",
                           "Skips the long movie at the start of a new game.",
                           "\xC3\x9C" "berspringt den langen Film beim Start eines neuen Spiels."});
  return page;
}

SettingsPage PortalPage() {
  SettingsPage page{"Portal", "Portal", {}};
  Setting mode{"portal_mode", "Portal", "Portal",
               "Virtual: the portal menu (Xbox button) puts figures on. Both: the menu and a real "
               "Portal of Power together, for the figures you have and the ones you don't; real "
               "figures are backed up in figures\\real before the game writes to them. USB only: "
               "just the real portal, also the ones from Spyro's Adventure and Giants.",
               "Virtuell: das Portal-Men\xC3\xBC (Xbox-Taste) stellt Figuren auf. Beides: das "
               "Men\xC3\xBC und ein echtes Portal of Power zusammen, f\xC3\xBCr die Figuren, die du "
               "hast, und die, die du nicht hast; echte Figuren werden in figures\\real gesichert, "
               "bevor das Spiel auf sie schreibt. Nur USB: nur das echte Portal, auch die von "
               "Spyro's Adventure und Giants.",
               {{"software", "Virtual (menu)", "Virtuell (Men\xC3\xBC)"},
                {"hybrid", "Both (menu + real portal)", "Beides (Men\xC3\xBC + echtes Portal)"},
                {"usb", "Real USB portal only", "Nur echtes USB-Portal"},
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
       "Which player the keyboard is. Controllers are assigned in the player menu (L3 + R3).",
       "Welcher Spieler die Tastatur ist. Controller teilst du im Spieler-Men\xC3\xBC zu (L3 + R3).",
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
      "The SDK's full list of every setting (F10). Its \"Save to config\" button rewrites the "
      "whole settings file and drops its comments; this menu saves only what you change.";
  all.help_de =
      "Die vollst\xC3\xA4ndige Liste aller Einstellungen des SDK (F10). Sein \"Save to config\" "
      "schreibt die ganze Einstellungsdatei neu und verwirft ihre Kommentare; dieses Men\xC3\xBC "
      "speichert nur, was du \xC3\xA4nderst.";
  all.action = std::move(open_all_settings);
  advanced.settings.push_back(std::move(all));
  pages.push_back(std::move(advanced));
  return pages;
}

}  // namespace swapforcerecomp
