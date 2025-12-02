#include "mimir/target.h"
#include <utility>

using namespace mimir;

Target::Target(std::string targetName)
    : name_(std::move(targetName))
{
}

const std::string& Target::getName() const noexcept
{
    return name_;
}

void Target::setName(std::string targetName)
{
    name_ = std::move(targetName);
}

const std::vector<std::string>& Target::getInputs() const noexcept
{
    return inputs_;
}

void Target::addInput(std::string input)
{
    inputs_.push_back(std::move(input));
}

void Target::setInputs(std::vector<std::string> inputList)
{
    inputs_ = std::move(inputList);
}

const std::vector<std::string>& Target::getOutputs() const noexcept
{
    return outputs_;
}

void Target::addOutput(std::string output)
{
    outputs_.push_back(std::move(output));
}

void Target::setOutputs(std::vector<std::string> outputList)
{
    outputs_ = std::move(outputList);
}

const std::string& Target::getCommand() const noexcept
{
    return command_;
}

void Target::setCommand(std::string cmd)
{
    command_ = std::move(cmd);
}

const std::vector<std::string>& Target::getDependencies() const noexcept
{
    return dependencies_;
}

void Target::addDependency(std::string dependency)
{
    dependencies_.push_back(std::move(dependency));
}

void Target::setDependencies(std::vector<std::string> deps)
{
    dependencies_ = std::move(deps);
}

const std::string& Target::getSignature() const noexcept
{
    return signature_;
}

void Target::setSignature(std::string sig)
{
    signature_ = std::move(sig);
}

bool Target::hasDependencies() const noexcept
{
    return !dependencies_.empty();
}

bool Target::hasInputs() const noexcept
{
    return !inputs_.empty();
}

bool Target::hasOutputs() const noexcept
{
    return !outputs_.empty();
}

TargetPtr mimir::makeTarget(std::string name)
{
    return std::make_shared<Target>(std::move(name));
}