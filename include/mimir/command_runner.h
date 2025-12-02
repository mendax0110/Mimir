#pragma once

#include <string>
#include <memory>
#include <functional>
#include <optional>

/// @brief Command execution abstraction for testability and portability \namespace mimir
namespace mimir
{
    /// @brief Result of command execution \struct CommandResult
    struct CommandResult
    {
        int exitCode;           ///< Process exit code (0 = success)
        std::string stdOut;     ///< Standard output captured
        std::string stdErr;     ///< Standard error captured
        bool timedOut;          ///< True if command timed out

        /**
        * @brief Check if command succeeded
        * @return True if exit code is 0
        */
        bool success() const noexcept
        {
            return exitCode == 0 && !timedOut;
        }
    };

    /// @brief Options for command execution \struct CommandOptions
    struct CommandOptions
    {
        std::string workingDir;             ///< Working directory (empty = current)
        std::optional<int> timeoutSeconds;  ///< Timeout in seconds (nullopt = no timeout)
        bool captureOutput;                 ///< Whether to capture stdout/stderr
        bool inheritEnvironment;            ///< Whether to inherit parent environment

        /**
        * @brief Default options
        */
        CommandOptions()
            : workingDir("")
            , timeoutSeconds(std::nullopt)
            , captureOutput(false)
            , inheritEnvironment(true)
        {
        }
    };

    /// @brief Interface for command runners \class ICommandRunner
    class ICommandRunner
    {
    public:
        /**
        * @brief Virtual destructor
        */
        virtual ~ICommandRunner() = default;

        /**
        * @brief Execute a shell command
        * @param command The command string to execute
        * @param options Execution options
        * @return Result of command execution
        */
        virtual CommandResult run(
            const std::string& command,
            const CommandOptions& options = CommandOptions()) = 0;

        /**
        * @brief Execute a command with simple success/failure result
        * @param command The command string to execute
        * @return True if command succeeded (exit code 0)
        */
        virtual bool runSimple(const std::string& command) = 0;
    };

    /**
    * @brief Shared pointer type for command runner
    */
    using CommandRunnerPtr = std::shared_ptr<ICommandRunner>;

    /// @brief Default command runner using system calls \class SystemCommandRunner
    class SystemCommandRunner : public ICommandRunner
    {
    public:
        /**
        * @brief Default constructor
        */
        SystemCommandRunner() = default;

        /**
        * @brief Destructor
        */
        ~SystemCommandRunner() override = default;

        /**
        * @brief Execute a shell command
        * @param command The command string to execute
        * @param options Execution options (workingDir and captureOutput supported)
        * @return Result of command execution
        */
        CommandResult run(
            const std::string& command,
            const CommandOptions& options = CommandOptions()) override;

        /**
        * @brief Execute a command with simple success/failure result
        * @param command The command string to execute
        * @return True if command succeeded (exit code 0)
        */
        bool runSimple(const std::string& command) override;
    };

    /// @brief Mock command runner for unit testing \class MockCommandRunner
    class MockCommandRunner : public ICommandRunner
    {
    public:
        /**
        * @brief Callback type for command handling
        */
        using CommandHandler = std::function<CommandResult(const std::string&, const CommandOptions&)>;

        /**
        * @brief Default constructor with all commands succeeding
        */
        MockCommandRunner();

        /**
        * @brief Destructor
        */
        ~MockCommandRunner() override = default;

        /**
        * @brief Execute a shell command (returns mocked result)
        * @param command The command string
        * @param options Execution options
        * @return Mocked result
        */
        CommandResult run(
            const std::string& command,
            const CommandOptions& options = CommandOptions()) override;

        /**
        * @brief Execute with simple result
        * @param command The command string
        * @return Mocked success/failure
        */
        bool runSimple(const std::string& command) override;

        /**
        * @brief Set default result for all commands
        * @param result The result to return
        */
        void setDefaultResult(const CommandResult& result);

        /**
        * @brief Set result for a specific command
        * @param command Command pattern to match
        * @param result Result to return for this command
        */
        void setResultFor(const std::string& command, const CommandResult& result);

        /**
        * @brief Set a custom handler for all commands
        * @param handler Function to handle commands
        */
        void setHandler(CommandHandler handler);

        /**
        * @brief Get the last command that was executed
        * @return The last command string
        */
        const std::string& getLastCommand() const;

        /**
        * @brief Get count of commands executed
        * @return Number of run() calls
        */
        size_t getCommandCount() const;

        /**
        * @brief Reset the mock state
        */
        void reset();

    private:
        CommandResult defaultResult_;
        std::unordered_map<std::string, CommandResult> commandResults_;
        CommandHandler handler_;
        std::string lastCommand_;
        size_t commandCount_;
    };

    /**
    * @brief Create the default command runner
    * @return Shared pointer to SystemCommandRunner
    */
    CommandRunnerPtr createDefaultCommandRunner();
} // namespace mimir

