#include "mimir/parser.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class ParserTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testDir_ = "/tmp/test_parser_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed());
        fs::create_directories(testDir_);
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
    mimir::Parser parser_;
};


TEST_F(ParserTest, ParseYAMLValidSingleTarget)
{
    std::string yaml = R"(
targets:
  - name: compile_main
    inputs:
      - main.c
    outputs:
      - main.o
    command: gcc -c main.c -o main.o
    dependencies:
)";
    std::string path = createTestFile("build.yaml", yaml);
    
    auto targets = parser_.parseYAML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].getName(), "compile_main");
    ASSERT_EQ(targets[0].getInputs().size(), 1);
    EXPECT_EQ(targets[0].getInputs()[0], "main.c");
    ASSERT_EQ(targets[0].getOutputs().size(), 1);
    EXPECT_EQ(targets[0].getOutputs()[0], "main.o");
    EXPECT_EQ(targets[0].getCommand(), "gcc -c main.c -o main.o");
}

TEST_F(ParserTest, ParseYAMLMultipleTargets)
{
    std::string yaml = R"(
targets:
  - name: compile_main
    inputs:
      - main.c
    outputs:
      - main.o
    command: gcc -c main.c -o main.o
    dependencies:

  - name: link_program
    inputs:
      - main.o
    outputs:
      - program
    command: gcc main.o -o program
    dependencies:
      - compile_main
)";
    std::string path = createTestFile("build.yaml", yaml);
    
    auto targets = parser_.parseYAML(path);
    
    ASSERT_EQ(targets.size(), 2);
    EXPECT_EQ(targets[0].getName(), "compile_main");
    EXPECT_EQ(targets[1].getName(), "link_program");
    ASSERT_EQ(targets[1].getDependencies().size(), 1);
    EXPECT_EQ(targets[1].getDependencies()[0], "compile_main");
}

TEST_F(ParserTest, ParseYAMLEmptyFile)
{
    std::string path = createTestFile("empty.yaml", "");
    
    auto targets = parser_.parseYAML(path);
    
    EXPECT_TRUE(targets.empty());
}

TEST_F(ParserTest, ParseYAMLNonExistentFile)
{
    auto targets = parser_.parseYAML("/nonexistent/path/build.yaml");
    
    EXPECT_TRUE(targets.empty());
}

TEST_F(ParserTest, ParseYAMLWithVariables)
{
    std::string yaml = R"(
variables:
  CC: gcc
  CFLAGS: -Wall -O2

targets:
  - name: compile
    inputs:
      - main.c
    outputs:
      - main.o
    command: ${CC} ${CFLAGS} -c main.c -o main.o
    dependencies:
)";
    std::string path = createTestFile("build.yaml", yaml);
    
    auto targets = parser_.parseYAML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_NE(targets[0].getCommand().find("gcc"), std::string::npos);
    EXPECT_NE(targets[0].getCommand().find("-Wall -O2"), std::string::npos);
}

TEST_F(ParserTest, ParseYAMLMultilineCommand)
{
    std::string yaml = R"(
targets:
  - name: multiline_cmd
    inputs:
      - main.c
    outputs:
      - main.o
    command: |
      gcc -c main.c -o main.o
      echo done
    dependencies:
)";
    std::string path = createTestFile("build.yaml", yaml);
    
    auto targets = parser_.parseYAML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_NE(targets[0].getCommand().find("gcc"), std::string::npos);
}

TEST_F(ParserTest, ParseYAMLWithComments)
{
    std::string yaml = R"(
# This is a comment
targets:
  # Another comment
  - name: compile
    inputs:
      - main.c
    outputs:
      - main.o
    command: gcc -c main.c -o main.o
    dependencies:
)";
    std::string path = createTestFile("build.yaml", yaml);
    
    auto targets = parser_.parseYAML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].getName(), "compile");
}


TEST_F(ParserTest, ParseTOMLValidSingleTarget)
{
    std::string toml = R"(
[target.compile_main]
name = "compile_main"
inputs = ["main.c"]
outputs = ["main.o"]
command = "gcc -c main.c -o main.o"
dependencies = []
)";
    std::string path = createTestFile("build.toml", toml);
    
    auto targets = parser_.parseTOML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].getName(), "compile_main");
    ASSERT_EQ(targets[0].getInputs().size(), 1);
    EXPECT_EQ(targets[0].getInputs()[0], "main.c");
    ASSERT_EQ(targets[0].getOutputs().size(), 1);
    EXPECT_EQ(targets[0].getOutputs()[0], "main.o");
    EXPECT_EQ(targets[0].getCommand(), "gcc -c main.c -o main.o");
}

TEST_F(ParserTest, ParseTOMLMultipleTargets)
{
    std::string toml = R"(
[target.compile_main]
name = "compile_main"
inputs = ["main.c"]
outputs = ["main.o"]
command = "gcc -c main.c -o main.o"
dependencies = []

[target.link_program]
name = "link_program"
inputs = ["main.o"]
outputs = ["program"]
command = "gcc main.o -o program"
dependencies = ["compile_main"]
)";
    std::string path = createTestFile("build.toml", toml);
    
    auto targets = parser_.parseTOML(path);
    
    ASSERT_EQ(targets.size(), 2);
}

TEST_F(ParserTest, ParseTOMLEmptyFile)
{
    std::string path = createTestFile("empty.toml", "");
    
    auto targets = parser_.parseTOML(path);
    
    EXPECT_TRUE(targets.empty());
}

TEST_F(ParserTest, ParseTOMLNonExistentFile)
{
    auto targets = parser_.parseTOML("/nonexistent/path/build.toml");
    
    EXPECT_TRUE(targets.empty());
}

TEST_F(ParserTest, ParseTOMLWithComments)
{
    std::string toml = R"(
# This is a comment
[target.compile]
name = "compile"
inputs = ["main.c"]
outputs = ["main.o"]
command = "gcc -c main.c -o main.o"
dependencies = []
)";
    std::string path = createTestFile("build.toml", toml);
    
    auto targets = parser_.parseTOML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].getName(), "compile");
}

TEST_F(ParserTest, ParseTOMLMultipleInputsOutputs)
{
    std::string toml = R"(
[target.link]
name = "link"
inputs = ["main.o", "utils.o", "helper.o"]
outputs = ["program", "program.map"]
command = "gcc main.o utils.o helper.o -o program -Wl,-Map=program.map"
dependencies = []
)";
    std::string path = createTestFile("build.toml", toml);
    
    auto targets = parser_.parseTOML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].getInputs().size(), 3);
    EXPECT_EQ(targets[0].getOutputs().size(), 2);
}


TEST_F(ParserTest, ParseYAMLEmptyDependencies)
{
    std::string yaml = R"(
targets:
  - name: standalone
    inputs:
      - file.c
    outputs:
      - file.o
    command: gcc -c file.c -o file.o
    dependencies:
)";
    std::string path = createTestFile("build.yaml", yaml);
    
    auto targets = parser_.parseYAML(path);
    
    ASSERT_EQ(targets.size(), 1);
    EXPECT_TRUE(targets[0].getDependencies().empty());
}

TEST_F(ParserTest, ParseYAMLMultipleDependencies)
{
    std::string yaml = R"(
targets:
  - name: final
    inputs:
      - all.o
    outputs:
      - program
    command: gcc all.o -o program
    dependencies:
      - dep1
      - dep2
      - dep3
)";
    std::string path = createTestFile("build.yaml", yaml);
    
    auto targets = parser_.parseYAML(path);
    
    ASSERT_EQ(targets.size(), 1);
    ASSERT_EQ(targets[0].getDependencies().size(), 3);
    EXPECT_EQ(targets[0].getDependencies()[0], "dep1");
    EXPECT_EQ(targets[0].getDependencies()[1], "dep2");
    EXPECT_EQ(targets[0].getDependencies()[2], "dep3");
}
