#include "mimir/dag.h"
#include <algorithm>
#include <stack>

using namespace mimir;

bool DAG::addTarget(const Target& target)
{
    if (targets_.find(target.getName()) != targets_.end())
    {
        return false;
    }
    targets_[target.getName()] = std::make_shared<Target>(target);
    return true;
}

bool DAG::addTarget(Target&& target)
{
    std::string name = target.getName();
    if (targets_.find(name) != targets_.end())
    {
        return false;
    }
    targets_[name] = std::make_shared<Target>(std::move(target));
    return true;
}

bool DAG::addTarget(TargetPtr target)
{
    if (!target || targets_.find(target->getName()) != targets_.end())
    {
        return false;
    }
    targets_[target->getName()] = std::move(target);
    return true;
}

bool DAG::removeTarget(const std::string& name)
{
    return targets_.erase(name) > 0;
}

bool DAG::hasTarget(const std::string& name) const
{
    return targets_.find(name) != targets_.end();
}

bool DAG::hasCycleUtil(
    const std::string& node,
    std::unordered_map<std::string, int>& visited,
    std::vector<std::string>& recStack,
    std::vector<std::string>& cycleNodes) const
{
    visited[node] = 1;
    recStack.push_back(node);

    auto it = targets_.find(node);
    if (it != targets_.end())
    {
        for (const auto& dep : it->second->getDependencies())
        {
            if (visited[dep] == 0)
            {
                if (hasCycleUtil(dep, visited, recStack, cycleNodes))
                {
                    return true;
                }
            }
            else if (visited[dep] == 1)
            {
                // Found cycle - extract cycle path
                auto cycleStart = std::find(recStack.begin(), recStack.end(), dep);
                cycleNodes.assign(cycleStart, recStack.end());
                cycleNodes.push_back(dep);
                return true;
            }
        }
    }
    
    visited[node] = 2;
    recStack.pop_back();
    return false;
}

bool DAG::detectCycles() const
{
    auto result = detectCyclesWithPath();
    return result.hasCycle;
}

CycleDetectionResult DAG::detectCyclesWithPath() const
{
    CycleDetectionResult result{false, {}};
    std::unordered_map<std::string, int> visited;
    std::vector<std::string> recStack;
    
    for (const auto& [name, target] : targets_)
    {
        if (visited[name] == 0)
        {
            if (hasCycleUtil(name, visited, recStack, result.cycleNodes))
            {
                result.hasCycle = true;
                return result;
            }
        }
    }
    return result;
}

CycleDetectionResult DAG::detectCyclesWithResult() const
{
    return detectCyclesWithPath();
}

std::vector<std::string> DAG::topologicalSort() const
{
    std::unordered_map<std::string, int> inDegree;
    std::vector<std::string> result;
    
    // Initialize in-degree for all targets
    for (const auto& [name, target] : targets_)
    {
        if (inDegree.find(name) == inDegree.end())
        {
            inDegree[name] = 0;
        }
    }
    
    // Calculate in-degree based on dependencies
    for (const auto& [name, target] : targets_)
    {
        for (const auto& dep : target->getDependencies())
        {
            inDegree[name]++;
        }
    }
    
    // Find all nodes with zero in-degree
    std::vector<std::string> queue;
    for (const auto& [name, degree] : inDegree)
    {
        if (degree == 0)
        {
            queue.push_back(name);
        }
    }
    
    // Process queue
    size_t idx = 0;
    while (idx < queue.size())
    {
        std::string current = queue[idx++];
        result.push_back(current);
        
        // Reduce in-degree for dependents
        for (const auto& [name, target] : targets_)
        {
            const auto& deps = target->getDependencies();
            if (std::find(deps.begin(), deps.end(), current) != deps.end())
            {
                inDegree[name]--;
                if (inDegree[name] == 0)
                {
                    queue.push_back(name);
                }
            }
        }
    }
    
    return result;
}

const Target* DAG::getTarget(const std::string& name) const
{
    auto it = targets_.find(name);
    return it != targets_.end() ? it->second.get() : nullptr;
}

Target* DAG::getTarget(const std::string& name)
{
    auto it = targets_.find(name);
    return it != targets_.end() ? it->second.get() : nullptr;
}

TargetPtr DAG::getTargetPtr(const std::string& name) const
{
    auto it = targets_.find(name);
    return it != targets_.end() ? it->second : nullptr;
}

std::vector<const Target*> DAG::getAllTargets() const
{
    std::vector<const Target*> result;
    result.reserve(targets_.size());
    for (const auto& [name, target] : targets_)
    {
        result.push_back(target.get());
    }
    return result;
}

std::vector<TargetPtr> DAG::getAllTargetPtrs() const
{
    std::vector<TargetPtr> result;
    result.reserve(targets_.size());
    for (const auto& [name, target] : targets_)
    {
        result.push_back(target);
    }
    return result;
}

size_t DAG::size() const noexcept
{
    return targets_.size();
}

bool DAG::empty() const noexcept
{
    return targets_.empty();
}

void DAG::clear()
{
    targets_.clear();
}

std::vector<std::string> DAG::getDependencies(const std::string& name) const
{
    auto it = targets_.find(name);
    if (it != targets_.end())
    {
        return it->second->getDependencies();
    }
    return {};
}

std::vector<std::string> DAG::getDependents(const std::string& name) const
{
    std::vector<std::string> dependents;
    for (const auto& [targetName, target] : targets_)
    {
        const auto& deps = target->getDependencies();
        if (std::find(deps.begin(), deps.end(), name) != deps.end())
        {
            dependents.push_back(targetName);
        }
    }
    return dependents;
}

std::vector<std::string> DAG::validateDependencies() const
{
    std::vector<std::string> missing;
    for (const auto& [name, target] : targets_)
    {
        for (const auto& dep : target->getDependencies())
        {
            if (targets_.find(dep) == targets_.end())
            {
                missing.push_back(dep);
            }
        }
    }
    return missing;
}