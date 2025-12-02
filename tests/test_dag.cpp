#include "mimir/dag.h"
#include <gtest/gtest.h>
#include <algorithm>

class DAGTest : public ::testing::Test
{
protected:
    mimir::DAG dag;
};

TEST_F(DAGTest, AddTargetAndGetTarget)
{
    mimir::Target t1("target1");
    dag.addTarget(t1);
    
    const mimir::Target* retrieved = dag.getTarget("target1");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "target1");
}

TEST_F(DAGTest, GetTargetNonExistent)
{
    const mimir::Target* retrieved = dag.getTarget("nonexistent");
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(DAGTest, GetAllTargets)
{
    mimir::Target t1("target1");
    mimir::Target t2("target2");
    mimir::Target t3("target3");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    dag.addTarget(t3);
    
    auto allTargets = dag.getAllTargets();
    EXPECT_EQ(allTargets.size(), 3);
}

TEST_F(DAGTest, NoCyclesLinearDependency)
{
    mimir::Target t1("target1");
    mimir::Target t2("target2");
    t2.addDependency("target1");
    mimir::Target t3("target3");
    t3.addDependency("target2");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    dag.addTarget(t3);
    
    EXPECT_FALSE(dag.detectCycles());
}

TEST_F(DAGTest, DetectSimpleCycle)
{
    mimir::Target c1("c1");
    c1.addDependency("c2");
    mimir::Target c2("c2");
    c2.addDependency("c1");
    
    dag.addTarget(c1);
    dag.addTarget(c2);
    
    EXPECT_TRUE(dag.detectCycles());
}

TEST_F(DAGTest, DetectSelfCycle)
{
    mimir::Target t1("target1");
    t1.addDependency("target1");
    
    dag.addTarget(t1);
    
    EXPECT_TRUE(dag.detectCycles());
}

TEST_F(DAGTest, TopologicalSortLinear)
{
    mimir::Target t1("target1");
    mimir::Target t2("target2");
    t2.addDependency("target1");
    mimir::Target t3("target3");
    t3.addDependency("target2");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    dag.addTarget(t3);
    
    auto order = dag.topologicalSort();
    ASSERT_EQ(order.size(), 3);
    
    size_t idx1 = std::find(order.begin(), order.end(), "target1") - order.begin();
    size_t idx2 = std::find(order.begin(), order.end(), "target2") - order.begin();
    size_t idx3 = std::find(order.begin(), order.end(), "target3") - order.begin();
    
    EXPECT_LT(idx1, idx2);
    EXPECT_LT(idx2, idx3);
}

TEST_F(DAGTest, TopologicalSortEmpty)
{
    auto order = dag.topologicalSort();
    EXPECT_TRUE(order.empty());
}

TEST_F(DAGTest, TopologicalSortSingleNode)
{
    mimir::Target t1("single");
    dag.addTarget(t1);
    
    auto order = dag.topologicalSort();
    ASSERT_EQ(order.size(), 1);
    EXPECT_EQ(order[0], "single");
}

TEST_F(DAGTest, TopologicalSortDiamond)
{
    mimir::Target a("A");
    mimir::Target b("B");
    b.addDependency("A");
    mimir::Target c("C");
    c.addDependency("A");
    mimir::Target d("D");
    d.addDependency("B");
    d.addDependency("C");
    
    dag.addTarget(a);
    dag.addTarget(b);
    dag.addTarget(c);
    dag.addTarget(d);
    
    EXPECT_FALSE(dag.detectCycles());
    
    auto order = dag.topologicalSort();
    ASSERT_EQ(order.size(), 4);
    
    size_t idxA = std::find(order.begin(), order.end(), "A") - order.begin();
    size_t idxB = std::find(order.begin(), order.end(), "B") - order.begin();
    size_t idxC = std::find(order.begin(), order.end(), "C") - order.begin();
    size_t idxD = std::find(order.begin(), order.end(), "D") - order.begin();
    
    EXPECT_LT(idxA, idxB);
    EXPECT_LT(idxA, idxC);
    EXPECT_LT(idxB, idxD);
    EXPECT_LT(idxC, idxD);
}

TEST_F(DAGTest, NoCyclesIndependentNodes)
{
    mimir::Target t1("target1");
    mimir::Target t2("target2");
    mimir::Target t3("target3");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    dag.addTarget(t3);
    
    EXPECT_FALSE(dag.detectCycles());
}

TEST_F(DAGTest, HasTargetTrue)
{
    mimir::Target t1("existing");
    dag.addTarget(t1);
    
    EXPECT_TRUE(dag.hasTarget("existing"));
}

TEST_F(DAGTest, HasTargetFalse)
{
    EXPECT_FALSE(dag.hasTarget("nonexistent"));
}

TEST_F(DAGTest, SizeEmpty)
{
    EXPECT_EQ(dag.size(), 0u);
}

TEST_F(DAGTest, SizeAfterAdding)
{
    dag.addTarget(mimir::Target("t1"));
    dag.addTarget(mimir::Target("t2"));
    dag.addTarget(mimir::Target("t3"));
    
    EXPECT_EQ(dag.size(), 3u);
}

TEST_F(DAGTest, EmptyTrue)
{
    EXPECT_TRUE(dag.empty());
}

TEST_F(DAGTest, EmptyFalse)
{
    dag.addTarget(mimir::Target("t1"));
    
    EXPECT_FALSE(dag.empty());
}

TEST_F(DAGTest, ClearRemovesAll)
{
    dag.addTarget(mimir::Target("t1"));
    dag.addTarget(mimir::Target("t2"));
    
    dag.clear();
    
    EXPECT_TRUE(dag.empty());
    EXPECT_EQ(dag.size(), 0u);
}

TEST_F(DAGTest, ValidateDependenciesAllExist)
{
    mimir::Target t1("t1");
    mimir::Target t2("t2");
    t2.addDependency("t1");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    
    auto missing = dag.validateDependencies();
    
    EXPECT_TRUE(missing.empty());
}

TEST_F(DAGTest, ValidateDependenciesMissing)
{
    mimir::Target t1("t1");
    t1.addDependency("nonexistent");
    
    dag.addTarget(t1);
    
    auto missing = dag.validateDependencies();
    
    ASSERT_EQ(missing.size(), 1u);
    EXPECT_EQ(missing[0], "nonexistent");
}

TEST_F(DAGTest, ValidateDependenciesMultipleMissing)
{
    mimir::Target t1("t1");
    t1.addDependency("missing1");
    t1.addDependency("missing2");
    
    mimir::Target t2("t2");
    t2.addDependency("missing1");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    
    auto missing = dag.validateDependencies();
    EXPECT_GE(missing.size(), 2u);
    
    bool hasMissing1 = std::find(missing.begin(), missing.end(), "missing1") != missing.end();
    bool hasMissing2 = std::find(missing.begin(), missing.end(), "missing2") != missing.end();
    EXPECT_TRUE(hasMissing1);
    EXPECT_TRUE(hasMissing2);
}

TEST_F(DAGTest, DetectCyclesWithResultNoCycle)
{
    mimir::Target t1("t1");
    mimir::Target t2("t2");
    t2.addDependency("t1");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    
    auto result = dag.detectCyclesWithResult();
    
    EXPECT_FALSE(result.hasCycle);
    EXPECT_TRUE(result.cycleNodes.empty());
}

TEST_F(DAGTest, DetectCyclesWithResultSimpleCycle)
{
    mimir::Target t1("t1");
    t1.addDependency("t2");
    mimir::Target t2("t2");
    t2.addDependency("t1");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    
    auto result = dag.detectCyclesWithResult();
    
    EXPECT_TRUE(result.hasCycle);
    EXPECT_FALSE(result.cycleNodes.empty());
}

TEST_F(DAGTest, GetDependentsNone)
{
    mimir::Target t1("t1");
    dag.addTarget(t1);
    
    auto dependents = dag.getDependents("t1");
    
    EXPECT_TRUE(dependents.empty());
}

TEST_F(DAGTest, GetDependentsSingle)
{
    mimir::Target t1("t1");
    mimir::Target t2("t2");
    t2.addDependency("t1");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    
    auto dependents = dag.getDependents("t1");
    
    ASSERT_EQ(dependents.size(), 1u);
    EXPECT_EQ(dependents[0], "t2");
}

TEST_F(DAGTest, GetDependentsMultiple)
{
    mimir::Target t1("t1");
    mimir::Target t2("t2");
    t2.addDependency("t1");
    mimir::Target t3("t3");
    t3.addDependency("t1");
    mimir::Target t4("t4");
    t4.addDependency("t1");
    
    dag.addTarget(t1);
    dag.addTarget(t2);
    dag.addTarget(t3);
    dag.addTarget(t4);
    
    auto dependents = dag.getDependents("t1");
    
    EXPECT_EQ(dependents.size(), 3u);
}

TEST_F(DAGTest, GetDependentsNonexistent)
{
    auto dependents = dag.getDependents("nonexistent");
    
    EXPECT_TRUE(dependents.empty());
}

TEST_F(DAGTest, AddTargetPtrAndRetrieve)
{
    auto target = mimir::makeTarget("ptr_target");
    target->setCommand("build command");
    target->addInput("input.cpp");
    
    dag.addTarget(*target);
    
    const mimir::Target* retrieved = dag.getTarget("ptr_target");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "ptr_target");
}

