#include "input_script.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include <rex/cvar.h>
#include <rex/input/input_driver.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>

REXCVAR_DEFINE_STRING(input_script, "", "Input",
                      "Developer option: play controller input from this script file (see "
                      "src/dev/input_script.h), for automated test runs")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace swapforcerecomp {
namespace {

using Clock = std::chrono::steady_clock;
using rex::X_RESULT;
using rex::X_STATUS;

constexpr auto kScriptDevice = static_cast<rex::input::DeviceId>(0x53435200);  // "SCR"

struct Event {
  enum class Kind { kPress, kStick, kLog, kFps, kRenderDoc, kQuit } kind;
  double start = 0.0;     // seconds since the timeline started
  double duration = 0.0;  // seconds, for presses and stick moves
  uint16_t buttons = 0;
  bool left_trigger = false;
  bool right_trigger = false;
  double x = 0.0, y = 0.0;
  std::string text;
  uint32_t frames = 0;  // renderdoc: guest frames to capture
};

bool ParseButton(const std::string& name, Event& e) {
  static const struct {
    const char* name;
    uint16_t bit;
  } kButtons[] = {
      {"a", rex::input::X_INPUT_GAMEPAD_A},
      {"b", rex::input::X_INPUT_GAMEPAD_B},
      {"x", rex::input::X_INPUT_GAMEPAD_X},
      {"y", rex::input::X_INPUT_GAMEPAD_Y},
      {"start", rex::input::X_INPUT_GAMEPAD_START},
      {"back", rex::input::X_INPUT_GAMEPAD_BACK},
      {"up", rex::input::X_INPUT_GAMEPAD_DPAD_UP},
      {"down", rex::input::X_INPUT_GAMEPAD_DPAD_DOWN},
      {"left", rex::input::X_INPUT_GAMEPAD_DPAD_LEFT},
      {"right", rex::input::X_INPUT_GAMEPAD_DPAD_RIGHT},
      {"lb", rex::input::X_INPUT_GAMEPAD_LEFT_SHOULDER},
      {"rb", rex::input::X_INPUT_GAMEPAD_RIGHT_SHOULDER},
      {"ls", rex::input::X_INPUT_GAMEPAD_LEFT_THUMB},
      {"rs", rex::input::X_INPUT_GAMEPAD_RIGHT_THUMB},
  };
  if (name == "lt") {
    e.left_trigger = true;
    return true;
  }
  if (name == "rt") {
    e.right_trigger = true;
    return true;
  }
  for (const auto& b : kButtons) {
    if (name == b.name) {
      e.buttons = b.bit;
      return true;
    }
  }
  return false;
}

// Returns false (and logs the line) on a syntax error, so a typo doesn't silently shift the
// whole rest of the timeline.
bool ParseScript(std::istream& in, std::vector<Event>& events) {
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  for (char& c : text) {
    if (c == ';') c = '\n';
  }
  std::istringstream lines(text);
  std::string line;
  double t = 0.0;
  while (std::getline(lines, line)) {
    if (auto hash = line.find('#'); hash != std::string::npos) line.erase(hash);
    std::istringstream words(line);
    std::string cmd;
    if (!(words >> cmd)) continue;
    Event e{};
    e.start = t;
    bool ok = true;
    if (cmd == "wait") {
      double seconds = 0.0;
      ok = bool(words >> seconds);
      t += seconds;
      continue;
    } else if (cmd == "tap" || cmd == "hold") {
      std::string button;
      double ms = 120.0;
      ok = bool(words >> button) && ParseButton(button, e);
      if (cmd == "hold") ok = ok && bool(words >> ms);
      e.kind = Event::Kind::kPress;
      e.duration = ms / 1000.0;
    } else if (cmd == "stick") {
      double ms = 0.0;
      ok = bool(words >> e.x >> e.y >> ms);
      e.kind = Event::Kind::kStick;
      e.duration = ms / 1000.0;
    } else if (cmd == "log") {
      std::getline(words >> std::ws, e.text);
      e.kind = Event::Kind::kLog;
    } else if (cmd == "fps") {
      ok = bool(words >> e.duration) && e.duration > 0.0;
      e.kind = Event::Kind::kFps;
    } else if (cmd == "renderdoc") {
      e.kind = Event::Kind::kRenderDoc;
      e.frames = 1;
      if (words >> e.frames) ok = e.frames > 0;
    } else if (cmd == "quit") {
      e.kind = Event::Kind::kQuit;
    } else {
      ok = false;
    }
    if (!ok) {
      REXLOG_ERROR("Input script: cannot parse '{}'", line);
      return false;
    }
    events.push_back(e);
    t += e.duration;
  }
  return true;
}

// Captures `frames` whole guest frames with an injected RenderDoc. RenderDoc's own triggers capture
// host frames, but the host presents more often than the game renders and one guest frame's GPU
// work is spread over several of them (the lighting of a frame can end up in a different capture
// than its G-buffer). So the capture is started and ended by hand when the guest frame counter
// (advanced by the GPU thread on each guest swap) moves on.
// RenderDoc's in-application API (renderdoc_app.h) is an array of function pointers whose layout
// later versions only extend: StartFrameCapture is entry 19, EndFrameCapture entry 21.
void CaptureGuestFrames(uint32_t frames, const std::function<uint64_t()>& guest_frame_count) {
#ifdef _WIN32
  HMODULE module = GetModuleHandleA("renderdoc.dll");
  if (!module) {
    REXLOG_WARN("Input script: renderdoc requested, but RenderDoc is not attached");
    return;
  }
  using GetApi = int (*)(int version, void** api);
  auto get_api = reinterpret_cast<GetApi>(GetProcAddress(module, "RENDERDOC_GetAPI"));
  void** api = nullptr;
  if (!get_api || !get_api(10000, reinterpret_cast<void**>(&api)) || !api) {
    REXLOG_WARN("Input script: RenderDoc API 1.0.0 not available");
    return;
  }
  using StartFrameCapture = void (*)(void* device, void* window);
  using EndFrameCapture = uint32_t (*)(void* device, void* window);
  auto wait_for_frame = [&](uint64_t after) {
    uint64_t now;
    while ((now = guest_frame_count()) <= after) {
      std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    return now;
  };
  const uint64_t first = wait_for_frame(guest_frame_count());
  reinterpret_cast<StartFrameCapture>(api[19])(nullptr, nullptr);
  const uint64_t last = wait_for_frame(first + frames - 1);
  const uint32_t ok = reinterpret_cast<EndFrameCapture>(api[21])(nullptr, nullptr);
  REXLOG_INFO("Input script: RenderDoc capture of guest frames {}-{} {}", first, last,
              ok ? "saved" : "failed");
#else
  (void)frames;
  (void)guest_frame_count;
  REXLOG_WARN("Input script: renderdoc is only supported on Windows");
#endif
}

class ScriptInputDriver final : public rex::input::InputDriver {
 public:
  ScriptInputDriver(std::vector<Event> events, std::function<uint64_t()> guest_frame_count)
      : InputDriver(nullptr, 0),
        events_(std::move(events)),
        guest_frame_count_(std::move(guest_frame_count)),
        start_(Clock::now()) {}

  X_STATUS Setup() override {
    // Logs, captures and quit fire at their time even while the game polls no input. The thread
    // works on copies and is detached, so it never outlives what it uses.
    std::thread([events = events_, start = start_, guest_frame_count = guest_frame_count_] {
      for (const Event& e : events) {
        if (e.kind != Event::Kind::kLog && e.kind != Event::Kind::kFps &&
            e.kind != Event::Kind::kRenderDoc && e.kind != Event::Kind::kQuit) {
          continue;
        }
        std::this_thread::sleep_until(start + std::chrono::duration_cast<Clock::duration>(
                                                  std::chrono::duration<double>(e.start)));
        if (e.kind == Event::Kind::kLog) {
          REXLOG_INFO("Input script: {}", e.text);
        } else if (e.kind == Event::Kind::kFps) {
          const uint64_t frames_before = guest_frame_count();
          const auto time_before = Clock::now();
          std::this_thread::sleep_for(std::chrono::duration<double>(e.duration));
          const uint64_t frames = guest_frame_count() - frames_before;
          const double seconds = std::chrono::duration<double>(Clock::now() - time_before).count();
          REXLOG_INFO("Input script: {:.1f} guest FPS ({} frames in {:.2f} s)",
                      double(frames) / seconds, frames, seconds);
        } else if (e.kind == Event::Kind::kRenderDoc) {
          CaptureGuestFrames(e.frames, guest_frame_count);
        } else {
          REXLOG_INFO("Input script: quit");
          std::_Exit(0);
        }
      }
    }).detach();
    return X_STATUS_SUCCESS;
  }

  void EnumerateDevices(std::vector<rex::input::DeviceInfo>& out) override {
    rex::input::DeviceInfo info;
    info.id = kScriptDevice;
    info.name = "Input script";
    info.synthetic = true;
    out.push_back(info);
  }

  X_RESULT GetDeviceState(rex::input::DeviceId id, rex::input::X_INPUT_STATE* out_state) override {
    if (id != kScriptDevice) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (!out_state) return X_ERROR_SUCCESS;
    const double now = std::chrono::duration<double>(Clock::now() - start_).count();
    uint16_t buttons = 0;
    bool lt = false, rt = false;
    double x = 0.0, y = 0.0;
    for (const Event& e : events_) {
      if (now < e.start || now >= e.start + e.duration) continue;
      if (e.kind == Event::Kind::kPress) {
        buttons |= e.buttons;
        lt |= e.left_trigger;
        rt |= e.right_trigger;
      } else if (e.kind == Event::Kind::kStick) {
        x = e.x;
        y = e.y;
      }
    }
    std::memset(out_state, 0, sizeof(*out_state));
    out_state->packet_number = ++packet_number_;
    out_state->gamepad.buttons = buttons;
    out_state->gamepad.left_trigger = lt ? 0xFF : 0;
    out_state->gamepad.right_trigger = rt ? 0xFF : 0;
    out_state->gamepad.thumb_lx = static_cast<int16_t>(x * 32767.0);
    out_state->gamepad.thumb_ly = static_cast<int16_t>(y * 32767.0);
    return X_ERROR_SUCCESS;
  }

  X_RESULT GetDeviceCapabilities(rex::input::DeviceId id, uint32_t,
                                 rex::input::X_INPUT_CAPABILITIES* out_caps) override {
    if (id != kScriptDevice) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (out_caps) {
      std::memset(out_caps, 0, sizeof(*out_caps));
      out_caps->type = 0x01;
      out_caps->sub_type = 0x01;
      out_caps->gamepad.buttons = 0xFFFF;
      out_caps->gamepad.left_trigger = 0xFF;
      out_caps->gamepad.right_trigger = 0xFF;
      out_caps->gamepad.thumb_lx = static_cast<int16_t>(0x7FFF);
      out_caps->gamepad.thumb_ly = static_cast<int16_t>(0x7FFF);
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT SetDeviceVibration(rex::input::DeviceId id, rex::input::X_INPUT_VIBRATION*) override {
    return id == kScriptDevice ? X_ERROR_SUCCESS : X_ERROR_DEVICE_NOT_CONNECTED;
  }

  X_RESULT GetDeviceKeystroke(rex::input::DeviceId id, uint32_t,
                              rex::input::X_INPUT_KEYSTROKE*) override {
    return id == kScriptDevice ? X_ERROR_EMPTY : X_ERROR_DEVICE_NOT_CONNECTED;
  }

 private:
  std::vector<Event> events_;
  std::function<uint64_t()> guest_frame_count_;
  Clock::time_point start_;
  uint32_t packet_number_ = 0;
};

}  // namespace

void InstallInputScript(rex::system::IInputSystem* input_system,
                        std::function<uint64_t()> guest_frame_count) {
  const std::string path = REXCVAR_GET(input_script);
  if (path.empty()) return;
  std::ifstream file(path);
  std::vector<Event> events;
  if (!file || !ParseScript(file, events)) {
    REXLOG_ERROR("Input script: cannot use '{}'", path);
    return;
  }
  auto* input = dynamic_cast<rex::input::InputSystem*>(input_system);
  if (!input) {
    REXLOG_ERROR("Input script: no input system");
    return;
  }
  auto driver = std::make_unique<ScriptInputDriver>(std::move(events), std::move(guest_frame_count));
  driver->Setup();
  input->AddDriver(std::move(driver));
  REXLOG_INFO("Input script: playing '{}'", path);
}

}  // namespace swapforcerecomp
