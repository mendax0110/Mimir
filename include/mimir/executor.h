#pragma once

#include "dag.h"
#include "cache.h"
#include "command_runner.h"
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

/// @brief Executor for building targets in a DAG \namespace mimir
namespace mimir
{
    /**
    * @brief Build progress callback type
    * @param targetName Name of the target being processed
    * @param current Current target number (1-based)
    * @param total Total number of targets
    * @param status Status message (BUILDING, UP-TO-DATE, FAILED, SUCCESS)
    */
    using ProgressCallback = std::function<void(
        const std::string& targetName,
        size_t current,
        size_t total,
        const std::string& status)>;

    /// @brief Statistics about a build execution \struct BuildStats
    struct BuildStats
    {
        size_t totalTargets;    ///< Total number of targets
        size_t builtTargets;    ///< Number of targets that were built
        size_t skippedTargets;  ///< Number of targets skipped (up-to-date)
        size_t failedTargets;   ///< Number of targets that failed
        double elapsedSeconds;  ///< Total build time in seconds

        /**
        * @brief Default constructor
        */
        BuildStats()
            : totalTargets(0)
            , builtTargets(0)
            , skippedTargets(0)
            , failedTargets(0)
            , elapsedSeconds(0.0)
        {
        }
    };

    /// @brief Configuration options for the executor \struct ExecutorConfig
    struct ExecutorConfig
    {
        int numThreads;             ///< Number of parallel worker threads
        bool dryRun;                ///< If true, don't actually run commands
        bool verbose;               ///< If true, print detailed output
        bool stopOnError;           ///< If true, stop on first error
        bool colorOutput;           ///< If true, use ANSI color codes

        /**
        * @brief Default configuration
        */
        ExecutorConfig()
            : numThreads(1)
            , dryRun(false)
            , verbose(false)
            , stopOnError(true)
            , colorOutput(true)
        {
        }
    };

    /// @brief Executor for building targets in a DAG \class Executor
    class Executor
    {
    public:
        /**
        * @brief Construct executor with specified thread count
        * @param numThreads Number of parallel threads (default: 1)
        */
        explicit Executor(int numThreads = 1);

        /**
        * @brief Construct executor with configuration
        * @param config Executor configuration options
        */
        explicit Executor(const ExecutorConfig& config);

        /**
        * @brief Construct executor with custom command runner
        * @param numThreads Number of parallel threads
        * @param runner Custom command runner implementation
        */
        Executor(int numThreads, CommandRunnerPtr runner);

        /**
        * @brief Destructor
        */
        ~Executor() = default;

        /**
        * @brief Execute the build for the given DAG
        * @param dag The DAG of targets to build
        * @param cache The build cache for incremental builds
        * @return True if the build was successful, false otherwise
        */
        bool execute(const DAG& dag, Cache& cache) const;

        /**
        * @brief Execute the build and return statistics
        * @param dag The DAG of targets to build
        * @param cache The build cache for incremental builds
        * @param stats Output parameter for build statistics
        * @return True if the build was successful
        */
        bool executeWithStats(const DAG& dag, Cache& cache, BuildStats& stats) const;

        /**
        * @brief Execute a single target
        * @param target The target to execute
        * @param cache The build cache to use
        * @return True if the target was successfully built
        */
        bool executeTarget(const Target& target, Cache& cache) const;

        /**
        * @brief Set the progress callback
        * @param callback Function to call on progress updates
        */
        void setProgressCallback(ProgressCallback callback);

        /**
        * @brief Get the executor configuration
        * @return Const reference to the configuration
        */
        const ExecutorConfig& getConfig() const noexcept;

        /**
        * @brief Set the executor configuration
        * @param config New configuration
        */
        void setConfig(const ExecutorConfig& config);

        /**
        * @brief Cancel any running build
        * @note Thread-safe
        */
        void cancel();

        /**
        * @brief Check if build was cancelled
        * @return True if cancel() was called
        */
        bool isCancelled() const;

        /**
        * @brief Reset the cancelled state
        */
        void resetCancelled();

    private:
        /**
        * @brief Check if a target is out of date
        * @param target The target to check
        * @param cache The build cache
        * @return True if the target needs rebuilding
        */
        bool isOutOfDate(const Target& target, const Cache& cache) const;

        /**
        * @brief Run a shell command
        * @param command The command to run
        * @return True if the command succeeded
        */
        bool runCommand(const std::string& command) const;

        /**
        * @brief Execute targets in single-threaded mode
        * @param dag The DAG of targets
        * @param cache The build cache
        * @param stats Build statistics
        * @return True if all targets succeeded
        */
        bool executeSingleThreaded(
            const DAG& dag,
            Cache& cache,
            BuildStats& stats) const;

        /**
        * @brief Execute targets in multi-threaded mode
        * @param dag The DAG of targets
        * @param cache The build cache
        * @param stats Build statistics
        * @return True if all targets succeeded
        */
        bool executeMultiThreaded(
            const DAG& dag,
            Cache& cache,
            BuildStats& stats) const;

        /**
        * @brief Print status message with optional color
        * @param status Status tag (BUILD, SUCCESS, FAILED, UP-TO-DATE)
        * @param targetName Name of the target
        * @param message Optional additional message
        */
        void printStatus(
            const std::string& status,
            const std::string& targetName,
            const std::string& message = "") const;

        ExecutorConfig config_;
        CommandRunnerPtr commandRunner_;
        ProgressCallback progressCallback_;
        mutable std::atomic<bool> cancelled_;
        mutable std::mutex outputMutex_;

        // ANSI color codes
        static constexpr const char* COLOR_GREEN = "\033[32m";
        static constexpr const char* COLOR_RED = "\033[31m";
        static constexpr const char* COLOR_YELLOW = "\033[33m";
        static constexpr const char* COLOR_RESET = "\033[0m";
    };
} // namespace mimir
