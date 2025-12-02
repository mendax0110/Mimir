#pragma once

#include <string>
#include <vector>

/// @brief mimir Build System Signature \namespace mimir
namespace mimir
{
    /// @brief Signature class for computing file and command signatures \class Signature
    class Signature
    {
    public:
        /**
         * @brief Compute the SHA-256 signature of a file
         * @param filepath The path to the file
         * @return The SHA-256 signature as a hex string
         */
        static std::string computeFileSignature(const std::string& filepath);

        /**
         * @brief Compute the signature of a command string
         * @param command The command string
         * @return The computed signature as a hex string
         */
        static std::string computeCommandSignature(const std::string& command);

        /**
         * @brief Compute the target signature based on command and input file signatures
         * @param command The command string
         * @param inputs The list of input file paths
         * @return The computed target signature as a hex string
         */
        static std::string computeTargetSignature(const std::string& command, const std::vector<std::string>& inputs);

    private:
        /**
         * @brief Compute the SHA-256 hash of a data string
         * @param data The input data string
         * @return The SHA-256 hash as a hex string
         */
        static std::string sha256(const std::string& data);
    };
}
