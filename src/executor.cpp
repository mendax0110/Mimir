#include "mimir/executor.h"
#include "mimir/signature.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <cstdlib>

using namespace mimir;

Executor::Executor(const int numThreads)
    : config_()
    , commandRunner_(createDefaultCommandRunner())
    , cancelled_(false)
{
    config_.numThreads = numThreads;
}

Executor::Executor(const ExecutorConfig& config)
    : config_(config)
    , commandRunner_(createDefaultCommandRunner())
    , cancelled_(false)
{
}

Executor::Executor(int numThreads, CommandRunnerPtr runner)
    : config_()
    , commandRunner_(std::move(runner))
    , cancelled_(false)
{
    config_.numThreads = numThreads;
    if (!commandRunner_)
    {
        commandRunner_ = createDefaultCommandRunner();
    }
}

bool Executor::isOutOfDate(const Target& target, const Cache& cache) const
{
    const std::string currentSig = Signature::computeTargetSignature(target.getCommand(), target.getInputs());
    return cache.needsRebuild(target.getName(), currentSig);
}

bool Executor::runCommand(const std::string& command) const
{
    if (config_.dryRun)
    {
        return true;
    }
    return commandRunner_->runSimple(command);
}

void Executor::printStatus(
    const std::string& status,
    const std::string& targetName,
    const std::string& message) const
{
    std::lock_guard<std::mutex> lock(outputMutex_);

    const char* color = COLOR_RESET;
    if (config_.colorOutput)
    {
        if (status == "SUCCESS" || status == "UP-TO-DATE")
        {
            color = COLOR_GREEN;
        }
        else if (status == "FAILED")
        {
            color = COLOR_RED;
        }
        else if (status == "BUILD")
        {
            color = COLOR_YELLOW;
        }
    }

    if (config_.colorOutput)
    {
        std::cout << color;
    }
    
    std::cout << "[ " << status << " ] " << targetName;
    
    if (!message.empty() && config_.verbose)
    {
        std::cout << "\n  " << message;
    }
    
    std::cout << std::endl;
    
    if (config_.colorOutput)
    {
        std::cout << COLOR_RESET;
    }
}

bool Executor::executeTarget(const Target& target, Cache& cache) const
{
    if (cancelled_.load())
    {
        return false;
    }

    // Check if outputs exist
    bool outputsExist = true;
    for (const auto& output : target.getOutputs())
    {
        std::ifstream file(output);
        if (!file.good())
        {
            outputsExist = false;
            break;
        }
    }

    if (outputsExist && !isOutOfDate(target, cache))
    {
        printStatus("UP-TO-DATE", target.getName());
        return true;
    }

    printStatus("BUILD", target.getName(), target.getCommand());

    if (!runCommand(target.getCommand()))
    {
        printStatus("FAILED", target.getName());
        return false;
    }

    const std::string newSig = Signature::computeTargetSignature(target.getCommand(), target.getInputs());
    cache.setSignature(target.getName(), newSig);

    printStatus("SUCCESS", target.getName());
    return true;
}

bool Executor::executeSingleThreaded(
    const DAG& dag,
    Cache& cache,
    BuildStats& stats) const
{
    const std::vector<std::string> order = dag.topologicalSort();
    stats.totalTargets = order.size();

    size_t current = 0;
    for (const auto& targetName : order)
    {
        if (cancelled_.load())
        {
            return false;
        }

        ++current;
        const Target* target = dag.getTarget(targetName);
        
        if (!target)
        {
            continue;
        }

        if (progressCallback_)
        {
            progressCallback_(targetName, current, stats.totalTargets, "BUILDING");
        }

        bool outputsExist = true;
        for (const auto& output : target->getOutputs())
        {
            std::ifstream file(output);
            if (!file.good())
            {
                outputsExist = false;
                break;
            }
        }

        if (outputsExist && !isOutOfDate(*target, cache))
        {
            printStatus("UP-TO-DATE", target->getName());
            ++stats.skippedTargets;
            if (progressCallback_)
            {
                progressCallback_(targetName, current, stats.totalTargets, "UP-TO-DATE");
            }
            continue;
        }

        printStatus("BUILD", target->getName(), target->getCommand());

        if (!runCommand(target->getCommand()))
        {
            printStatus("FAILED", target->getName());
            ++stats.failedTargets;
            if (progressCallback_)
            {
                progressCallback_(targetName, current, stats.totalTargets, "FAILED");
            }
            if (config_.stopOnError)
            {
                return false;
            }
            continue;
        }

        const std::string newSig = Signature::computeTargetSignature(target->getCommand(), target->getInputs());
        cache.setSignature(target->getName(), newSig);

        printStatus("SUCCESS", target->getName());
        ++stats.builtTargets;
        if (progressCallback_)
        {
            progressCallback_(targetName, current, stats.totalTargets, "SUCCESS");
        }
    }

    return stats.failedTargets == 0;
}

bool Executor::executeMultiThreaded(
    const DAG& dag,
    Cache& cache,
    BuildStats& stats) const
{
    const std::vector<std::string> order = dag.topologicalSort();
    stats.totalTargets = order.size();

    std::mutex mtx;
    std::condition_variable cv;
    std::unordered_map<std::string, bool> completed;
    std::unordered_map<std::string, bool> inProgress;
    std::atomic<bool> failed{false};
    std::atomic<size_t> processedCount{0};

    for (const auto& name : order)
    {
        completed[name] = false;
        inProgress[name] = false;
    }

    auto canExecute = [&](const Target& target) -> bool
    {
        for (const auto& dep : target.getDependencies())
        {
            if (!completed[dep])
            {
                return false;
            }
        }
        return !inProgress[target.getName()];
    };

    auto worker = [&]()
    {
        while (true)
        {
            if (cancelled_.load())
            {
                return;
            }

            std::string targetName;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&]()
                {
                    if (failed.load() && config_.stopOnError)
                    {
                        return true;
                    }
                    if (cancelled_.load())
                    {
                        return true;
                    }

                    bool allDone = true;
                    for (const auto& [name, done] : completed)
                    {
                        if (!done)
                        {
                            allDone = false;
                            break;
                        }
                    }
                    if (allDone)
                    {
                        return true;
                    }

                    for (const auto& name : order)
                    {
                        const Target* t = dag.getTarget(name);
                        if (t && !completed[name] && canExecute(*t))
                        {
                            return true;
                        }
                    }
                    return false;
                });

                if (cancelled_.load() || (failed.load() && config_.stopOnError))
                {
                    return;
                }

                bool allDone = true;
                for (const auto& [name, done] : completed)
                {
                    if (!done)
                    {
                        allDone = false;
                        break;
                    }
                }
                if (allDone)
                {
                    return;
                }

                for (const auto& name : order)
                {
                    const Target* t = dag.getTarget(name);
                    if (t && !completed[name] && canExecute(*t))
                    {
                        targetName = name;
                        inProgress[name] = true;
                        break;
                    }
                }
            }

            if (!targetName.empty())
            {
                const Target* target = dag.getTarget(targetName);
                if (target)
                {
                    size_t current = ++processedCount;
                    if (progressCallback_)
                    {
                        progressCallback_(targetName, current, stats.totalTargets, "BUILDING");
                    }

                    bool success = false;
                    
                    // Check if up-to-date
                    bool outputsExist = true;
                    for (const auto& output : target->getOutputs())
                    {
                        std::ifstream file(output);
                        if (!file.good())
                        {
                            outputsExist = false;
                            break;
                        }
                    }

                    if (outputsExist && !isOutOfDate(*target, cache))
                    {
                        printStatus("UP-TO-DATE", target->getName());
                        success = true;
                        {
                            std::lock_guard<std::mutex> statsLock(mtx);
                            ++stats.skippedTargets;
                        }
                    }
                    else
                    {
                        printStatus("BUILD", target->getName(), target->getCommand());
                        
                        if (runCommand(target->getCommand()))
                        {
                            const std::string newSig = Signature::computeTargetSignature(
                                target->getCommand(), target->getInputs());
                            cache.setSignature(target->getName(), newSig);
                            printStatus("SUCCESS", target->getName());
                            success = true;
                            {
                                std::lock_guard<std::mutex> statsLock(mtx);
                                ++stats.builtTargets;
                            }
                        }
                        else
                        {
                            printStatus("FAILED", target->getName());
                            {
                                std::lock_guard<std::mutex> statsLock(mtx);
                                ++stats.failedTargets;
                            }
                        }
                    }

                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        inProgress[targetName] = false;
                        completed[targetName] = true;
                        if (!success)
                        {
                            failed.store(true);
                        }
                    }
                    cv.notify_all();
                }
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(config_.numThreads);
    for (int i = 0; i < config_.numThreads; ++i)
    {
        threads.emplace_back(worker);
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    return !failed.load();
}

bool Executor::execute(const DAG& dag, Cache& cache) const
{
    BuildStats stats;
    return executeWithStats(dag, cache, stats);
}

bool Executor::executeWithStats(const DAG& dag, Cache& cache, BuildStats& stats) const
{
    auto startTime = std::chrono::steady_clock::now();

    bool success;
    if (config_.numThreads <= 1)
    {
        success = executeSingleThreaded(dag, cache, stats);
    }
    else
    {
        success = executeMultiThreaded(dag, cache, stats);
    }

    auto endTime = std::chrono::steady_clock::now();
    stats.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    return success;
}

void Executor::setProgressCallback(ProgressCallback callback)
{
    progressCallback_ = std::move(callback);
}

const ExecutorConfig& Executor::getConfig() const noexcept
{
    return config_;
}

void Executor::setConfig(const ExecutorConfig& config)
{
    config_ = config;
}

void Executor::cancel()
{
    cancelled_.store(true);
}

bool Executor::isCancelled() const
{
    return cancelled_.load();
}

void Executor::resetCancelled()
{
    cancelled_.store(false);
}