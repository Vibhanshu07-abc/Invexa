#include "../include/decision_engine.h"

#include "test_helpers.h"

#include <gtest/gtest.h>
#include <algorithm>

using namespace test_support;

TEST(DecisionEngineTests, GeneratesRecommendationsAcrossAnalysisDomains) {
    const auto snapshot = sampleSnapshot();
    DecisionEngine engine;

    const auto recommendations = engine.generateRecommendations(snapshot);

    ASSERT_GE(recommendations.size(), 4);

    auto hasCategory = [&](const std::string& category) {
        return std::any_of(recommendations.begin(), recommendations.end(),
            [&](const BusinessRecommendation& recommendation) {
                return recommendation.category == category;
            });
    };

    EXPECT_TRUE(hasCategory("inventory-control"));
    EXPECT_TRUE(hasCategory("risk-prioritization"));
    EXPECT_TRUE(hasCategory("budget-allocation"));
    EXPECT_TRUE(hasCategory("operational-triage"));
    EXPECT_TRUE(hasCategory("warehouse-network"));
    EXPECT_TRUE(hasCategory("audit-intelligence"));
}

TEST(DecisionEngineTests, RecommendationActionsAreBoundedAndMeaningful) {
    const auto snapshot = sampleSnapshot();
    DecisionEngine engine;

    const auto recommendations = engine.generateRecommendations(snapshot);

    for (const auto& recommendation : recommendations) {
        EXPECT_FALSE(recommendation.title.empty());
        EXPECT_FALSE(recommendation.rationale.empty());
        EXPECT_LE(recommendation.actions.size(), 3);
    }
}
