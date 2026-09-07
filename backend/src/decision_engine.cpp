#include "../include/decision_engine.h"

#include <algorithm>

using namespace std;

vector<BusinessRecommendation> DecisionEngine::generateRecommendations(
    const AnalysisSnapshot& snapshot) const {
    vector<BusinessRecommendation> recommendations;

    appendHashRecommendations(recommendations, snapshot);
    appendHeapRecommendations(recommendations, snapshot);
    appendDynamicProgrammingRecommendations(recommendations, snapshot);
    appendGreedyRecommendations(recommendations, snapshot);
    appendGraphRecommendations(recommendations, snapshot);
    appendAuditRecommendations(recommendations, snapshot);

    return recommendations;
}

void DecisionEngine::appendHashRecommendations(vector<BusinessRecommendation>& recommendations,
                                               const AnalysisSnapshot& snapshot) const {
    if (snapshot.mismatches.empty()) {
        return;
    }

    BusinessRecommendation recommendation;
    recommendation.category = "inventory-control";
    recommendation.priority = snapshot.summary.highRiskItems > 0 ? "HIGH" : "MEDIUM";
    recommendation.title = "Resolve stock mismatches before they expand into shrinkage";
    recommendation.rationale =
        to_string(snapshot.mismatches.size()) + " mismatched items are generating an estimated loss of " +
        to_string(static_cast<int>(snapshot.totalLoss)) + ".";

    for (const auto& mismatch : snapshot.mismatches) {
        recommendation.relatedItemIds.push_back(mismatch.itemId);
        if (recommendation.actions.size() == 3) {
            continue;
        }

        recommendation.actions.push_back(
            "Investigate " + mismatch.itemName + " and apply " + mismatch.action + ".");
    }

    recommendations.push_back(recommendation);
}

void DecisionEngine::appendHeapRecommendations(vector<BusinessRecommendation>& recommendations,
                                               const AnalysisSnapshot& snapshot) const {
    if (snapshot.topRiskItems.empty()) {
        return;
    }

    BusinessRecommendation recommendation;
    recommendation.category = "risk-prioritization";
    recommendation.priority = "HIGH";
    recommendation.title = "Escalate the highest-risk inventory first";
    recommendation.rationale =
        "Heap-based ranking identified the most exposed items for immediate managerial focus.";

    for (const auto& rankedItem : snapshot.topRiskItems) {
        recommendation.relatedItemIds.push_back(rankedItem.item.id);
        if (recommendation.actions.size() == 3) {
            continue;
        }

        recommendation.actions.push_back(
            "Review rank #" + to_string(rankedItem.rank) + " item " + rankedItem.item.name +
            " with badge " + rankedItem.badge + ".");
    }

    recommendations.push_back(recommendation);
}

void DecisionEngine::appendDynamicProgrammingRecommendations(
    vector<BusinessRecommendation>& recommendations,
    const AnalysisSnapshot& snapshot) const {
    if (snapshot.optimization.selectedItems.empty()) {
        return;
    }

    BusinessRecommendation recommendation;
    recommendation.category = "budget-allocation";
    recommendation.priority = "HIGH";
    recommendation.title = "Execute the optimal replenishment plan after transfer checks";
    recommendation.rationale = snapshot.optimization.status +
        " Budget efficiency is " + to_string(static_cast<int>(snapshot.optimization.efficiency)) + " percent.";

    for (const auto& item : snapshot.optimization.selectedItems) {
        recommendation.relatedItemIds.push_back(item.itemId);
        if (recommendation.actions.size() == 3) {
            continue;
        }

        recommendation.actions.push_back(
            "Allocate budget to " + item.name + " with priority " + item.priority + ".");
    }

    recommendations.push_back(recommendation);
}

void DecisionEngine::appendGreedyRecommendations(vector<BusinessRecommendation>& recommendations,
                                                 const AnalysisSnapshot& snapshot) const {
    int highRiskCount = 0;
    vector<int> highRiskItemIds;
    vector<string> highRiskActions;

    for (const auto& item : snapshot.classification) {
        if (item.riskLevel != "HIGH") {
            continue;
        }

        highRiskCount++;
        highRiskItemIds.push_back(item.item.id);
        if (highRiskActions.size() < 3) {
            highRiskActions.push_back(
                "Perform targeted review for " + item.item.name + ": " + item.explanation + ".");
        }
    }

    if (highRiskCount == 0) {
        return;
    }

    BusinessRecommendation recommendation;
    recommendation.category = "operational-triage";
    recommendation.priority = "HIGH";
    recommendation.title = "Triage high-risk items using urgency-based ordering";
    recommendation.rationale =
        "Greedy classification marked " + to_string(highRiskCount) + " items as high-risk.";
    recommendation.relatedItemIds = highRiskItemIds;
    recommendation.actions = highRiskActions;
    recommendations.push_back(recommendation);
}

void DecisionEngine::appendGraphRecommendations(vector<BusinessRecommendation>& recommendations,
                                                const AnalysisSnapshot& snapshot) const {
    if (snapshot.clusters.empty()) {
        return;
    }

    BusinessRecommendation recommendation;
    recommendation.category = "warehouse-network";
    recommendation.priority = "MEDIUM";
    recommendation.title = "Review linked inventory clusters for transfer and control dependencies";

    int suspiciousClusters = 0;
    for (const auto& cluster : snapshot.clusters) {
        if (cluster.label == "stable cluster") {
            continue;
        }

        suspiciousClusters++;
        recommendation.relatedItemIds.insert(recommendation.relatedItemIds.end(),
                                             cluster.itemIds.begin(),
                                             cluster.itemIds.end());
        if (recommendation.actions.size() < 3) {
            recommendation.actions.push_back(
                "Inspect " + cluster.label + " cluster #" + to_string(cluster.clusterId) +
                " affecting " + to_string(cluster.itemIds.size()) + " items.");
        }
    }

    if (suspiciousClusters == 0) {
        return;
    }

    recommendation.priority = suspiciousClusters > 1 ? "HIGH" : "MEDIUM";
    recommendation.rationale =
        "Graph analysis detected " + to_string(suspiciousClusters) +
        " non-stable inventory clusters that may affect warehouse decisions.";
    recommendations.push_back(recommendation);
}

void DecisionEngine::appendAuditRecommendations(vector<BusinessRecommendation>& recommendations,
                                                const AnalysisSnapshot& snapshot) const {
    if (snapshot.theftTiming.empty()) {
        return;
    }

    BusinessRecommendation recommendation;
    recommendation.category = "audit-intelligence";
    recommendation.priority = "HIGH";
    recommendation.title = "Investigate first-seen mismatch windows from audit history";
    recommendation.rationale =
        "Audit comparison found " + to_string(snapshot.theftTiming.size()) +
        " items with traceable mismatch timing.";

    for (const auto& timing : snapshot.theftTiming) {
        recommendation.relatedItemIds.push_back(timing.itemId);
        if (recommendation.actions.size() == 3) {
            continue;
        }

        recommendation.actions.push_back(
            "Audit " + timing.itemName + " between " + timing.previousAudit +
            " and " + timing.mismatchAudit + ".");
    }

    recommendations.push_back(recommendation);
}

bool DecisionEngine::hasCategory(const vector<BusinessRecommendation>& recommendations,
                                 const string& category) const {
    return any_of(recommendations.begin(), recommendations.end(),
        [&](const BusinessRecommendation& recommendation) {
            return recommendation.category == category;
        });
}
