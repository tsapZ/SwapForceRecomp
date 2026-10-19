// Finds which game function writes a guest address, without a debugger (input script `watch` /
// `unwatch`, src/dev/input_script.h). A hardware write breakpoint (debug register 0) goes on every
// thread of the process; a vectored exception handler notes where each write came from, and
// StopWriteWatch names those places by guest function through PPCFuncMappings (host function
// containing the instruction -> its guest address). Developer tool for reverse engineering;
// Windows only.

#pragma once

#include <cstdint>
#include <optional>

namespace rex::memory {
class Memory;
}

namespace swapforcerecomp {

// Watches 4 bytes at a virtual guest address; with `value`, only writes that leave that
// big-endian word there count. Each hit keeps the call chain (host frames, named by guest function
// in the log). Threads started later are not covered.
void StartWriteWatch(rex::memory::Memory* memory, uint32_t guest_address,
                     std::optional<uint32_t> value = std::nullopt);

// Removes the breakpoint and logs every writer seen since StartWriteWatch.
void StopWriteWatch();

}  // namespace swapforcerecomp
