#ifndef PROTON_PROFILER_ROCTRACER_PROFILER_H_
#define PROTON_PROFILER_ROCTRACER_PROFILER_H_

#include "Profiler/GPUProfiler.h"
#include <stdexcept>

namespace proton {

#ifndef _WIN32
// Roctracer is only available on Linux (AMD ROCm)

class RoctracerProfiler : public GPUProfiler<RoctracerProfiler> {
public:
  RoctracerProfiler();
  virtual ~RoctracerProfiler();

private:
  struct RoctracerProfilerPimpl;

  virtual void
  doSetMode(const std::vector<std::string> &modeAndOptions) override;
};

#else // _WIN32

// Stub class for Windows - roctracer is not supported
class RoctracerProfiler : public Profiler, public Singleton<RoctracerProfiler> {
public:
  RoctracerProfiler() = default;
  virtual ~RoctracerProfiler() = default;

protected:
  void doStart() override {
    throw std::runtime_error("Roctracer is not supported on Windows");
  }
  void doFlush() override {
    throw std::runtime_error("Roctracer is not supported on Windows");
  }
  void doStop() override {
    throw std::runtime_error("Roctracer is not supported on Windows");
  }
  void doSetMode(const std::vector<std::string> &modeAndOptions) override {
    throw std::runtime_error("Roctracer is not supported on Windows");
  }
  void doAddMetrics(
      size_t scopeId,
      const std::map<std::string, MetricValueType> &scalarMetrics,
      const std::map<std::string, TensorMetric> &tensorMetrics) override {
    throw std::runtime_error("Roctracer is not supported on Windows");
  }
};

#endif // _WIN32

} // namespace proton

#endif // PROTON_PROFILER_ROCTRACER_PROFILER_H_
