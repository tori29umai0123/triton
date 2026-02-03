#ifndef PROTON_DATA_TRACE_DATA_H_
#define PROTON_DATA_TRACE_DATA_H_

#include "Data.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>

namespace proton {

class TraceData : public Data {
public:
  TraceData(const std::string &path, ContextSource *contextSource = nullptr);
  virtual ~TraceData();

  std::string toJsonString(size_t phase) const override;

  std::vector<uint8_t> toMsgPack(size_t phase) const override;

  DataEntry addOp(const std::string &name) override;

  DataEntry addOp(size_t phase, size_t eventId,
                  const std::vector<Context> &contexts) override;

  void
  addMetrics(size_t scopeId,
             const std::map<std::string, MetricValueType> &metrics) override;

  void
  addMetrics(size_t phase, size_t entryId,
             const std::map<std::string, MetricValueType> &metrics) override;

  // Trace class definition moved here for MSVC compatibility
  // (std::unique_ptr in PhaseStore requires complete type)
  class Trace {
  public:
    struct TraceContext : public Context {
      inline static const size_t RootId = 0;
      inline static const size_t DummyId = std::numeric_limits<size_t>::max();

      TraceContext() = default;
      explicit TraceContext(size_t id, const std::string &name)
          : id(id), Context(name) {}
      TraceContext(size_t id, size_t parentId, const std::string &name)
          : id(id), parentId(parentId), Context(name) {}
      virtual ~TraceContext() = default;

      void addChild(const Context &context, size_t id) { children[context] = id; }

      bool hasChild(const Context &context) const {
        return children.find(context) != children.end();
      }

      size_t getChild(const Context &context) const {
        return children.at(context);
      }

      size_t getParent() const { return parentId; }

      size_t parentId = DummyId;
      size_t id = DummyId;
      std::map<Context, size_t> children = {};
      friend class Trace;
    };

    struct TraceEvent {
      TraceEvent() = default;
      TraceEvent(size_t id, size_t contextId) : id(id), contextId(contextId) {}
      size_t id = 0;
      size_t scopeId = Scope::DummyScopeId;
      size_t contextId = TraceContext::DummyId;
      std::map<MetricKind, std::unique_ptr<Metric>> metrics = {};
      std::map<std::string, FlexibleMetric> flexibleMetrics = {};

      const static inline size_t DummyId = std::numeric_limits<size_t>::max();
    };

    Trace() {
      traceContextMap.try_emplace(TraceContext::RootId, TraceContext::RootId,
                                  "ROOT");
    }

    size_t addContext(const Context &context, size_t parentId) {
      if (traceContextMap[parentId].hasChild(context)) {
        return traceContextMap[parentId].getChild(context);
      }
      auto id = nextTreeContextId++;
      traceContextMap.try_emplace(id, id, parentId, context.name);
      traceContextMap[parentId].addChild(context, id);
      return id;
    }

    size_t addContexts(const std::vector<Context> &contexts, size_t parentId) {
      for (const auto &context : contexts) {
        parentId = addContext(context, parentId);
      }
      return parentId;
    }

    size_t addContexts(const std::vector<Context> &indices) {
      auto parentId = TraceContext::RootId;
      for (auto index : indices) {
        parentId = addContext(index, parentId);
      }
      return parentId;
    }

    std::vector<Context> getContexts(size_t contextId) {
      std::vector<Context> contexts;
      auto it = traceContextMap.find(contextId);
      if (it == traceContextMap.end()) {
        throw std::runtime_error("Context not found");
      }
      std::reference_wrapper<TraceContext> context = it->second;
      contexts.push_back(context.get());
      while (context.get().parentId != TraceContext::DummyId) {
        context = traceContextMap[context.get().parentId];
        contexts.push_back(context.get());
      }
      std::reverse(contexts.begin(), contexts.end());
      return contexts;
    }

    size_t addEvent(size_t contextId) {
      traceEvents.emplace(nextEventId, TraceEvent(nextEventId, contextId));
      return nextEventId++;
    }

    bool hasEvent(size_t eventId) {
      return traceEvents.find(eventId) != traceEvents.end();
    }

    TraceEvent &getEvent(size_t eventId) {
      auto it = traceEvents.find(eventId);
      if (it == traceEvents.end()) {
        throw std::runtime_error("Event not found");
      }
      return it->second;
    }

    void removeEvent(size_t eventId) { traceEvents.erase(eventId); }

    const std::map<size_t, TraceEvent> &getEvents() const { return traceEvents; }

  private:
    size_t nextTreeContextId = TraceContext::RootId + 1;
    size_t nextEventId = 0;
    std::map<size_t, TraceEvent> traceEvents;
    // tree node id -> trace context
    std::map<size_t, TraceContext> traceContextMap;
  };

protected:
  // ScopeInterface
  void enterScope(const Scope &scope) override final;

  void exitScope(const Scope &scope) override final;

private:
  // Data
  void doDump(std::ostream &os, OutputFormat outputFormat,
              size_t phase) const override;

  OutputFormat getDefaultOutputFormat() const override {
    return OutputFormat::ChromeTrace;
  }

  void dumpChromeTrace(std::ostream &os, size_t phase) const;

  PhaseStore<Trace> tracePhases;
  // ScopeId -> EventId
  std::unordered_map<size_t, size_t> scopeIdToEventId;
};

} // namespace proton

#endif // PROTON_DATA_TRACE_DATA_H_
