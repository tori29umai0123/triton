#ifndef PROTON_DRIVER_GPU_HSA_API_H_
#define PROTON_DRIVER_GPU_HSA_API_H_

#include "Device.h"

#ifndef _WIN32
// HSA is only supported on Linux (AMD ROCm)

#include "hsa/hsa_ext_amd.h"

namespace proton {

namespace hsa {

template <bool CheckSuccess>
hsa_status_t agentGetInfo(hsa_agent_t agent, hsa_agent_info_t attribute,
                          void *value);

hsa_status_t iterateAgents(hsa_status_t (*callback)(hsa_agent_t agent,
                                                    void *data),
                           void *data);

} // namespace hsa

} // namespace proton

#endif // _WIN32

#endif // PROTON_DRIVER_GPU_HSA_API_H_
