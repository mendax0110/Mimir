#include "mimir/parser.h"
#include "mimir/dag.h"
#include "mimir/executor.h"
#include "mimir/cache.h"
#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>

void printUsage(const char* prog)
{
    std::cout << "Mimir - Modern C++ Build System\n\n";
    std::cout << "Usage: " << prog << " [OPTIONS] [COMMAND]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  build       Build targets (default)\n";
    std::cout << "  clean       Clean cache\n\n";
    std::cout << "Options:\n";
    std::cout << "  -f FILE     Build file (default: build.yaml)\n";
    std::cout << "  -j N        Number of parallel jobs (default: 1)\n";
    std::cout << "  -n          Dry run (don't execute commands)\n";
    std::cout << "  -v          Verbose output\n";
    std::cout << "  --no-color  Disable colored output\n";
    std::cout << "  -h          Show this help\n";
}

void printBuildStats(const mimir::BuildStats& stats)
{
    std::cout << "\n";
    std::cout << "Build Statistics:\n";
    std::cout << "  Total targets:   " << stats.totalTargets << "\n";
    std::cout << "  Built:           " << stats.builtTargets << "\n";
    std::cout << "  Skipped:         " << stats.skippedTargets << "\n";
    std::cout << "  Failed:          " << stats.failedTargets << "\n";
    std::cout << "  Elapsed time:    " << std::fixed << std::setprecision(2) 
              << stats.elapsedSeconds << "s\n";
}

int main(const int argc, char* argv[])
{
    std::string buildFile = "build.yaml";
    mimir::ExecutorConfig config;
    std::string command = "build";
    
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printUsage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            buildFile = argv[++i];
        }
        else if (strcmp(argv[i], "-j") == 0 && i + 1 < argc)
        {
            config.numThreads = std::atoi(argv[++i]);
            if (config.numThreads < 1)
            {
                config.numThreads = 1;
            }
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--dry-run") == 0)
        {
            config.dryRun = true;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            config.verbose = true;
        }
        else if (strcmp(argv[i], "--no-color") == 0)
        {
            config.colorOutput = false;
        }
        else if (strcmp(argv[i], "build") == 0)
        {
            command = "build";
        }
        else if (strcmp(argv[i], "clean") == 0)
        {
            command = "clean";
        }
    }
    
    if (command == "clean")
    {
        std::cout << "Cleaning cache...\n";
        mimir::Cache cache;
        cache.clear();
        const int ret = system("rm -rf .mimir");
        (void)ret;
        std::cout << "Cache cleaned.\n";
        return 0;
    }
    
    std::vector<mimir::Target> targets;
    mimir::Parser parser;
    
    if (buildFile.find(".yaml") != std::string::npos || buildFile.find(".yml") != std::string::npos)
    {
        targets = parser.parseYAML(buildFile);
    }
    else if (buildFile.find(".toml") != std::string::npos)
    {
        targets = parser.parseTOML(buildFile);
    }
    else
    {
        std::cerr << "Unknown file format: " << buildFile << std::endl;
        return 1;
    }
    
    if (targets.empty())
    {
        if (auto error = parser.getLastError())
        {
            std::cerr << "Parse error in " << error->file << ":" << error->line 
                      << ": " << error->message << std::endl;
        }
        else
        {
            std::cerr << "No targets found in " << buildFile << std::endl;
        }
        return 1;
    }
    
    std::cout << "Loaded " << targets.size() << " targets from " << buildFile << std::endl;
    
    mimir::DAG dag;
    for (const auto& target : targets)
    {
        dag.addTarget(target);
    }
    
    auto missingDeps = dag.validateDependencies();
    if (!missingDeps.empty())
    {
        std::cerr << "Error: Missing dependencies:\n";
        for (const auto& dep : missingDeps)
        {
            std::cerr << "  - " << dep << "\n";
        }
        return 1;
    }
    
    auto cycleResult = dag.detectCyclesWithResult();
    if (cycleResult.hasCycle)
    {
        std::cerr << "Error: Cycle detected in dependency graph!\n";
        if (!cycleResult.cycleNodes.empty())
        {
            std::cerr << "  Involved targets: ";
            for (const auto& node : cycleResult.cycleNodes)
            {
                std::cerr << node << " ";
            }
            std::cerr << "\n";
        }
        return 1;
    }
    
    mimir::Cache cache;
    cache.load();

    mimir::Executor executor(config);
    
    if (config.dryRun)
    {
        std::cout << "[DRY RUN] ";
    }
    std::cout << "Building with " << config.numThreads << " parallel job(s)...\n";

    mimir::BuildStats stats;
    const bool success = executor.executeWithStats(dag, cache, stats);
    
    cache.save();
    
    printBuildStats(stats);
    
    if (!success)
    {
        std::cerr << "\nBuild failed!\n";
        return 1;
    }
    
    std::cout << "\nBuild completed successfully!\n";
    return 0;
}
