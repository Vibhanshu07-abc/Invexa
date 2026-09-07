#include "../include/audit_history.h"
#include "../include/dp_algo.h"
#include "../include/graph_algo.h"
#include "../include/greedy_algo.h"
#include "../include/hash_algo.h"
#include "../include/heap_algo.h"
#include "../include/optimization_engine.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test_support;

TEST(AlgorithmTests, HashAlgoTracksFrequencyAndSortsMismatchesByLoss) {
    const ItemList items = sampleItems();
    HashAlgo algorithm(items);

    algorithm.trackFrequency();
    algorithm.detectMismatches();

    EXPECT_GE(algorithm.getMismatchCount(), 3);
    EXPECT_EQ(algorithm.getFrequencyMap().at(1), 2);
    EXPECT_GT(algorithm.getTotalLoss(), 0.0);
    EXPECT_EQ(algorithm.getMismatches().front().itemId, 1);
}

TEST(AlgorithmTests, HeapAlgoExtractsTopRiskItemsInDescendingOrder) {
    const ItemList items = sampleItems();
    HeapAlgo algorithm(items);

    const auto ranked = algorithm.extractTopK(3);

    ASSERT_EQ(ranked.size(), 3);
    EXPECT_EQ(ranked.front().rank, 1);
    EXPECT_GE(ranked[0].item.riskScore, ranked[1].item.riskScore);
    EXPECT_FALSE(ranked[0].badge.empty());
}

TEST(AlgorithmTests, DynamicProgrammingBuildsFeasibleRestockPlan) {
    const ItemList items = sampleItems();
    DPAlgo algorithm(items, 1200);

    const auto result = algorithm.solve();

    EXPECT_FALSE(result.selectedItems.empty());
    EXPECT_LE(result.totalWeight, result.budget);
    EXPECT_GT(result.totalValue, 0);
    EXPECT_EQ(result.status, "Optimal restock plan generated.");
}

TEST(AlgorithmTests, DynamicProgrammingReportsInsufficientBudget) {
    const ItemList items = sampleItems();
    DPAlgo algorithm(items, 1);

    const auto result = algorithm.solve();

    EXPECT_TRUE(result.selectedItems.empty());
    EXPECT_EQ(result.status, "Budget is too small to restock any lost item.");
}

TEST(AlgorithmTests, GreedyAlgoClassifiesRiskAndAggregatesLoss) {
    const ItemList items = sampleItems();
    GreedyAlgo algorithm(items);

    algorithm.classify();

    const auto classified = algorithm.getClassified();
    ASSERT_EQ(classified.size(), items.size());
    EXPECT_GE(classified.front().urgencyScore, classified.back().urgencyScore);
    EXPECT_GT(algorithm.riskCounts().at("HIGH"), 0);
    EXPECT_GT(algorithm.riskTotalLoss().at("HIGH"), 0.0);
}

TEST(AlgorithmTests, GraphAlgoBuildsEdgesAndClusters) {
    const ItemList items = sampleItems();
    GraphAlgo algorithm(items);

    algorithm.buildGraph();
    const auto clusters = algorithm.detectClusters();
    const auto bfs = algorithm.bfs(1);
    const auto dfs = algorithm.dfs(1);

    EXPECT_FALSE(algorithm.getEdges().empty());
    EXPECT_FALSE(clusters.empty());
    EXPECT_FALSE(bfs.empty());
    EXPECT_FALSE(dfs.empty());
}

TEST(AlgorithmTests, AuditHistoryCountsMismatchesAndFindsFirstMismatchByBinarySearch) {
    AuditHistory history = sampleAuditHistory();

    EXPECT_FALSE(history.empty());
    EXPECT_EQ(history.mismatchFrequency(1), 2);
    EXPECT_EQ(history.mismatchFrequency(2), 1);

    const auto timing = history.detectFirstMismatchMoments();
    ASSERT_FALSE(timing.empty());
    EXPECT_EQ(timing.front().itemId, 1);
    EXPECT_EQ(timing.front().previousAudit, "2026-08-05T08:00:00");
    EXPECT_EQ(timing.front().mismatchAudit, "2026-08-05T09:00:00");
}

TEST(AlgorithmTests, OptimizationEngineRefreshesFrequenciesAndExtractsTopRiskItems) {
    ItemList items = sampleItems();
    const AuditHistory history = sampleAuditHistory();
    OptimizationEngine engine(2);

    engine.refreshFrequencies(items, history);
    const auto topRisk = engine.extractTopRiskItems(items);
    const auto theftTiming = engine.detectTheftTiming(history);

    EXPECT_EQ(items[0].frequency, 2);
    ASSERT_EQ(topRisk.size(), 2);
    EXPECT_FALSE(theftTiming.empty());
}
