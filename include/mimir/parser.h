#pragma once

#include "target.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <optional>

/// @brief Parser for build configuration files \namespace mimir
namespace mimir
{
    /// @brief Error information for parsing failures \struct ParseError
    struct ParseError
    {
        std::string message;      ///< Human-readable error message
        std::string file;         ///< File path where error occurred
        size_t line;              ///< Line number (1-based, 0 if unknown)
        size_t column;            ///< Column number (1-based, 0 if unknown)

        /**
        * @brief Construct a parse error
        * @param msg Error message
        * @param filePath Path to the file
        * @param lineNum Line number
        * @param colNum Column number
        */
        ParseError(
            std::string msg,
            std::string filePath = "",
            size_t lineNum = 0,
            size_t colNum = 0)
            : message(std::move(msg))
            , file(std::move(filePath))
            , line(lineNum)
            , column(colNum)
        {
        }

        /**
        * @brief Format error as string
        * @return Formatted error string
        */
        std::string toString() const;
    };

    /**
    * @brief Result type for parse operations
    */
    template<typename T>
    using ParseResult = std::variant<T, ParseError>;

    /// @brief Parser for build configuration files (YAML and TOML) \class Parser
    class Parser
    {
    public:
        /**
        * @brief Default constructor
        */
        Parser() = default;

        /**
        * @brief Destructor
        */
        ~Parser() = default;

        /**
        * @brief Parse a YAML build configuration file
        * @param filepath Path to the YAML file
        * @return Vector of parsed Targets (empty on failure)
        * @deprecated Use parseYAMLWithResult for proper error handling
        */
        std::vector<Target> parseYAML(const std::string& filepath);

        /**
        * @brief Parse a YAML file with error reporting
        * @param filepath Path to the YAML file
        * @return ParseResult containing targets or error
        */
        ParseResult<std::vector<Target>> parseYAMLWithResult(const std::string& filepath);

        /**
        * @brief Parse a TOML build configuration file
        * @param filepath Path to the TOML file
        * @return Vector of parsed Targets (empty on failure)
        * @deprecated Use parseTOMLWithResult for proper error handling
        */
        std::vector<Target> parseTOML(const std::string& filepath);

        /**
        * @brief Parse a TOML file with error reporting
        * @param filepath Path to the TOML file
        * @return ParseResult containing targets or error
        */
        ParseResult<std::vector<Target>> parseTOMLWithResult(const std::string& filepath);

        /**
        * @brief Parse a file based on its extension
        * @param filepath Path to the build file
        * @return ParseResult containing targets or error
        */
        ParseResult<std::vector<Target>> parseFile(const std::string& filepath);

        /**
        * @brief Get the last parse error (if any)
        * @return Optional containing the last error
        */
        std::optional<ParseError> getLastError() const;

        /**
        * @brief Clear any stored error state
        */
        void clearError();

    private:
        /**
        * @brief Replace all occurrences of a substring
        * @param str String to modify
        * @param from Substring to find
        * @param to Replacement string
        */
        static void replaceAll(std::string& str, const std::string& from, const std::string& to);

        /**
        * @brief Evaluate a conditional expression
        * @param expr Expression to evaluate
        * @param vars Variable map
        * @param cfg Configuration map
        * @return Evaluated result
        */
        std::string evaluateExpression(
            const std::string& expr,
            const std::unordered_map<std::string, std::string>& vars,
            const std::unordered_map<std::string, std::string>& cfg);

        /**
        * @brief Expand variables in a string
        * @param input Input string with variable references
        * @param vars Variable map
        * @param cfg Configuration map
        * @return String with variables expanded
        */
        std::string expandVariables(
            const std::string& input,
            const std::unordered_map<std::string, std::string>& vars,
            const std::unordered_map<std::string, std::string>& cfg);

        /**
        * @brief Expand glob patterns to file list
        * @param pattern Glob pattern
        * @return Vector of matching file paths
        */
        std::vector<std::string> expandGlob(const std::string& pattern);

        /**
        * @brief Join a list of strings with spaces
        * @param list Vector of strings
        * @return Space-separated string
        */
        std::string joinList(const std::vector<std::string>& list);

        std::optional<ParseError> lastError_;
    };
} // namespace mimir
