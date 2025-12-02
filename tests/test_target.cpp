#include "mimir/target.h"
#include <gtest/gtest.h>
#include <memory>

class TargetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }
};

TEST_F(TargetTest, DefaultConstructor)
{
    mimir::Target target;
    
    EXPECT_EQ(target.getName(), "");
    EXPECT_TRUE(target.getInputs().empty());
    EXPECT_TRUE(target.getOutputs().empty());
    EXPECT_EQ(target.getCommand(), "");
    EXPECT_TRUE(target.getDependencies().empty());
}

TEST_F(TargetTest, NameConstructor)
{
    mimir::Target target("my_target");
    
    EXPECT_EQ(target.getName(), "my_target");
}

TEST_F(TargetTest, CopyConstructor)
{
    mimir::Target original("original");
    original.setCommand("echo test");
    original.addInput("input.txt");
    
    mimir::Target copy(original);
    
    EXPECT_EQ(copy.getName(), "original");
    EXPECT_EQ(copy.getCommand(), "echo test");
    EXPECT_EQ(copy.getInputs().size(), 1u);
}

TEST_F(TargetTest, MoveConstructor)
{
    mimir::Target original("movable");
    original.setCommand("make all");
    
    mimir::Target moved(std::move(original));
    
    EXPECT_EQ(moved.getName(), "movable");
    EXPECT_EQ(moved.getCommand(), "make all");
}

TEST_F(TargetTest, SetAndGetName)
{
    mimir::Target target;
    
    target.setName("new_name");
    
    EXPECT_EQ(target.getName(), "new_name");
}

TEST_F(TargetTest, SetAndGetCommand)
{
    mimir::Target target("test");
    
    target.setCommand("gcc -c main.c -o main.o");
    
    EXPECT_EQ(target.getCommand(), "gcc -c main.c -o main.o");
}

TEST_F(TargetTest, AddAndGetInputs)
{
    mimir::Target target("test");
    
    target.addInput("file1.c");
    target.addInput("file2.c");
    
    const auto& inputs = target.getInputs();
    EXPECT_EQ(inputs.size(), 2u);
    EXPECT_EQ(inputs[0], "file1.c");
    EXPECT_EQ(inputs[1], "file2.c");
}

TEST_F(TargetTest, SetInputs)
{
    mimir::Target target("test");
    target.addInput("old.c");
    
    target.setInputs({"new1.c", "new2.c", "new3.c"});
    
    EXPECT_EQ(target.getInputs().size(), 3u);
}

TEST_F(TargetTest, AddAndGetOutputs)
{
    mimir::Target target("test");
    
    target.addOutput("output.o");
    target.addOutput("output.d");
    
    const auto& outputs = target.getOutputs();
    EXPECT_EQ(outputs.size(), 2u);
}

TEST_F(TargetTest, SetOutputs)
{
    mimir::Target target("test");
    
    target.setOutputs({"a.out", "a.map"});
    
    EXPECT_EQ(target.getOutputs().size(), 2u);
}

TEST_F(TargetTest, AddAndGetDependencies)
{
    mimir::Target target("app");
    
    target.addDependency("lib1");
    target.addDependency("lib2");
    
    const auto& deps = target.getDependencies();
    EXPECT_EQ(deps.size(), 2u);
    EXPECT_EQ(deps[0], "lib1");
    EXPECT_EQ(deps[1], "lib2");
}

TEST_F(TargetTest, SetDependencies)
{
    mimir::Target target("test");
    
    target.setDependencies({"dep1", "dep2"});
    
    EXPECT_EQ(target.getDependencies().size(), 2u);
}

TEST_F(TargetTest, SetAndGetSignature)
{
    mimir::Target target("test");
    
    target.setSignature("abc123def456");
    
    EXPECT_EQ(target.getSignature(), "abc123def456");
}

TEST_F(TargetTest, HasDependenciesEmpty)
{
    mimir::Target target("test");
    
    EXPECT_FALSE(target.hasDependencies());
}

TEST_F(TargetTest, HasDependenciesWithDeps)
{
    mimir::Target target("test");
    target.addDependency("other");
    
    EXPECT_TRUE(target.hasDependencies());
}

TEST_F(TargetTest, HasInputsEmpty)
{
    mimir::Target target("test");
    
    EXPECT_FALSE(target.hasInputs());
}

TEST_F(TargetTest, HasInputsWithInputs)
{
    mimir::Target target("test");
    target.addInput("source.c");
    
    EXPECT_TRUE(target.hasInputs());
}

TEST_F(TargetTest, HasOutputsEmpty)
{
    mimir::Target target("test");
    
    EXPECT_FALSE(target.hasOutputs());
}

TEST_F(TargetTest, HasOutputsWithOutputs)
{
    mimir::Target target("test");
    target.addOutput("output.o");
    
    EXPECT_TRUE(target.hasOutputs());
}

TEST_F(TargetTest, MakeTargetFactory)
{
    mimir::TargetPtr target = mimir::makeTarget("shared_target");
    
    ASSERT_NE(target, nullptr);
    EXPECT_EQ(target->getName(), "shared_target");
}

TEST_F(TargetTest, TargetPtrModification)
{
    mimir::TargetPtr target = mimir::makeTarget("ptr_test");
    
    target->setCommand("build command");
    target->addInput("input.cpp");
    
    EXPECT_EQ(target->getCommand(), "build command");
    EXPECT_EQ(target->getInputs().size(), 1u);
}

TEST_F(TargetTest, TargetWeakPtrUsage)
{
    mimir::TargetWeakPtr weakTarget;
    
    {
        mimir::TargetPtr target = mimir::makeTarget("temp");
        weakTarget = target;
        
        EXPECT_FALSE(weakTarget.expired());
        
        if (auto locked = weakTarget.lock())
        {
            EXPECT_EQ(locked->getName(), "temp");
        }
    }
    
    EXPECT_TRUE(weakTarget.expired());
}


TEST_F(TargetTest, BackwardCompatibilityPublicMembers)
{
    mimir::Target target("compat_test");
    
    target.setCommand("new command");
    target.addInput("input.c");
    target.addOutput("output.o");
    target.addDependency("dep");
    
    EXPECT_EQ(target.getCommand(), "new command");
    EXPECT_EQ(target.getInputs().size(), 1u);
    EXPECT_EQ(target.getOutputs().size(), 1u);
    EXPECT_EQ(target.getDependencies().size(), 1u);
}

TEST_F(TargetTest, MixedAccessorAndPublicMember)
{
    mimir::Target target("mixed");
    
    target.setCommand("new api command");
    target.addInput("input.c");
    
    EXPECT_EQ(target.getCommand(), "new api command");
    EXPECT_EQ(target.getInputs().size(), 1u);
}

