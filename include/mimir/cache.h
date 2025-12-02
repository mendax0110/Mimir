#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <optional>


/// @brief Thread-safe cache for storing and retrieving build signatures \namespace mimir
namespace mimir
{
    /// @brief Thread-safe cache for storing and retrieving build signatures \class Cache
    class Cache
    {
    public:
        /**
        * @brief Construct a cache with the specified directory
        * @param cacheDir Directory path for cache storage (default: ".mimir")
        */
        explicit Cache(const std::string& cacheDir = ".mimir");

        /**
        * @brief Destructor
        */
        ~Cache() = default;

        /**
        * @brief Cache is not copyable
        */
        Cache(const Cache&) = delete;

        /**
        * @brief Cache is not copy-assignable
        */
        Cache& operator=(const Cache&) = delete;

        /**
        * @brief Cache is movable
        * @param other Cache to move from
        */
        Cache(Cache&& other) noexcept;

        /**
        * @brief Cache is move-assignable
        * @param other Cache to move from
        * @return Reference to this cache
        */
        Cache& operator=(Cache&& other) noexcept;

        /**
        * @brief Load cache data from persistent storage
        * @return True if loading succeeded, false otherwise
        * @note Thread-safe: acquires exclusive lock
        */
        bool load();

        /**
        * @brief Save cache data to persistent storage
        * @return True if saving succeeded, false otherwise
        * @note Thread-safe: acquires shared lock
        */
        bool save() const;

        /**
        * @brief Get the stored signature for a target
        * @param targetName Name of the target to query
        * @return The stored signature, or empty string if not found
        * @note Thread-safe: acquires shared lock
        */
        std::string getSignature(const std::string& targetName) const;

        /**
        * @brief Get the stored signature as optional
        * @param targetName Name of the target to query
        * @return Optional containing signature if found, nullopt otherwise
        * @note Thread-safe: acquires shared lock
        */
        std::optional<std::string> findSignature(const std::string& targetName) const;

        /**
        * @brief Set the signature for a target
        * @param targetName Name of the target
        * @param signature The signature to store
        * @note Thread-safe: acquires exclusive lock
        */
        void setSignature(const std::string& targetName, const std::string& signature);

        /**
        * @brief Check if a target needs to be rebuilt
        * @param targetName Name of the target to check
        * @param currentSignature The current computed signature
        * @return True if rebuild is needed, false if up-to-date
        * @note Thread-safe: acquires shared lock
        */
        bool needsRebuild(const std::string& targetName, const std::string& currentSignature) const;

        /**
        * @brief Remove a target's signature from the cache
        * @param targetName Name of the target to remove
        * @return True if the target was found and removed
        * @note Thread-safe: acquires exclusive lock
        */
        bool removeSignature(const std::string& targetName);

        /**
        * @brief Clear all cached signatures
        * @note Thread-safe: acquires exclusive lock
        */
        void clear();

        /**
        * @brief Get the number of cached signatures
        * @return Number of entries in the cache
        * @note Thread-safe: acquires shared lock
        */
        size_t size() const;

        /**
        * @brief Check if the cache is empty
        * @return True if no signatures are cached
        * @note Thread-safe: acquires shared lock
        */
        bool empty() const;

        /**
        * @brief Get the cache directory path
        * @return Const reference to the cache directory
        */
        const std::string& getCacheDir() const noexcept;

        /**
        * @brief Get the cache file path
        * @return Const reference to the cache file path
        */
        const std::string& getCacheFile() const noexcept;

    private:
        /**
        * @brief Ensure the cache directory exists
        * @return True if directory exists or was created
        */
        bool ensureCacheDir() const;

        std::string cacheDir_;
        std::string cacheFile_;
        std::unordered_map<std::string, std::string> signatures_;
        mutable std::shared_mutex mutex_;
    };
} // namespace mimir
