#pragma once

#ifndef INVENTORY_ANALYSIS_SERVICE_H
#define INVENTORY_ANALYSIS_SERVICE_H

#include "item.h"
#include "optimization_engine.h"

#include <string>
#include <vector>

struct ActionEntry {
    std::string type;
    int itemId;
    std::string itemName;
    std::string priority;
    std::string message;
};

struct SummaryReport {
    int totalItems;
    int lostItems;
    int misplacedItems;
    int highRiskItems;
    double estimatedFinancialLoss;
    std::vector<std::string> recommendedActions;
};

struct AnalysisSnapshot {
    std::vector<MismatchRecord> mismatches;
    std::vector<RankedItem> topRiskItems;
    std::vector<ClassifiedItem> classification;
    KnapsackResult optimization;
    std::vector<Cluster> clusters;
    std::vector<TheftTimingRecord> theftTiming;
    std::vector<Item> misplacedItems;
    std::vector<ActionEntry> actionReport;
    std::vector<std::string> alerts;
    SummaryReport summary;
    double totalLoss;
};

class InventoryAnalysisService {
public:
    explicit InventoryAnalysisService(int topK = 5);

    AnalysisSnapshot analyze(ItemList& items, const AuditHistory& history, int budget) const;

private:
    OptimizationEngine optimizationEngine_;

    std::vector<Item> getMisplacedItems(const ItemList& items) const;
    std::vector<ActionEntry> buildTransferRecommendations(
        const std::vector<ClassifiedItem>& classified,
        const std::vector<TransferCandidate>& transferCandidates) const;
    std::vector<ActionEntry> buildActionReport(const std::vector<Item>& misplacedItems,
                                               const std::vector<ClassifiedItem>& classified,
                                               const std::vector<TransferCandidate>& transferCandidates,
                                               const KnapsackResult& optimization,
                                               const std::vector<Cluster>& clusters) const;
    std::vector<std::string> buildAlerts(const AnalysisSnapshot& snapshot) const;
    SummaryReport buildSummary(const ItemList& items,
                               const std::vector<Item>& misplacedItems,
                               const std::vector<ClassifiedItem>& classified,
                               double totalLoss,
                               const std::vector<ActionEntry>& actions) const;
};

#endif
