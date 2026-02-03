#ifndef PROTON_DATA_TREE_DATA_H_
#define PROTON_DATA_TREE_DATA_H_

#include "Context/Context.h"
#include "Data.h"
#include "nlohmann/json.hpp"
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace proton {

class TreeData : public Data {
public:
  TreeData(const std::string &path, ContextSource *contextSource);
  virtual ~TreeData();

  TreeData(const std::string &path) : TreeData(path, nullptr) {}

  std::string toJsonString(size_t phase) const override;

  std::vector<uint8_t> toMsgPack(size_t phase) const override;

  DataEntry addOp(const std::string &name) override;

  DataEntry addOp(size_t phase, size_t contextId,
                  const std::vector<Context> &contexts) override;

  void
  addMetrics(size_t scopeId,
             const std::map<std::string, MetricValueType> &metrics) override;

  void
  addMetrics(size_t phase, size_t entryId,
             const std::map<std::string, MetricValueType> &metrics) override;

  // Tree class definition moved here for MSVC compatibility
  // (std::unique_ptr in PhaseStore requires complete type)
  class Tree {
  public:
    struct TreeNode : public Context {
      inline static const size_t RootId = 0;
      inline static const size_t DummyId = std::numeric_limits<size_t>::max();

      struct ChildEntry {
        std::string_view name;
        size_t id = DummyId;
      };

      TreeNode() = default;
      explicit TreeNode(size_t id, const std::string &name)
          : id(id), Context(name) {}
      TreeNode(size_t id, size_t parentId, const std::string &name)
          : id(id), parentId(parentId), Context(name) {}
      virtual ~TreeNode() = default;

      void addChild(std::string_view childName, size_t id) {
        children.push_back({childName, id});
        childIndex.emplace(childName, id);
      }

      size_t findChild(std::string_view childName) const {
        auto it = childIndex.find(childName);
        return it != childIndex.end() ? it->second : DummyId;
      }

      size_t parentId = DummyId;
      size_t id = DummyId;
      std::vector<ChildEntry> children = {};
      std::unordered_map<std::string_view, size_t> childIndex = {};
      std::map<MetricKind, std::unique_ptr<Metric>> metrics = {};
      std::map<std::string, FlexibleMetric> flexibleMetrics = {};
      friend class Tree;
    };

    Tree() {
      treeNodeMap.try_emplace(TreeNode::RootId, TreeNode::RootId,
                              TreeNode::RootId, "ROOT");
    }

    size_t addNode(const std::vector<Context> &contexts, size_t parentId) {
      for (const auto &context : contexts) {
        parentId = addNode(context, parentId);
      }
      return parentId;
    }

    size_t addNode(const Context &context, size_t parentId) {
      auto &parent = treeNodeMap.at(parentId);
      std::string_view contextName = context.name;
      auto existingChildId = parent.findChild(contextName);
      if (existingChildId != TreeNode::DummyId)
        return existingChildId;
      auto id = nextContextId++;
      auto [it, inserted] =
          treeNodeMap.try_emplace(id, id, parentId, context.name);
      parent.addChild(it->second.name, id);
      return id;
    }

    size_t addNode(const std::vector<Context> &indices) {
      auto parentId = TreeNode::RootId;
      for (auto index : indices) {
        parentId = addNode(index, parentId);
      }
      return parentId;
    }

    TreeNode &getNode(size_t id) { return treeNodeMap.at(id); }

    void upsertFlexibleMetric(size_t contextId,
                              const FlexibleMetric &flexibleMetric) {
      auto &node = treeNodeMap.at(contextId);
      auto it = node.flexibleMetrics.find(flexibleMetric.getValueName(0));
      if (it == node.flexibleMetrics.end()) {
        node.flexibleMetrics.emplace(flexibleMetric.getValueName(0),
                                     flexibleMetric);
      } else {
        it->second.updateMetric(flexibleMetric);
      }
    }

    enum class WalkPolicy { PreOrder, PostOrder };

    template <WalkPolicy walkPolicy, typename FnT> void walk(FnT &&fn) {
      if constexpr (walkPolicy == WalkPolicy::PreOrder) {
        walkPreOrder(TreeNode::RootId, fn);
      } else if constexpr (walkPolicy == WalkPolicy::PostOrder) {
        walkPostOrder(TreeNode::RootId, fn);
      }
    }

    template <typename FnT> void walkPreOrder(size_t contextId, FnT &&fn) {
      fn(getNode(contextId));
      for (const auto &child : getNode(contextId).children) {
        walkPreOrder(child.id, fn);
      }
    }

    template <typename FnT> void walkPostOrder(size_t contextId, FnT &&fn) {
      for (const auto &child : getNode(contextId).children) {
        walkPostOrder(child.id, fn);
      }
      fn(getNode(contextId));
    }

    size_t size() const { return nextContextId; }

  private:
    size_t nextContextId = TreeNode::RootId + 1;
    // tree node id -> tree node
    std::unordered_map<size_t, TreeNode> treeNodeMap;
  };

protected:
  // ScopeInterface
  void enterScope(const Scope &scope) override;

  void exitScope(const Scope &scope) override;

private:
  // `tree` and `scopeIdToContextId` can be accessed by both the user thread and
  // the background threads concurrently, so methods that access them should be
  // protected by a (shared) mutex.
  json buildHatchetJson(TreeData::Tree *tree) const;
  std::vector<uint8_t> buildHatchetMsgPack(TreeData::Tree *tree) const;

  // Data
  void doDump(std::ostream &os, OutputFormat outputFormat,
              size_t phase) const override;

  OutputFormat getDefaultOutputFormat() const override {
    return OutputFormat::Hatchet;
  }

  void dumpHatchet(std::ostream &os, size_t phase) const;
  void dumpHatchetMsgPack(std::ostream &os, size_t phase) const;

  PhaseStore<Tree> treePhases;
  // ScopeId -> ContextId
  std::unordered_map<size_t, size_t> scopeIdToContextId;
};

} // namespace proton

#endif // PROTON_DATA_TREE_DATA_H_
