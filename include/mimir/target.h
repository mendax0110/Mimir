#pragma once

#include <string>
#include <vector>
#include <memory>

/// @brief Build target representation for mimir Build System \namespace mimir
namespace mimir
{
    /// @brief Represents a build target with inputs, outputs, and dependencies \class Target
    class Target
    {
    public:
        /**
        * @brief Default constructor
        */
        Target() = default;

        /**
        * @brief Construct a target with a name
        * @param name The unique identifier for this target
        */
        explicit Target(std::string name);

        /**
        * @brief Copy constructor
        * @param other The target to copy from
        */
        Target(const Target& other) = default;

        /**
        * @brief Move constructor
        * @param other The target to move from
        */
        Target(Target&& other) noexcept = default;

        /**
        * @brief Copy assignment operator
        * @param other The target to copy from
        * @return Reference to this target
        */
        Target& operator=(const Target& other) = default;

        /**
        * @brief Move assignment operator
        * @param other The target to move from
        * @return Reference to this target
        */
        Target& operator=(Target&& other) noexcept = default;

        /**
        * @brief Virtual destructor for proper cleanup in derived classes
        */
        virtual ~Target() = default;

        /**
        * @brief Get the target name
        * @return Const reference to the target name
        */
        const std::string& getName() const noexcept;

        /**
        * @brief Set the target name
        * @param name The new target name
        */
        void setName(std::string name);

        /**
        * @brief Get the input files
        * @return Const reference to the input file list
        */
        const std::vector<std::string>& getInputs() const noexcept;

        /**
        * @brief Add an input file
        * @param input Path to the input file
        */
        void addInput(std::string input);

        /**
        * @brief Set all input files
        * @param inputs Vector of input file paths
        */
        void setInputs(std::vector<std::string> inputs);

        /**
        * @brief Get the output files
        * @return Const reference to the output file list
        */
        const std::vector<std::string>& getOutputs() const noexcept;

        /**
        * @brief Add an output file
        * @param output Path to the output file
        */
        void addOutput(std::string output);

        /**
        * @brief Set all output files
        * @param outputs Vector of output file paths
        */
        void setOutputs(std::vector<std::string> outputs);

        /**
        * @brief Get the build command
        * @return Const reference to the command string
        */
        const std::string& getCommand() const noexcept;

        /**
        * @brief Set the build command
        * @param command The shell command to execute
        */
        void setCommand(std::string command);

        /**
        * @brief Get the target dependencies
        * @return Const reference to the dependency list
        */
        const std::vector<std::string>& getDependencies() const noexcept;

        /**
        * @brief Add a dependency
        * @param dependency Name of the target this depends on
        */
        void addDependency(std::string dependency);

        /**
        * @brief Set all dependencies
        * @param dependencies Vector of dependency target names
        */
        void setDependencies(std::vector<std::string> dependencies);

        /**
        * @brief Get the cached signature
        * @return Const reference to the signature string
        */
        const std::string& getSignature() const noexcept;

        /**
        * @brief Set the cached signature
        * @param signature The computed signature
        */
        void setSignature(std::string signature);

        /**
        * @brief Check if this target has any dependencies
        * @return True if the target has dependencies
        */
        bool hasDependencies() const noexcept;

        /**
        * @brief Check if this target has any inputs
        * @return True if the target has input files
        */
        bool hasInputs() const noexcept;

        /**
        * @brief Check if this target has any outputs
        * @return True if the target has output files
        */
        bool hasOutputs() const noexcept;

    private:
        std::string name_;
        std::vector<std::string> inputs_;
        std::vector<std::string> outputs_;
        std::string command_;
        std::vector<std::string> dependencies_;
        std::string signature_;
    };

    /**
    * @brief Shared pointer type for Target
    */
    using TargetPtr = std::shared_ptr<mimir::Target>;

    /**
    * @brief Const shared pointer type for Target
    */
    using TargetConstPtr = std::shared_ptr<const mimir::Target>;

    /**
    * @brief Weak pointer type for Target (for avoiding circular references)
    */
    using TargetWeakPtr = std::weak_ptr<mimir::Target>;

    /**
    * @brief Factory function to create a new Target
    * @param name The target name
    * @return Shared pointer to the new target
    */
    TargetPtr makeTarget(std::string name);

} // namespace mimir
