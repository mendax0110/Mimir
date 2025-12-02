#include "../include/mimir/command_runner.h"
#include <cstdlib>
#include <array>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace mimir;

CommandResult SystemCommandRunner::run(
    const std::string& command,
    const CommandOptions& options)
{
    CommandResult result{0, "", "", false};

    std::string fullCommand = command;

    if (!options.workingDir.empty())
    {
        fullCommand = "cd " + options.workingDir + " && " + command;
    }

    if (options.captureOutput)
    {
        std::string tmpOut = "/tmp/mimir_out_" + std::to_string(std::hash<std::string>{}(command));
        std::string tmpErr = "/tmp/mimir_err_" + std::to_string(std::hash<std::string>{}(command));

        fullCommand += " > " + tmpOut + " 2> " + tmpErr;
        
        result.exitCode = std::system(fullCommand.c_str());

        // Read captured output
        std::ifstream outFile(tmpOut);
        if (outFile.is_open())
        {
            std::ostringstream oss;
            oss << outFile.rdbuf();
            result.stdOut = oss.str();
            outFile.close();
        }

        std::ifstream errFile(tmpErr);
        if (errFile.is_open())
        {
            std::ostringstream oss;
            oss << errFile.rdbuf();
            result.stdErr = oss.str();
            errFile.close();
        }

        // Cleanup
        std::error_code ec;
        fs::remove(tmpOut, ec);
        fs::remove(tmpErr, ec);
    }
    else
    {
        result.exitCode = std::system(fullCommand.c_str());
    }

    //normalize exit code (system())
#ifdef _WIN32
    // Windows returns the exit code directly
#else
    // Unix: WEXITSTATUS macro extracts the exit code
    if (WIFEXITED(result.exitCode))
    {
        result.exitCode = WEXITSTATUS(result.exitCode);
    }
#endif

    return result;
}

bool SystemCommandRunner::runSimple(const std::string& command)
{
    int result = std::system(command.c_str());
    return result == 0;
}

MockCommandRunner::MockCommandRunner()
    : defaultResult_{0, "", "", false}
    , commandCount_(0)
{
}

CommandResult MockCommandRunner::run(
    const std::string& command,
    const CommandOptions& options)
{
    (void)options;
    
    lastCommand_ = command;
    ++commandCount_;

    if (handler_)
    {
        return handler_(command, options);
    }

    auto it = commandResults_.find(command);
    if (it != commandResults_.end())
    {
        return it->second;
    }

    return defaultResult_;
}

bool MockCommandRunner::runSimple(const std::string& command)
{
    return run(command).success();
}

void MockCommandRunner::setDefaultResult(const CommandResult& result)
{
    defaultResult_ = result;
}

void MockCommandRunner::setResultFor(const std::string& command, const CommandResult& result)
{
    commandResults_[command] = result;
}

void MockCommandRunner::setHandler(CommandHandler handler)
{
    handler_ = std::move(handler);
}

const std::string& MockCommandRunner::getLastCommand() const
{
    return lastCommand_;
}

size_t MockCommandRunner::getCommandCount() const
{
    return commandCount_;
}

void MockCommandRunner::reset()
{
    lastCommand_.clear();
    commandCount_ = 0;
    commandResults_.clear();
    handler_ = nullptr;
    defaultResult_ = {0, "", "", false};
}

CommandRunnerPtr mimir::createDefaultCommandRunner()
{
    return std::make_shared<SystemCommandRunner>();
}
