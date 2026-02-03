#include "Driver/GPU/NvtxApi.h"
#include "Driver/GPU/CuptiApi.h"

#include <cstdint>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace proton {

namespace {

// Declare nvtx function params without including the nvtx header
struct RangePushAParams {
  const char *message;
};

} // namespace

namespace nvtx {

void enable() {
  // Get cupti lib path and append it to NVTX_INJECTION64_PATH
  const std::string cuptiLibPath =
      Dispatch<cupti::ExternLibCupti>::getLibPath();
  if (!cuptiLibPath.empty()) {
#ifdef _WIN32
    SetEnvironmentVariableA("NVTX_INJECTION64_PATH", cuptiLibPath.c_str());
#else
    setenv("NVTX_INJECTION64_PATH", cuptiLibPath.c_str(), 1);
#endif
  }
}

void disable() {
#ifdef _WIN32
  SetEnvironmentVariableA("NVTX_INJECTION64_PATH", NULL);
#else
  unsetenv("NVTX_INJECTION64_PATH");
#endif
}

std::string getMessageFromRangePushA(const void *params) {
  if (const auto *p = static_cast<const RangePushAParams *>(params))
    return std::string(p->message ? p->message : "");
  return "";
}

} // namespace nvtx

} // namespace proton
