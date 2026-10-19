#include "write_watch.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

#include <rex/logging.h>
#include <rex/ppc/func.h>
#include <rex/system/xmemory.h>

namespace swapforcerecomp {
namespace {

#ifdef _WIN32

// The call chain of each hit (the instruction after the write first), written by the exception
// handler (no locks or logging there: it runs inside whatever game code did the write).
constexpr size_t kMaxHits = 2048;
constexpr int kFrames = 6;
struct Hit {
  uintptr_t frames[kFrames];
};
std::array<Hit, kMaxHits> g_hits{};
std::atomic<size_t> g_hit_count{0};
std::atomic<size_t> g_write_count{0};
std::atomic<uintptr_t> g_watched{0};
std::atomic<bool> g_filter{false};
std::atomic<uint32_t> g_value{0};  // as the bytes read little-endian
PVOID g_handler = nullptr;
uint32_t g_guest_address = 0;

// Walks the stack from the exception context with the binary's own unwind data (no symbols needed).
void Unwind(CONTEXT ctx, uintptr_t* frames) {
  for (int i = 0; i < kFrames; ++i) {
    frames[i] = uintptr_t(ctx.Rip);
    DWORD64 image_base = 0;
    PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(ctx.Rip, &image_base, nullptr);
    if (!fn) {
      // A leaf function: the return address is on top of the stack.
      ctx.Rip = *reinterpret_cast<DWORD64*>(ctx.Rsp);
      ctx.Rsp += 8;
    } else {
      PVOID handler_data = nullptr;
      DWORD64 establisher = 0;
      RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, ctx.Rip, fn, &ctx, &handler_data,
                       &establisher, nullptr);
    }
    if (!ctx.Rip) {
      for (int k = i + 1; k < kFrames; ++k) frames[k] = 0;
      return;
    }
  }
}

LONG CALLBACK OnException(PEXCEPTION_POINTERS info) {
  if (info->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) return EXCEPTION_CONTINUE_SEARCH;
  CONTEXT* ctx = info->ContextRecord;
  const uintptr_t watched = g_watched.load();
  if (!(ctx->Dr6 & 1) || !watched) return EXCEPTION_CONTINUE_SEARCH;
  ctx->Dr6 = 0;
  g_write_count.fetch_add(1);
  if (g_filter.load() && *reinterpret_cast<const uint32_t*>(watched) != g_value.load()) {
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  const size_t i = g_hit_count.fetch_add(1);
  if (i < kMaxHits) Unwind(*ctx, g_hits[i].frames);
  return EXCEPTION_CONTINUE_EXECUTION;
}

// Sets (address != 0) or clears debug register 0 on every other thread of the process.
void SetBreakpointOnAllThreads(uintptr_t address) {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot == INVALID_HANDLE_VALUE) return;
  const DWORD pid = GetCurrentProcessId(), self = GetCurrentThreadId();
  THREADENTRY32 entry{};
  entry.dwSize = sizeof(entry);
  int threads = 0;
  for (BOOL ok = Thread32First(snapshot, &entry); ok; ok = Thread32Next(snapshot, &entry)) {
    if (entry.th32OwnerProcessID != pid || entry.th32ThreadID == self) continue;
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME,
                               FALSE, entry.th32ThreadID);
    if (!thread) continue;
    if (SuspendThread(thread) != DWORD(-1)) {
      CONTEXT ctx{};
      ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
      if (GetThreadContext(thread, &ctx)) {
        if (address) {
          ctx.Dr0 = address;
          // L0 on, RW0 = 01 (write), LEN0 = 11 (4 bytes).
          ctx.Dr7 = (ctx.Dr7 & ~DWORD64(0xF0003)) | 1 | (1u << 16) | (3u << 18);
        } else {
          ctx.Dr0 = 0;
          ctx.Dr7 &= ~DWORD64(0xF0003);
        }
        if (SetThreadContext(thread, &ctx)) ++threads;
      }
      ResumeThread(thread);
    }
    CloseHandle(thread);
  }
  CloseHandle(snapshot);
  REXLOG_INFO("Write watch: debug register {} on {} threads", address ? "set" : "cleared", threads);
}

// The guest function whose host code contains `rip`: the mapping with the highest host address
// not above it.
std::pair<uint32_t, uintptr_t> GuestFunctionAt(uintptr_t rip) {
  static std::vector<std::pair<uintptr_t, uint32_t>> table;
  if (table.empty()) {
    for (const PPCFuncMapping* m = PPCFuncMappings; m->host; ++m) {
      table.emplace_back(reinterpret_cast<uintptr_t>(m->host), uint32_t(m->guest));
    }
    std::sort(table.begin(), table.end());
  }
  auto it = std::upper_bound(table.begin(), table.end(), std::make_pair(rip, UINT32_MAX));
  if (it == table.begin()) return {0, 0};
  --it;
  return {it->second, rip - it->first};
}

#endif  // _WIN32

}  // namespace

void StartWriteWatch(rex::memory::Memory* memory, uint32_t guest_address,
                     std::optional<uint32_t> value) {
#ifdef _WIN32
  if (!memory || g_watched.load()) return;
  const auto host = reinterpret_cast<uintptr_t>(memory->TranslateVirtual<uint8_t*>(guest_address));
  g_guest_address = guest_address;
  g_hit_count.store(0);
  g_write_count.store(0);
  g_filter.store(value.has_value());
  // Guest memory is big-endian; the handler compares the raw bytes.
  if (value) g_value.store(_byteswap_ulong(*value));
  if (!g_handler) g_handler = AddVectoredExceptionHandler(1, OnException);
  g_watched.store(host);
  SetBreakpointOnAllThreads(host);
  REXLOG_INFO("Write watch: watching guest {:08X} (host {:X})", guest_address, host);
#else
  (void)memory;
  (void)guest_address;
  (void)value;
  REXLOG_WARN("Write watch: only on Windows");
#endif
}

void StopWriteWatch() {
#ifdef _WIN32
  if (!g_watched.load()) return;
  SetBreakpointOnAllThreads(0);
  g_watched.store(0);
  const size_t count = std::min(g_hit_count.load(), kMaxHits);
  // Each distinct call chain once, as guest functions.
  std::map<std::string, size_t> chains;
  for (size_t i = 0; i < count; ++i) {
    std::string chain;
    for (uintptr_t frame : g_hits[i].frames) {
      if (!frame) break;
      const auto [guest, offset] = GuestFunctionAt(frame);
      chain += fmt::format("{}sub_{:08X}+0x{:X}", chain.empty() ? "" : " <- ", guest, offset);
    }
    ++chains[chain];
  }
  REXLOG_INFO("Write watch: {} writes to {:08X}, {} counted, {} call chains", g_write_count.load(),
              g_guest_address, g_hit_count.load(), chains.size());
  for (const auto& [chain, n] : chains) REXLOG_INFO("Write watch:   {} x {}", n, chain);
#endif
}

}  // namespace swapforcerecomp
