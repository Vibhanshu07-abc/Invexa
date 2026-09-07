#include "../include/ai_warehouse_assistant.h"
#include "../include/gemini_service.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test_support;

TEST(ForecastingTests, GeminiServiceFallsBackToDeterministicInsightsWhenDisabled) {
    GeminiService service;
    const ItemList items = sampleItems();
    const auto snapshot = sampleSnapshot();
    DecisionEngine engine;
    const auto recommendations = engine.generateRecommendations(snapshot);

    const auto insights = service.generateInsights(items, snapshot, recommendations);

    EXPECT_EQ(insights.provider, "google-gemini");
    EXPECT_FALSE(insights.enabled);
    EXPECT_FALSE(insights.recommendationExplanations.empty());
    EXPECT_EQ(insights.executiveSummary.status, "fallback");
    EXPECT_FALSE(insights.demandForecasts.empty());
    EXPECT_EQ(insights.demandForecasts.front().status, "fallback");
}

TEST(ForecastingTests, AssistantAnswersDemandQuestionUsingForecastFallback) {
    AIWarehouseAssistant assistant;
    const ItemList items = sampleItems();
    const auto snapshot = sampleSnapshot();

    const auto answer = assistant.answerQuestion(
        "What demand changes are expected?",
        items,
        snapshot,
        sampleWarehouses());

    EXPECT_EQ(answer.status, "answered-with-fallback");
    EXPECT_FALSE(answer.answer.empty());
    EXPECT_FALSE(answer.evidence.empty());
}
