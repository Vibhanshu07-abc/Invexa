#pragma once

#ifndef DECISION_ENGINE_H
#define DECISION_ENGINE_H

#include "inventory_analysis_service.h"

#include <string>
#include <vector>

struct BusinessRecommendation {
    std::string category;
    std::string priority;
    std::string title;
    std::string rationale;
    std::vector<int> relatedItemIds;
    std::vector<std::string> actions;
};

class DecisionEngine {
public:
    std::vector<BusinessRecommendation> generateRecommendations(
        const AnalysisSnapshot& snapshot) const;

private:
    void appendHashRecommendations(std::vector<BusinessRecommendation>& recommendations,
                                   const AnalysisSnapshot& snapshot) const;
    void appendHeapRecommendations(std::vector<BusinessRecommendation>& recommendations,
                                   const AnalysisSnapshot& snapshot) const;
    void appendDynamicProgrammingRecommendations(std::vector<BusinessRecommendation>& recommendations,
                                                 const AnalysisSnapshot& snapshot) const;
    void appendGreedyRecommendations(std::vector<BusinessRecommendation>& recommendations,
                                     const AnalysisSnapshot& snapshot) const;
    void appendGraphRecommendations(std::vector<BusinessRecommendation>& recommendations,
                                    const AnalysisSnapshot& snapshot) const;
    void appendAuditRecommendations(std::vector<BusinessRecommendation>& recommendations,
                                    const AnalysisSnapshot& snapshot) const;
    bool hasCategory(const std::vector<BusinessRecommendation>& recommendations,
                     const std::string& category) const;
};

#endif
