#include "mimir/executor.h"
#include "mimir/dag.h"
#include "mimir/cache.h"
#include "mimir/signature.h"
#include "mimir/command_runner.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>

namespace fs = std::filesystem;

class ExecutorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testDir_ = "/tmp/test_executor_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed());
        fs::create_directories(testDir_);
        cacheDir_ = testDir_ + "/cache";
    }

    void TearDown() override
    {
        fs::remove_all(testDir_);
    }

    std::string createTestFile(const std::string& name, const std::string& content)
    {
        std::string path = testDir_ + "/" + name;
        std::ofstream file(path);
        file << content;
        file.close();
        return path;
    }

    std::string testDir_;
    std::string cacheDir_;
};


TEST_F(ExecutorTest, ConstructorSingleThread)
{
    mimir::Executor executor(1);
    SUCCEED();
}

TEST_F(ExecutorTest, ConstructorMultiThread)
{
    mimir::Executor executor(4);
    SUCCEED();
}

TEST_F(ExecutorTest, ConstructorDefaultThread)
{
    mimir::Executor executor;
    SUCCEED();
}

TEST_F(ExecutorTest, ExecuteEmptyDAG)
{
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
}

TEST_F(ExecutorTest, ExecuteEmptyDAGMultiThread)
{
    mimir::Executor executor(4);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
}

TEST_F(ExecutorTest, ExecuteSingleTargetEchoCommand)
{
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("echo_test");
    target.setCommand("echo 'hello world' > " + testDir_ + "/output.txt");
    target.addOutput(testDir_ + "/output.txt");
    
    dag.addTarget(target);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(testDir_ + "/output.txt"));
}

TEST_F(ExecutorTest, ExecuteTargetWithInputFile)
{
    std::string inputPath = createTestFile("input.txt", "test input content");
    std::string outputPath = testDir_ + "/output.txt";
    
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("copy_test");
    target.addInput(inputPath);
    target.addOutput(outputPath);
    target.setCommand("cp " + inputPath + " " + outputPath);
    
    dag.addTarget(target);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(outputPath));
}

TEST_F(ExecutorTest, ExecuteChainedTargets)
{
    std::string file1 = testDir_ + "/file1.txt";
    std::string file2 = testDir_ + "/file2.txt";
    std::string file3 = testDir_ + "/file3.txt";
    
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target t1("create_file1");
    t1.setCommand("echo 'step1' > " + file1);
    t1.addOutput(file1);
    
    mimir::Target t2("create_file2");
    t2.setCommand("cat " + file1 + " > " + file2 + " && echo 'step2' >> " + file2);
    t2.addInput(file1);
    t2.addOutput(file2);
    t2.addDependency("create_file1");
    
    mimir::Target t3("create_file3");
    t3.setCommand("cat " + file2 + " > " + file3 + " && echo 'step3' >> " + file3);
    t3.addInput(file2);
    t3.addOutput(file3);
    t3.addDependency("create_file2");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    dag.addTarget(t3);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(file1));
    EXPECT_TRUE(fs::exists(file2));
    EXPECT_TRUE(fs::exists(file3));
}

TEST_F(ExecutorTest, ExecuteFailingCommand) 
{
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("failing_test");
    target.setCommand("exit 1");
    target.addOutput(testDir_ + "/nonexistent.txt");
    
    dag.addTarget(target);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_FALSE(result);
}


TEST_F(ExecutorTest, IncrementalBuildSkipsUpToDate)
{
    std::string outputPath = testDir_ + "/output.txt";
    
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("echo_test");
    target.setCommand("echo 'hello' > " + outputPath);
    target.addOutput(outputPath);
    
    dag.addTarget(target);
    
    mimir::Executor executor1(1);
    bool result1 = executor1.execute(dag, cache);
    EXPECT_TRUE(result1);
    cache.save();
    
    auto modTime1 = fs::last_write_time(outputPath);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mimir::Cache cache2(cacheDir_);
    cache2.load();
    mimir::Executor executor2(1);
    bool result2 = executor2.execute(dag, cache2);
    EXPECT_TRUE(result2);
    
    auto modTime2 = fs::last_write_time(outputPath);
    EXPECT_EQ(modTime1, modTime2);
}


TEST_F(ExecutorTest, ExecuteParallelIndependentTargets)
{
    mimir::Executor executor(4);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    for (int i = 0; i < 5; ++i)
    {
        mimir::Target target("target_" + std::to_string(i));
        target.setCommand("echo 'target " + std::to_string(i) + "' > " + testDir_ + "/out_" + std::to_string(i) + ".txt");
        target.addOutput(testDir_ + "/out_" + std::to_string(i) + ".txt");
        dag.addTarget(target);
    }
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_TRUE(fs::exists(testDir_ + "/out_" + std::to_string(i) + ".txt"));
    }
}

TEST_F(ExecutorTest, ExecuteParallelWithDependencies)
{
    mimir::Executor executor(2);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target root("root");
    root.setCommand("echo 'root' > " + testDir_ + "/root.txt");
    root.addOutput(testDir_ + "/root.txt");
    
    mimir::Target child1("child1");
    child1.setCommand("echo 'child1' > " + testDir_ + "/child1.txt");
    child1.addOutput(testDir_ + "/child1.txt");
    child1.addDependency("root");
    
    mimir::Target child2("child2");
    child2.setCommand("echo 'child2' > " + testDir_ + "/child2.txt");
    child2.addOutput(testDir_ + "/child2.txt");
    child2.addDependency("root");
    
    mimir::Target final("final");
    final.setCommand("cat " + testDir_ + "/child1.txt " + testDir_ + "/child2.txt > " + testDir_ + "/final.txt");
    final.addInput(testDir_ + "/child1.txt");
    final.addInput(testDir_ + "/child2.txt");
    final.addOutput(testDir_ + "/final.txt");
    final.addDependency("child1");
    final.addDependency("child2");
    
    dag.addTarget(root);
    dag.addTarget(child1);
    dag.addTarget(child2);
    dag.addTarget(final);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(testDir_ + "/root.txt"));
    EXPECT_TRUE(fs::exists(testDir_ + "/child1.txt"));
    EXPECT_TRUE(fs::exists(testDir_ + "/child2.txt"));
    EXPECT_TRUE(fs::exists(testDir_ + "/final.txt"));
}


TEST_F(ExecutorTest, ExecuteTargetSuccess)
{
    mimir::Executor executor(1);
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("test_target");
    target.setCommand("echo 'success' > " + testDir_ + "/test_output.txt");
    target.addOutput(testDir_ + "/test_output.txt");
    
    bool result = executor.executeTarget(target, cache);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(testDir_ + "/test_output.txt"));
}

TEST_F(ExecutorTest, ExecuteTargetFailure)
{
    mimir::Executor executor(1);
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("failing_target");
    target.setCommand("exit 1");
    target.addOutput(testDir_ + "/nonexistent.txt");
    
    bool result = executor.executeTarget(target, cache);
    
    EXPECT_FALSE(result);
}

TEST_F(ExecutorTest, ExecuteTargetUpdatesCache)
{
    mimir::Executor executor(1);
    mimir::Cache cache(cacheDir_);
    
    std::string inputPath = createTestFile("input.txt", "test content");
    
    mimir::Target target("cached_target");
    target.addInput(inputPath);
    target.setCommand("echo 'done' > " + testDir_ + "/output.txt");
    target.addOutput(testDir_ + "/output.txt");
    
    EXPECT_EQ(cache.getSignature("cached_target"), "");
    
    bool result = executor.executeTarget(target, cache);
    
    EXPECT_TRUE(result);
    EXPECT_NE(cache.getSignature("cached_target"), "");
}

TEST_F(ExecutorTest, ExecutorConfigDefaults)
{
    mimir::ExecutorConfig config;
    
    EXPECT_EQ(config.numThreads, 1);
    EXPECT_FALSE(config.dryRun);
    EXPECT_FALSE(config.verbose);
    EXPECT_TRUE(config.stopOnError);
    EXPECT_TRUE(config.colorOutput);
}

TEST_F(ExecutorTest, ConstructorWithConfig)
{
    mimir::ExecutorConfig config;
    config.numThreads = 8;
    config.dryRun = true;
    config.verbose = true;
    
    mimir::Executor executor(config);
    
    EXPECT_EQ(executor.getConfig().numThreads, 8);
    EXPECT_TRUE(executor.getConfig().dryRun);
    EXPECT_TRUE(executor.getConfig().verbose);
}

TEST_F(ExecutorTest, SetConfig)
{
    mimir::Executor executor(1);
    
    mimir::ExecutorConfig newConfig;
    newConfig.numThreads = 4;
    newConfig.colorOutput = false;
    
    executor.setConfig(newConfig);
    
    EXPECT_EQ(executor.getConfig().numThreads, 4);
    EXPECT_FALSE(executor.getConfig().colorOutput);
}

TEST_F(ExecutorTest, BuildStatsDefaults)
{
    mimir::BuildStats stats;
    
    EXPECT_EQ(stats.totalTargets, 0u);
    EXPECT_EQ(stats.builtTargets, 0u);
    EXPECT_EQ(stats.skippedTargets, 0u);
    EXPECT_EQ(stats.failedTargets, 0u);
    EXPECT_EQ(stats.elapsedSeconds, 0.0);
}

TEST_F(ExecutorTest, ExecuteWithStatsSuccess)
{
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    mimir::BuildStats stats;
    
    mimir::Target target("stats_test");
    target.setCommand("echo 'test' > " + testDir_ + "/stats_out.txt");
    target.addOutput(testDir_ + "/stats_out.txt");
    dag.addTarget(target);
    
    bool result = executor.executeWithStats(dag, cache, stats);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(stats.totalTargets, 1u);
    EXPECT_EQ(stats.builtTargets, 1u);
    EXPECT_EQ(stats.failedTargets, 0u);
    EXPECT_GT(stats.elapsedSeconds, 0.0);
}

TEST_F(ExecutorTest, ExecuteWithStatsMultipleTargets)
{
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    mimir::BuildStats stats;
    
    for (int i = 0; i < 3; ++i)
    {
        mimir::Target target("target_" + std::to_string(i));
        target.setCommand("echo 'out' > " + testDir_ + "/out_" + std::to_string(i) + ".txt");
        target.addOutput(testDir_ + "/out_" + std::to_string(i) + ".txt");
        dag.addTarget(target);
    }
    
    bool result = executor.executeWithStats(dag, cache, stats);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(stats.totalTargets, 3u);
    EXPECT_EQ(stats.builtTargets, 3u);
}

TEST_F(ExecutorTest, ExecuteWithStatsSkippedTargets)
{
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("skip_test");
    target.setCommand("echo 'skip' > " + testDir_ + "/skip_out.txt");
    target.addOutput(testDir_ + "/skip_out.txt");
    dag.addTarget(target);
    
    mimir::Executor executor1(1);
    mimir::BuildStats stats1;
    executor1.executeWithStats(dag, cache, stats1);
    cache.save();
    
    mimir::Cache cache2(cacheDir_);
    cache2.load();
    mimir::Executor executor2(1);
    mimir::BuildStats stats2;
    executor2.executeWithStats(dag, cache2, stats2);
    
    EXPECT_EQ(stats2.skippedTargets, 1u);
    EXPECT_EQ(stats2.builtTargets, 0u);
}

TEST_F(ExecutorTest, ExecutorWithMockCommandRunner)
{
    auto mockRunner = std::make_shared<mimir::MockCommandRunner>();
    mimir::Executor executor(1, mockRunner);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("mock_test");
    target.setCommand("fake build command");
    target.addOutput(testDir_ + "/mock_out.txt");
    dag.addTarget(target);
    
    std::ofstream(testDir_ + "/mock_out.txt") << "mock";
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(mockRunner->getCommandCount(), 1u);
    EXPECT_EQ(mockRunner->getLastCommand(), "fake build command");
}

TEST_F(ExecutorTest, ExecutorWithMockFailure)
{
    auto mockRunner = std::make_shared<mimir::MockCommandRunner>();
    mimir::CommandResult failResult{1, "", "build failed", false};
    mockRunner->setDefaultResult(failResult);
    
    mimir::Executor executor(1, mockRunner);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("fail_mock");
    target.setCommand("will fail");
    target.addOutput(testDir_ + "/fail_out.txt");
    dag.addTarget(target);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_FALSE(result);
}

TEST_F(ExecutorTest, ProgressCallbackCalled)
{
    mimir::Executor executor(1);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    std::vector<std::string> progressUpdates;
    
    executor.setProgressCallback([&progressUpdates](
        const std::string& targetName,
        size_t current,
        size_t total,
        const std::string& status)
    {
        progressUpdates.push_back(targetName + ":" + status);
    });
    
    mimir::Target target("progress_test");
    target.setCommand("echo 'progress' > " + testDir_ + "/progress_out.txt");
    target.addOutput(testDir_ + "/progress_out.txt");
    dag.addTarget(target);
    
    executor.execute(dag, cache);
    
    EXPECT_FALSE(progressUpdates.empty());
}

TEST_F(ExecutorTest, CancelInitiallyFalse)
{
    mimir::Executor executor(1);
    
    EXPECT_FALSE(executor.isCancelled());
}

TEST_F(ExecutorTest, CancelSetsFlag)
{
    mimir::Executor executor(1);
    
    executor.cancel();
    
    EXPECT_TRUE(executor.isCancelled());
}

TEST_F(ExecutorTest, ResetCancelled)
{
    mimir::Executor executor(1);
    executor.cancel();
    
    executor.resetCancelled();
    
    EXPECT_FALSE(executor.isCancelled());
}

TEST_F(ExecutorTest, DryRunDoesNotExecuteCommands)
{
    mimir::ExecutorConfig config;
    config.dryRun = true;
    
    mimir::Executor executor(config);
    mimir::DAG dag;
    mimir::Cache cache(cacheDir_);
    
    std::string outputPath = testDir_ + "/dryrun_out.txt";
    
    mimir::Target target("dryrun_test");
    target.setCommand("echo 'should not run' > " + outputPath);
    target.addOutput(outputPath);
    dag.addTarget(target);
    
    bool result = executor.execute(dag, cache);
    
    EXPECT_TRUE(result);
    EXPECT_FALSE(fs::exists(outputPath));
}


TEST_F(ExecutorTest, InstanceExecuteTargetSuccess)
{
    mimir::Executor executor(1);
    mimir::Cache cache(cacheDir_);
    
    mimir::Target target("instance_test");
    target.setCommand("echo 'instance' > " + testDir_ + "/instance_out.txt");
    target.addOutput(testDir_ + "/instance_out.txt");
    
    bool result = executor.executeTarget(target, cache);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(testDir_ + "/instance_out.txt"));
}

TEST_F(ExecutorTest, InstanceExecuteTargetWithMock)
{
    auto mockRunner = std::make_shared<mimir::MockCommandRunner>();
    mimir::Executor executor(1, mockRunner);
    mimir::Cache cache(cacheDir_);
    
    std::ofstream(testDir_ + "/mock_instance.txt") << "x";
    
    mimir::Target target("mock_instance_test");
    target.setCommand("mock command here");
    target.addOutput(testDir_ + "/mock_instance.txt");
    
    executor.executeTarget(target, cache);
    
    EXPECT_EQ(mockRunner->getLastCommand(), "mock command here");
}

