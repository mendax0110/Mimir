#include "mimir/signature.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class SignatureTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testFile_ = "/tmp/test_sig_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + ".txt";
    }

    void TearDown() override
    {
        fs::remove(testFile_);
    }

    std::string testFile_;
};

TEST_F(SignatureTest, ComputeFileSignatureConsistent)
{
    std::ofstream file(testFile_);
    file << "test content";
    file.close();
    
    std::string sig1 = mimir::Signature::computeFileSignature(testFile_);
    std::string sig2 = mimir::Signature::computeFileSignature(testFile_);
    
    EXPECT_FALSE(sig1.empty());
    EXPECT_EQ(sig1, sig2);
}

TEST_F(SignatureTest, ComputeFileSignatureChangesWithContent)
{
    std::ofstream file1(testFile_);
    file1 << "test content";
    file1.close();
    
    std::string sig1 = mimir::Signature::computeFileSignature(testFile_);
    
    std::ofstream file2(testFile_);
    file2 << "different content";
    file2.close();
    
    std::string sig2 = mimir::Signature::computeFileSignature(testFile_);
    
    EXPECT_NE(sig1, sig2);
}

TEST_F(SignatureTest, ComputeFileSignatureNonExistent)
{
    std::string sig = mimir::Signature::computeFileSignature("/nonexistent/file/path.txt");
    EXPECT_TRUE(sig.empty());
}

TEST_F(SignatureTest, ComputeCommandSignatureConsistent)
{
    std::string sig1 = mimir::Signature::computeCommandSignature("gcc -o test test.c");
    std::string sig2 = mimir::Signature::computeCommandSignature("gcc -o test test.c");
    
    EXPECT_FALSE(sig1.empty());
    EXPECT_EQ(sig1, sig2);
}

TEST_F(SignatureTest, ComputeCommandSignatureDifferentCommands)
{
    std::string sig1 = mimir::Signature::computeCommandSignature("gcc -o test test.c");
    std::string sig2 = mimir::Signature::computeCommandSignature("gcc -O2 -o test test.c");
    
    EXPECT_NE(sig1, sig2);
}

TEST_F(SignatureTest, ComputeCommandSignatureEmptyCommand)
{
    std::string sig = mimir::Signature::computeCommandSignature("");
    EXPECT_FALSE(sig.empty());
}

TEST_F(SignatureTest, ComputeTargetSignatureConsistent)
{
    std::ofstream file(testFile_);
    file << "test content";
    file.close();
    
    std::vector<std::string> inputs = {testFile_};
    std::string command = "gcc -c test.c -o test.o";
    
    std::string sig1 = mimir::Signature::computeTargetSignature(command, inputs);
    std::string sig2 = mimir::Signature::computeTargetSignature(command, inputs);
    
    EXPECT_FALSE(sig1.empty());
    EXPECT_EQ(sig1, sig2);
}

TEST_F(SignatureTest, ComputeTargetSignatureChangesWithCommand)
{
    std::ofstream file(testFile_);
    file << "test content";
    file.close();
    
    std::vector<std::string> inputs = {testFile_};
    
    std::string sig1 = mimir::Signature::computeTargetSignature("gcc -c test.c -o test.o", inputs);
    std::string sig2 = mimir::Signature::computeTargetSignature("gcc -O2 -c test.c -o test.o", inputs);
    
    EXPECT_NE(sig1, sig2);
}

TEST_F(SignatureTest, ComputeTargetSignatureChangesWithInputContent)
{
    std::ofstream file1(testFile_);
    file1 << "content1";
    file1.close();
    
    std::vector<std::string> inputs = {testFile_};
    std::string command = "gcc -c test.c -o test.o";
    
    std::string sig1 = mimir::Signature::computeTargetSignature(command, inputs);
    
    std::ofstream file2(testFile_);
    file2 << "content2";
    file2.close();
    
    std::string sig2 = mimir::Signature::computeTargetSignature(command, inputs);
    
    EXPECT_NE(sig1, sig2);
}

TEST_F(SignatureTest, ComputeTargetSignatureEmptyInputs)
{
    std::vector<std::string> inputs;
    std::string command = "echo hello";
    
    std::string sig = mimir::Signature::computeTargetSignature(command, inputs);
    EXPECT_FALSE(sig.empty());
}

TEST_F(SignatureTest, ComputeTargetSignatureMultipleInputs)
{
    std::string testFile2 = testFile_ + "_2";
    
    std::ofstream file1(testFile_);
    file1 << "content1";
    file1.close();
    
    std::ofstream file2(testFile2);
    file2 << "content2";
    file2.close();
    
    std::vector<std::string> inputs = {testFile_, testFile2};
    std::string command = "gcc -c test.c -o test.o";
    
    std::string sig = mimir::Signature::computeTargetSignature(command, inputs);
    EXPECT_FALSE(sig.empty());
    
    fs::remove(testFile2);
}
