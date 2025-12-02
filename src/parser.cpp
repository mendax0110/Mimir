#include "mimir/parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <unordered_map>
#include <filesystem>
#include <cmath>

namespace fs = std::filesystem;
using namespace mimir;

std::string ParseError::toString() const
{
    std::ostringstream oss;
    oss << "Parse error";
    if (!file.empty())
    {
        oss << " in " << file;
    }
    if (line > 0)
    {
        oss << " at line " << line;
        if (column > 0)
        {
            oss << ", column " << column;
        }
    }
    oss << ": " << message;
    return oss.str();
}

std::optional<ParseError> Parser::getLastError() const
{
    return lastError_;
}

void Parser::clearError()
{
    lastError_.reset();
}

ParseResult<std::vector<Target>> Parser::parseFile(const std::string& filepath)
{
    if (filepath.find(".yaml") != std::string::npos || 
        filepath.find(".yml") != std::string::npos)
    {
        return parseYAMLWithResult(filepath);
    }
    else if (filepath.find(".toml") != std::string::npos)
    {
        return parseTOMLWithResult(filepath);
    }
    
    return ParseError("Unknown file format", filepath);
}

ParseResult<std::vector<Target>> Parser::parseYAMLWithResult(const std::string& filepath)
{
    auto targets = parseYAML(filepath);
    if (lastError_)
    {
        return *lastError_;
    }
    return targets;
}

ParseResult<std::vector<Target>> Parser::parseTOMLWithResult(const std::string& filepath)
{
    auto targets = parseTOML(filepath);
    if (lastError_)
    {
        return *lastError_;
    }
    return targets;
}

std::vector<Target> Parser::parseYAML(const std::string& filepath)
{
    std::vector<Target> targets;
    lastError_.reset();
    
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        lastError_ = ParseError("Failed to open file", filepath);
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return targets;
    }

    Target current;
    std::string line;
    bool inTarget = false;
    std::string currentList;
    std::string currentSection;
    size_t multilineIndentLevel = 0;
    bool readingMultilineCommand = false;
    std::stringstream multilineBuffer;
    size_t lineNumber = 0;

    std::unordered_map<std::string, std::string> vars;
    std::unordered_map<std::string, std::string> cfg;

    auto expandAllLists = [&](const Target& t) -> std::unordered_map<std::string, std::string>
    {
        std::unordered_map<std::string, std::string> tmpVars = vars;

        std::vector<std::string> expandedInputs;
        for (const auto& i : t.getInputs())
        {
            auto v = expandGlob(i);
            expandedInputs.insert(expandedInputs.end(), v.begin(), v.end());
        }

        std::vector<std::string> expandedOutputs;
        for (const auto& o : t.getOutputs())
        {
            auto v = expandGlob(o);
            expandedOutputs.insert(expandedOutputs.end(), v.begin(), v.end());
        }

        std::vector<std::string> expandedDeps;
        for (const auto& d : t.getDependencies())
        {
            auto v = expandGlob(d);
            expandedDeps.insert(expandedDeps.end(), v.begin(), v.end());
        }

        tmpVars["inputs"] = joinList(expandedInputs);
        tmpVars["outputs"] = joinList(expandedOutputs);
        tmpVars["dependencies"] = joinList(expandedDeps);

        return tmpVars;
    };

    while (std::getline(file, line))
    {
        ++lineNumber;
        
        if (readingMultilineCommand)
        {
            size_t indent = line.find_first_not_of(" \t");
            if (indent != std::string::npos && indent >= multilineIndentLevel)
            {
                multilineBuffer << line.substr(multilineIndentLevel) << "\n";
                continue;
            }
            else
            {
                current.setCommand(expandVariables(multilineBuffer.str(), expandAllLists(current), cfg));
                readingMultilineCommand = false;
                multilineBuffer.str("");
            }
        }

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t indent = line.find_first_not_of(" \t");
        if (indent == std::string::npos)
        {
            continue;
        }
        std::string trimmed = line.substr(indent);

        if (indent == 0)
        {
            if (trimmed == "variables:")
            {
                currentSection = "variables";
                continue;
            }
            if (trimmed == "config:")
            {
                currentSection = "config";
                continue;
            }
            if (trimmed == "targets:")
            {
                currentSection.clear();
                continue;
            }
        }

        if (!currentSection.empty())
        {
            size_t colonPos = trimmed.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = trimmed.substr(0, colonPos);
                std::string value = trimmed.substr(colonPos + 1);
                size_t valueStart = value.find_first_not_of(" \t");
                if (valueStart != std::string::npos)
                {
                    value = value.substr(valueStart);
                }

                if (currentSection == "variables")
                {
                    vars[key] = value;
                }
                else if (currentSection == "config")
                {
                    cfg[key] = value;
                }
                continue;
            }
        }

        if (indent == 2 && trimmed[0] == '-')
        {
            if (inTarget && !current.getName().empty())
            {
                targets.push_back(current);
            }

            current = Target();
            currentList.clear();
            inTarget = true;

            if (trimmed.find("name:") != std::string::npos)
            {
                size_t pos = trimmed.find("name:");
                std::string nameValue = trimmed.substr(pos + 5);
                size_t start = nameValue.find_first_not_of(" \t");
                if (start != std::string::npos)
                {
                    nameValue = nameValue.substr(start);
                }
                current.setName(nameValue);
            }
            continue;
        }

        if (!inTarget)
        {
            continue;
        }

        size_t colonPos = trimmed.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = trimmed.substr(0, colonPos);
            std::string value = trimmed.substr(colonPos + 1);
            size_t start = value.find_first_not_of(" \t");
            if (start != std::string::npos)
            {
                value = value.substr(start);
            }

            if (key == "name")
            {
                current.setName(value);
                currentList.clear();
            }
            else if (key == "command")
            {
                if (value == "|" || value == ">")
                {
                    readingMultilineCommand = true;
                    multilineIndentLevel = indent + 2;
                    multilineBuffer.str("");
                    continue;
                }
                else
                {
                    current.setCommand(expandVariables(value, expandAllLists(current), cfg));
                }
                currentList.clear();
            }
            else if (key == "inputs")
            {
                currentList = "inputs";
            }
            else if (key == "outputs")
            {
                currentList = "outputs";
            }
            else if (key == "dependencies")
            {
                currentList = "dependencies";
            }
        }
        else if (trimmed[0] == '-' && !currentList.empty())
        {
            std::string item = trimmed.substr(1);
            size_t start = item.find_first_not_of(" \t");
            if (start != std::string::npos)
            {
                item = item.substr(start);
            }

            if (currentList == "inputs")
            {
                current.addInput(item);
            }
            else if (currentList == "outputs")
            {
                current.addOutput(item);
            }
            else if (currentList == "dependencies")
            {
                current.addDependency(item);
            }
        }
    }

    if (readingMultilineCommand)
    {
        current.setCommand(expandVariables(multilineBuffer.str(), expandAllLists(current), cfg));
    }

    if (inTarget && !current.getName().empty())
    {
        targets.push_back(current);
    }
    
    return targets;
}

std::vector<Target> Parser::parseTOML(const std::string& filepath)
{
    std::vector<Target> targets;
    lastError_.reset();
    
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        lastError_ = ParseError("Failed to open file", filepath);
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return targets;
    }

    Target current;
    std::string line;
    std::string currentList;
    std::unordered_map<std::string, std::string> vars;
    std::unordered_map<std::string, std::string> cfg;
    size_t lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;
        
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t trimStart = line.find_first_not_of(" \t");
        if (trimStart == std::string::npos)
        {
            continue;
        }
        line = line.substr(trimStart);

        // Sections
        if (line[0] == '[' && line.back() == ']')
        {
            if (!current.getName().empty())
            {
                targets.push_back(current);
            }
            current = Target();
            currentList.clear();
            std::string section = line.substr(1, line.length() - 2);
            if (section.find("target") == 0)
            {
                size_t dot = section.find('.');
                if (dot != std::string::npos)
                {
                    current.setName(section.substr(dot + 1));
                }
            }
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        size_t keyEnd = key.find_last_not_of(" \t");
        if (keyEnd != std::string::npos)
        {
            key = key.substr(0, keyEnd + 1);
        }

        size_t valStart = value.find_first_not_of(" \t");
        if (valStart != std::string::npos)
        {
            value = value.substr(valStart);
        }

        if (!value.empty() && value.front() == '"' && value.back() == '"')
        {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "name")
        {
            current.setName(value);
        }
        else if (key == "command")
        {
            std::unordered_map<std::string, std::string> tmpVars = vars;
            tmpVars["inputs"] = joinList(current.getInputs());
            tmpVars["outputs"] = joinList(current.getOutputs());
            tmpVars["dependencies"] = joinList(current.getDependencies());
            current.setCommand(expandVariables(value, tmpVars, cfg));
            currentList.clear();
        }
        else if (key == "inputs" || key == "outputs" || key == "dependencies")
        {
            currentList = key;
            if (!value.empty() && value.front() == '[')
            {
                size_t endPos = value.find(']');
                std::string items = value.substr(1, endPos - 1);
                std::istringstream iss(items);
                std::string item;
                while (std::getline(iss, item, ','))
                {
                    size_t start = item.find_first_not_of(" \t\"");
                    size_t end = item.find_last_not_of(" \t\"");
                    if (start != std::string::npos && end != std::string::npos)
                    {
                        item = item.substr(start, end - start + 1);
                        if (currentList == "inputs")
                        {
                            current.addInput(item);
                        }
                        else if (currentList == "outputs")
                        {
                            current.addOutput(item);
                        }
                        else if (currentList == "dependencies")
                        {
                            current.addDependency(item);
                        }
                    }
                }
            }
        }
    }

    if (!current.getName().empty())
    {
        targets.push_back(current);
    }
    
    return targets;
}

void Parser::replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

std::string Parser::evaluateExpression(
    const std::string& expr,
    const std::unordered_map<std::string, std::string>& vars,
    const std::unordered_map<std::string, std::string>& cfg)
{
    std::regex ternary_expr(R"lit(^\s*(\w+)\s+if\s+config\.(\w+)\s*==\s*"([^"]+)"\s*else\s*(\w+)\s*$)lit");
    std::smatch m;
    if (std::regex_match(expr, m, ternary_expr))
    {
        std::string varTrue = m[1];
        std::string cfgKey = m[2];
        std::string cfgVal = m[3];
        std::string varFalse = m[4];

        auto itCfg = cfg.find(cfgKey);
        if (itCfg != cfg.end() && itCfg->second == cfgVal)
        {
            auto itVar = vars.find(varTrue);
            return itVar != vars.end() ? itVar->second : "";
        }
        else
        {
            auto itVar = vars.find(varFalse);
            return itVar != vars.end() ? itVar->second : "";
        }
    }

    auto itVar = vars.find(expr);
    if (itVar != vars.end())
    {
        return itVar->second;
    }
    return expr;
}

std::string Parser::expandVariables(
    const std::string& input,
    const std::unordered_map<std::string, std::string>& vars,
    const std::unordered_map<std::string, std::string>& cfg)
{
    std::string out = input;

    std::regex varPattern(R"(\$\{(\w+)\})");
    std::smatch m;
    std::string tmp;
    while (std::regex_search(out, m, varPattern))
    {
        tmp = evaluateExpression(m[1].str(), vars, cfg);
        out.replace(m.position(0), m.length(0), tmp);
    }

    std::regex exprPattern(R"(\$\{\{([^\}]+)\}\})");
    while (std::regex_search(out, m, exprPattern))
    {
        tmp = evaluateExpression(m[1].str(), vars, cfg);
        out.replace(m.position(0), m.length(0), tmp);
    }

    return out;
}

std::vector<std::string> Parser::expandGlob(const std::string& pattern)
{
    std::vector<std::string> files;

    if (pattern.find("**") != std::string::npos)
    {
        std::string base = pattern.substr(0, pattern.find("**"));
        std::error_code ec;
        if (fs::exists(base, ec))
        {
            for (const auto& p : fs::recursive_directory_iterator(base, ec))
            {
                if (fs::is_regular_file(p, ec))
                {
                    files.push_back(p.path().string());
                }
            }
        }
    }
    else
    {
        std::error_code ec;
        if (fs::exists(pattern, ec))
        {
            files.push_back(pattern);
        }
    }

    return files;
}

std::string Parser::joinList(const std::vector<std::string>& list)
{
    std::ostringstream oss;
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (i > 0)
        {
            oss << " ";
        }
        oss << list[i];
    }
    return oss.str();
}