#pragma once

#ifndef GEMINI_SERVICE_H
#define GEMINI_SERVICE_H

#include "decision_engine.h"
#include "item.h"

#include <string>
#include <vector>

struct RecommendationExplanation {
    std::string category;
    std::string title;
    std::string explanation;
    std::string status;
};

struct ExecutiveSummaryInsight {
    std::string summary;
    std::string status;
};

struct DemandForecastInsight {
    int itemId;
    std::string itemName;
    int currentDemand;
    int projectedDemand;
    std::string confidence;
    std::string explanation;
    std::string status;
};

struct GeminiInsightBundle {
    std::string provider;
    bool enabled;
    std::string sourceOfTruth;
    std::vector<RecommendationExplanation> recommendationExplanations;
    ExecutiveSummaryInsight executiveSummary;
    std::vector<DemandForecastInsight> demandForecasts;
};

class GeminiService {
public:
    GeminiInsightBundle generateInsights(const ItemList& items,
                                         const AnalysisSnapshot& snapshot,
                                         const std::vector<BusinessRecommendation>& recommendations) const;

private:
    bool isEnabled() const;
    std::string apiKey() const;
    std::string callModel(const std::string& prompt) const;
    std::string extractText(const std::string& response) const;
    std::string buildExplanationPrompt(const std::vector<BusinessRecommendation>& recommendations) const;
    std::string buildExecutiveSummaryPrompt(const AnalysisSnapshot& snapshot,
                                            const std::vector<BusinessRecommendation>& recommendations) const;
    std::string buildDemandForecastPrompt(const ItemList& items) const;
    std::vector<RecommendationExplanation> buildFallbackExplanations(
        const std::vector<BusinessRecommendation>& recommendations) const;
    ExecutiveSummaryInsight buildFallbackSummary(const AnalysisSnapshot& snapshot) const;
    std::vector<DemandForecastInsight> buildFallbackForecasts(const ItemList& items) const;
    std::vector<RecommendationExplanation> parseExplanationLines(const std::string& text) const;
    std::vector<DemandForecastInsight> parseForecastLines(const std::string& text) const;
    static std::string trim(const std::string& value);
};

#endif
