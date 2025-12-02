#pragma once

#include "target.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

/// @brief Directed Acyclic Graph for managing build targets and their dependencies \namespace mimir
namespace mimir
{
    /// @brief Result of cycle detection containing cycle path if found
    struct CycleDetectionResult
    {
        bool hasCycle;                      ///< True if a cycle was detected
        std::vector<std::string> cycleNodes; ///< Nodes involved in the cycle
    };

    /// @brief Directed Acyclic Graph (DAG) for build targets \class DAG
    class DAG
    {
    public:
        /**
        * @brief Default constructor
        */
        DAG() = default;

        /**
        * @brief Destructor
        */
        ~DAG() = default;

        /**
        * @brief DAG is copyable
        */
        DAG(const DAG&) = default;

        /**
        * @brief DAG is copy-assignable
        */
        DAG& operator=(const DAG&) = default;

        /**
        * @brief DAG is movable
        */
        DAG(DAG&&) noexcept = default;

        /**
        * @brief DAG is move-assignable
        */
        DAG& operator=(DAG&&) noexcept = default;

        /**
        * @brief Add a target to the DAG by copying
        * @param target The target to add
        * @return True if the target was added, false if a target with that name exists
        */
        bool addTarget(const Target& target);

        /**
        * @brief Add a target to the DAG by moving
        * @param target The target to move into the DAG
        * @return True if the target was added, false if a target with that name exists
        */
        bool addTarget(Target&& target);

        /**
        * @brief Add a shared target to the DAG
        * @param target Shared pointer to the target
        * @return True if the target was added, false if a target with that name exists
        */
        bool addTarget(TargetPtr target);

        /**
        * @brief Remove a target from the DAG
        * @param name Name of the target to remove
        * @return True if the target was found and removed
        */
        bool removeTarget(const std::string& name);

        /**
        * @brief Check if a target exists in the DAG
        * @param name Name of the target to check
        * @return True if the target exists
        */
        bool hasTarget(const std::string& name) const;

        /**
        * @brief Detect cycles in the DAG
        * @return True if a cycle is detected, false otherwise
        */
        bool detectCycles() const;

        /**
        * @brief Detect cycles and return the cycle path if found
        * @return CycleDetectionResult containing cycle information
        */
        CycleDetectionResult detectCyclesWithPath() const;

        /**
        * @brief Detect cycles with detailed result (alias for detectCyclesWithPath)
        * @return CycleDetectionResult containing cycle information
        */
        CycleDetectionResult detectCyclesWithResult() const;

        /**
        * @brief Perform topological sort on the DAG
        * @return Vector of target names in topologically sorted order (dependencies first)
        * @note Returns empty vector if a cycle is detected
        */
        std::vector<std::string> topologicalSort() const;

        /**
        * @brief Get a target by name (const version)
        * @param name The name of the target
        * @return Const pointer to the Target, or nullptr if not found
        */
        const Target* getTarget(const std::string& name) const;

        /**
        * @brief Get a target by name (mutable version)
        * @param name The name of the target
        * @return Pointer to the Target, or nullptr if not found
        */
        Target* getTarget(const std::string& name);

        /**
        * @brief Get a shared pointer to a target
        * @param name The name of the target
        * @return Shared pointer to the Target, or nullptr if not found
        */
        TargetPtr getTargetPtr(const std::string& name) const;

        /**
        * @brief Get all targets in the DAG
        * @return Vector of const pointers to all targets
        */
        std::vector<const Target*> getAllTargets() const;

        /**
        * @brief Get all target shared pointers
        * @return Vector of shared pointers to all targets
        */
        std::vector<TargetPtr> getAllTargetPtrs() const;

        /**
        * @brief Get the number of targets in the DAG
        * @return Number of targets
        */
        size_t size() const noexcept;

        /**
        * @brief Check if the DAG is empty
        * @return True if no targets exist
        */
        bool empty() const noexcept;

        /**
        * @brief Remove all targets from the DAG
        */
        void clear();

        /**
        * @brief Get all direct dependencies of a target
        * @param name Name of the target
        * @return Vector of dependency target names, or empty if target not found
        */
        std::vector<std::string> getDependencies(const std::string& name) const;

        /**
        * @brief Get all targets that depend on a given target
        * @param name Name of the target
        * @return Vector of dependent target names
        */
        std::vector<std::string> getDependents(const std::string& name) const;

        /**
        * @brief Validate that all dependencies exist in the DAG
        * @return Vector of missing dependency names (empty if all valid)
        */
        std::vector<std::string> validateDependencies() const;

    private:
        /**
        * @brief Utility function for cycle detection using DFS
        * @param node Current node being visited
        * @param visited Map of visited nodes (0=unvisited, 1=in-progress, 2=completed)
        * @param recStack Current recursion stack for path tracking
        * @param cycleNodes Output parameter for the cycle nodes if found
        * @return True if a cycle is detected
        */
        bool hasCycleUtil(
            const std::string& node,
            std::unordered_map<std::string, int>& visited,
            std::vector<std::string>& recStack,
            std::vector<std::string>& cycleNodes) const;

        std::unordered_map<std::string, TargetPtr> targets_;
    };
} // namespace mimir
