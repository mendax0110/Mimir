#include "mimir/cache.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace mimir;

Cache::Cache(const std::string& cacheDir) 
    : cacheDir_(cacheDir)
    , cacheFile_(cacheDir + "/cache.txt")
{
    ensureCacheDir();
}

Cache::Cache(Cache&& other) noexcept
    : cacheDir_(std::move(other.cacheDir_))
    , cacheFile_(std::move(other.cacheFile_))
    , signatures_(std::move(other.signatures_))
{
}

Cache& Cache::operator=(Cache&& other) noexcept
{
    if (this != &other)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cacheDir_ = std::move(other.cacheDir_);
        cacheFile_ = std::move(other.cacheFile_);
        signatures_ = std::move(other.signatures_);
    }
    return *this;
}

bool Cache::ensureCacheDir() const
{
    std::error_code ec;
    if (!fs::exists(cacheDir_, ec))
    {
        return fs::create_directories(cacheDir_, ec);
    }
    return true;
}

bool Cache::load()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    std::ifstream file(cacheFile_);
    if (!file.is_open())
    {
        return false;
    }
    
    signatures_.clear();
    std::string line;
    while (std::getline(file, line))
    {
        size_t pos = line.find('=');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            signatures_[key] = value;
        }
    }
    return true;
}

bool Cache::save() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    ensureCacheDir();
    
    std::ofstream file(cacheFile_);
    if (!file.is_open())
    {
        return false;
    }
    
    for (const auto& [targetName, signature] : signatures_)
    {
        file << targetName << "=" << signature << "\n";
    }
    return true;
}

std::string Cache::getSignature(const std::string& targetName) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = signatures_.find(targetName);
    return it != signatures_.end() ? it->second : "";
}

std::optional<std::string> Cache::findSignature(const std::string& targetName) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = signatures_.find(targetName);
    if (it != signatures_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void Cache::setSignature(const std::string& targetName, const std::string& signature)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    signatures_[targetName] = signature;
}

bool Cache::needsRebuild(const std::string& targetName, const std::string& currentSignature) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = signatures_.find(targetName);
    if (it == signatures_.end())
    {
        return true;
    }
    return it->second != currentSignature;
}

bool Cache::removeSignature(const std::string& targetName)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return signatures_.erase(targetName) > 0;
}

void Cache::clear()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    signatures_.clear();
}

size_t Cache::size() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return signatures_.size();
}

bool Cache::empty() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return signatures_.empty();
}

const std::string& Cache::getCacheDir() const noexcept
{
    return cacheDir_;
}

const std::string& Cache::getCacheFile() const noexcept
{
    return cacheFile_;
}