#include "mimir/cache.h"
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;

class CacheTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testDir_ = "/tmp/test_mimir_cache_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed());
        fs::remove_all(testDir_);
    }

    void TearDown() override
    {
        fs::remove_all(testDir_);
    }

    std::string testDir_;
};

TEST_F(CacheTest, SetAndGetSignature)
{
    mimir::Cache cache(testDir_);
    
    cache.setSignature("target1", "abc123");
    cache.setSignature("target2", "def456");
    
    EXPECT_EQ(cache.getSignature("target1"), "abc123");
    EXPECT_EQ(cache.getSignature("target2"), "def456");
}

TEST_F(CacheTest, GetSignatureNonExistent)
{
    mimir::Cache cache(testDir_);
    
    EXPECT_EQ(cache.getSignature("nonexistent"), "");
}

TEST_F(CacheTest, SaveAndLoad)
{
    {
        mimir::Cache cache(testDir_);
        cache.setSignature("target1", "abc123");
        cache.setSignature("target2", "def456");
        EXPECT_TRUE(cache.save());
    }
    
    mimir::Cache cache2(testDir_);
    EXPECT_TRUE(cache2.load());
    
    EXPECT_EQ(cache2.getSignature("target1"), "abc123");
    EXPECT_EQ(cache2.getSignature("target2"), "def456");
}

TEST_F(CacheTest, LoadNonExistentCache)
{
    mimir::Cache cache(testDir_);

    cache.load();
    EXPECT_EQ(cache.getSignature("anything"), "");
}

TEST_F(CacheTest, NeedsRebuildSameSignature)
{
    mimir::Cache cache(testDir_);
    cache.setSignature("target1", "abc123");
    
    EXPECT_FALSE(cache.needsRebuild("target1", "abc123"));
}

TEST_F(CacheTest, NeedsRebuildDifferentSignature)
{
    mimir::Cache cache(testDir_);
    cache.setSignature("target1", "abc123");
    
    EXPECT_TRUE(cache.needsRebuild("target1", "xyz789"));
}

TEST_F(CacheTest, NeedsRebuildNonExistent)
{
    mimir::Cache cache(testDir_);
    
    EXPECT_TRUE(cache.needsRebuild("nonexistent", "abc123"));
}

TEST_F(CacheTest, OverwriteSignature)
{
    mimir::Cache cache(testDir_);
    
    cache.setSignature("target1", "old_sig");
    EXPECT_EQ(cache.getSignature("target1"), "old_sig");
    
    cache.setSignature("target1", "new_sig");
    EXPECT_EQ(cache.getSignature("target1"), "new_sig");
}

TEST_F(CacheTest, SizeEmpty)
{
    mimir::Cache cache(testDir_);
    
    EXPECT_EQ(cache.size(), 0u);
}

TEST_F(CacheTest, SizeAfterAdding)
{
    mimir::Cache cache(testDir_);
    
    cache.setSignature("target1", "sig1");
    cache.setSignature("target2", "sig2");
    cache.setSignature("target3", "sig3");
    
    EXPECT_EQ(cache.size(), 3u);
}

TEST_F(CacheTest, EmptyTrue)
{
    mimir::Cache cache(testDir_);
    
    EXPECT_TRUE(cache.empty());
}

TEST_F(CacheTest, EmptyFalse)
{
    mimir::Cache cache(testDir_);
    cache.setSignature("target", "sig");
    
    EXPECT_FALSE(cache.empty());
}

TEST_F(CacheTest, ClearRemovesAll)
{
    mimir::Cache cache(testDir_);
    cache.setSignature("target1", "sig1");
    cache.setSignature("target2", "sig2");
    
    cache.clear();
    
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.getSignature("target1"), "");
}

TEST_F(CacheTest, FindSignatureExists)
{
    mimir::Cache cache(testDir_);
    cache.setSignature("target1", "found_sig");
    
    auto result = cache.findSignature("target1");
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "found_sig");
}

TEST_F(CacheTest, FindSignatureNotExists)
{
    mimir::Cache cache(testDir_);
    
    auto result = cache.findSignature("nonexistent");
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(CacheTest, ConcurrentReads)
{
    mimir::Cache cache(testDir_);
    cache.setSignature("shared_target", "shared_sig");
    
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&cache, &successCount]()
        {
            for (int j = 0; j < 100; ++j)
            {
                if (cache.getSignature("shared_target") == "shared_sig")
                {
                    ++successCount;
                }
            }
        });
    }
    
    for (auto& t : threads)
    {
        t.join();
    }
    
    EXPECT_EQ(successCount.load(), 1000);
}

TEST_F(CacheTest, ConcurrentWrites)
{
    mimir::Cache cache(testDir_);
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&cache, i]()
        {
            for (int j = 0; j < 100; ++j)
            {
                cache.setSignature("target_" + std::to_string(i) + "_" + std::to_string(j), 
                                   "sig_" + std::to_string(i * 100 + j));
            }
        });
    }
    
    for (auto& t : threads)
    {
        t.join();
    }
    
    EXPECT_EQ(cache.size(), 1000u);
}

TEST_F(CacheTest, ConcurrentReadWrite)
{
    mimir::Cache cache(testDir_);
    std::atomic<bool> writerDone{false};
    std::atomic<bool> readersStarted{false};
    std::atomic<int> readCount{0};
    std::atomic<int> readersReady{0};
    
    cache.setSignature("target_0", "initial_sig");
    
    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i)
    {
        readers.emplace_back([&cache, &writerDone, &readCount, &readersReady]()
        {
            ++readersReady;
            while (!writerDone)
            {
                cache.getSignature("target_0");
                ++readCount;
            }
            for (int j = 0; j < 10; ++j)
            {
                cache.getSignature("target_0");
                ++readCount;
            }
        });
    }
    
    while (readersReady < 5)
    {
        std::this_thread::yield();
    }
    
    std::thread writer([&cache, &writerDone]()
    {
        for (int i = 0; i < 100; ++i)
        {
            cache.setSignature("target_" + std::to_string(i), "sig_" + std::to_string(i));
        }
        writerDone = true;
    });
    
    writer.join();
    for (auto& r : readers)
    {
        r.join();
    }
    
    EXPECT_GE(readCount.load(), 50);
}

TEST_F(CacheTest, DefaultCacheDirectory)
{
    fs::remove_all(".mimir");
    
    mimir::Cache cache;
    cache.setSignature("default_test", "default_sig");
    cache.save();
    
    EXPECT_TRUE(fs::exists(".mimir"));
    
    fs::remove_all(".mimir");
}

