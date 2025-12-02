#include "mimir/command_runner.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class CommandRunnerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testDir_ = "/tmp/test_command_runner_" + 
            std::to_string(::testing::UnitTest::GetInstance()->random_seed());
        fs::create_directories(testDir_);
    }

    void TearDown() override
    {
        fs::remove_all(testDir_);
    }

    std::string testDir_;
};

TEST_F(CommandRunnerTest, SystemCommandRunnerSimpleSuccess)
{
    mimir::SystemCommandRunner runner;
    
    bool result = runner.runSimple("true");
    
    EXPECT_TRUE(result);
}

TEST_F(CommandRunnerTest, SystemCommandRunnerSimpleFailure)
{
    mimir::SystemCommandRunner runner;
    
    bool result = runner.runSimple("false");
    
    EXPECT_FALSE(result);
}

TEST_F(CommandRunnerTest, SystemCommandRunnerCreatesFile)
{
    mimir::SystemCommandRunner runner;
    std::string outputFile = testDir_ + "/test_output.txt";
    
    bool result = runner.runSimple("echo 'hello' > " + outputFile);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(outputFile));
}

TEST_F(CommandRunnerTest, SystemCommandRunnerRunWithOptions)
{
    mimir::SystemCommandRunner runner;
    mimir::CommandOptions options;
    options.captureOutput = true;
    
    mimir::CommandResult result = runner.run("echo 'test output'", options);
    
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.exitCode, 0);
    EXPECT_FALSE(result.timedOut);
}

TEST_F(CommandRunnerTest, SystemCommandRunnerRunFailure)
{
    mimir::SystemCommandRunner runner;
    mimir::CommandOptions options;
    
    mimir::CommandResult result = runner.run("exit 42", options);
    
    EXPECT_FALSE(result.success());
    EXPECT_NE(result.exitCode, 0);
}

TEST_F(CommandRunnerTest, SystemCommandRunnerWithWorkingDir)
{
    mimir::SystemCommandRunner runner;
    mimir::CommandOptions options;
    options.workingDir = testDir_;
    
    mimir::CommandResult result = runner.run("touch workdir_test.txt", options);
    
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(fs::exists(testDir_ + "/workdir_test.txt"));
}

TEST_F(CommandRunnerTest, MockCommandRunnerDefaultSuccess)
{
    mimir::MockCommandRunner mock;
    
    bool result = mock.runSimple("any command");
    
    EXPECT_TRUE(result);
}

TEST_F(CommandRunnerTest, MockCommandRunnerSetDefaultResult)
{
    mimir::MockCommandRunner mock;
    mimir::CommandResult failResult{1, "", "error", false};
    mock.setDefaultResult(failResult);
    
    mimir::CommandResult result = mock.run("some command");
    
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.exitCode, 1);
}

TEST_F(CommandRunnerTest, MockCommandRunnerSetResultFor)
{
    mimir::MockCommandRunner mock;
    mimir::CommandResult specificResult{0, "specific output", "", false};
    mock.setResultFor("specific command", specificResult);
    
    mimir::CommandResult result1 = mock.run("specific command");
    mimir::CommandResult result2 = mock.run("other command");
    
    EXPECT_EQ(result1.stdOut, "specific output");
    EXPECT_NE(result2.stdOut, "specific output");
}

TEST_F(CommandRunnerTest, MockCommandRunnerTracksLastCommand)
{
    mimir::MockCommandRunner mock;
    
    mock.runSimple("first command");
    mock.runSimple("second command");
    
    EXPECT_EQ(mock.getLastCommand(), "second command");
}

TEST_F(CommandRunnerTest, MockCommandRunnerTracksCommandCount)
{
    mimir::MockCommandRunner mock;
    
    mock.runSimple("cmd1");
    mock.runSimple("cmd2");
    mock.runSimple("cmd3");
    
    EXPECT_EQ(mock.getCommandCount(), 3u);
}

TEST_F(CommandRunnerTest, MockCommandRunnerReset)
{
    mimir::MockCommandRunner mock;
    mock.runSimple("some command");
    
    mock.reset();
    
    EXPECT_EQ(mock.getCommandCount(), 0u);
    EXPECT_EQ(mock.getLastCommand(), "");
}

TEST_F(CommandRunnerTest, MockCommandRunnerCustomHandler)
{
    mimir::MockCommandRunner mock;
    int callCount = 0;
    
    mock.setHandler([&callCount](const std::string& cmd, const mimir::CommandOptions&) 
    {
        callCount++;
        mimir::CommandResult result;
        result.exitCode = cmd.find("fail") != std::string::npos ? 1 : 0;
        result.timedOut = false;
        return result;
    });
    
    EXPECT_TRUE(mock.run("success").success());
    EXPECT_FALSE(mock.run("fail").success());
    EXPECT_EQ(callCount, 2);
}

TEST_F(CommandRunnerTest, CommandResultSuccessTrue)
{
    mimir::CommandResult result{0, "output", "", false};
    
    EXPECT_TRUE(result.success());
}

TEST_F(CommandRunnerTest, CommandResultSuccessFalseExitCode)
{
    mimir::CommandResult result{1, "", "", false};
    
    EXPECT_FALSE(result.success());
}

TEST_F(CommandRunnerTest, CommandResultSuccessFalseTimedOut)
{
    mimir::CommandResult result{0, "", "", true};
    
    EXPECT_FALSE(result.success());
}

TEST_F(CommandRunnerTest, CreateDefaultCommandRunner)
{
    mimir::CommandRunnerPtr runner = mimir::createDefaultCommandRunner();
    
    ASSERT_NE(runner, nullptr);
    EXPECT_TRUE(runner->runSimple("true"));
}

TEST_F(CommandRunnerTest, CommandOptionsDefaults)
{
    mimir::CommandOptions options;
    
    EXPECT_EQ(options.workingDir, "");
    EXPECT_FALSE(options.timeoutSeconds.has_value());
    EXPECT_FALSE(options.captureOutput);
    EXPECT_TRUE(options.inheritEnvironment);
}

