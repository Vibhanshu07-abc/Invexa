#include "../include/inventory_analysis_service.h"
#include "../include/config_service.h"
#include "../include/monitoring_registry.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <set>

using namespace std;

InventoryAnalysisService::InventoryAnalysisService(int topK)
    : optimizationEngine_(topK) {}

vector<Item> InventoryAnalysisService::getMisplacedItems(const ItemList& items) const {
    vector<Item> misplaced;
    for (const auto& item : items) {
        if (item.isMisplaced()) {
            misplaced.push_back(item);
        }
    }

    sort(misplaced.begin(), misplaced.end(),
        [](const Item& a, const Item& b) {
            if (a.riskScore == b.riskScore) return a.id < b.id;
            return a.riskScore > b.riskScore;
        });

    return misplaced;
}

vector<ActionEntry> InventoryAnalysisService::buildTransferRecommendations(
    const vector<ClassifiedItem>& classified,
    const vector<TransferCandidate>& transferCandidates) const {
    map<int, ClassifiedItem> classifiedByItemId;
    for (const auto& item : classified) {
        classifiedByItemId[item.item.id] = item;
    }

    vector<ActionEntry> actions;
    set<int> recommendedItems;

    for (const auto& candidate : transferCandidates) {
        auto classifiedIt = classifiedByItemId.find(candidate.itemId);
        if (classifiedIt == classifiedByItemId.end()) {
            continue;
        }

        const ClassifiedItem& classifiedItem = classifiedIt->second;
        if (classifiedItem.item.mismatch <= 0) {
            continue;
        }

        if (classifiedItem.riskLevel == "LOW") {
            continue;
        }

        if (!recommendedItems.insert(candidate.itemId).second) {
            continue;
        }

        ActionEntry entry;
        entry.type = "transfer";
        entry.itemId = candidate.itemId;
        entry.itemName = candidate.itemName;
        entry.priority = classifiedItem.riskLevel;
        entry.message = "Check transfer availability for " + candidate.itemName +
                        " from " + candidate.toWarehouseName +
                        " to support " + candidate.fromWarehouseName +
                        " before external purchase (" + candidate.reason + ").";
        actions.push_back(entry);
    }

    return actions;
}

vector<ActionEntry> InventoryAnalysisService::buildActionReport(
    const vector<Item>& misplacedItems,
    const vector<ClassifiedItem>& classified,
    const vector<TransferCandidate>& transferCandidates,
    const KnapsackResult& optimization,
    const vector<Cluster>& clusters) const {
    vector<ActionEntry> actions;
    vector<ActionEntry> transferActions =
        buildTransferRecommendations(classified, transferCandidates);
    set<int> transferItemIds;

    for (const auto& entry : transferActions) {
        transferItemIds.insert(entry.itemId);
        actions.push_back(entry);
    }

    for (const auto& item : optimization.selectedItems) {
        if (transferItemIds.count(item.itemId)) {
            continue;
        }

        ActionEntry entry;
        entry.type = "restock";
        entry.itemId = item.itemId;
        entry.itemName = item.name;
        entry.priority = item.priority;
        entry.message = "Restock " + item.name + " within budget allocation.";
        actions.push_back(entry);
    }

    for (const auto& item : misplacedItems) {
        ActionEntry entry;
        entry.type = "move";
        entry.itemId = item.id;
        entry.itemName = item.name;
        entry.priority = item.riskLevel;
        entry.message = "Move " + item.name + " from " + item.currentLocation +
                        " to " + item.expectedLocation + ".";
        actions.push_back(entry);
    }

    for (const auto& item : classified) {
        if (item.riskLevel != "HIGH") continue;

        ActionEntry entry;
        entry.type = "audit";
        entry.itemId = item.item.id;
        entry.itemName = item.item.name;
        entry.priority = "HIGH";
        entry.message = "Audit " + item.item.name + " for repeated shrink indicators.";
        actions.push_back(entry);
    }

    for (const auto& cluster : clusters) {
        if (cluster.label == "stable cluster") continue;

        ActionEntry entry;
        entry.type = "cluster";
        entry.itemId = cluster.clusterId;
        entry.itemName = "Cluster " + to_string(cluster.clusterId);
        entry.priority = cluster.severity;
        entry.message = "Review " + cluster.label + " with " +
                        to_string(cluster.itemIds.size()) + " linked items.";
        actions.push_back(entry);
    }

    return actions;
}

vector<string> InventoryAnalysisService::buildAlerts(const AnalysisSnapshot& snapshot) const {
    vector<string> alerts;

    if (snapshot.summary.totalItems == 0) {
        alerts.push_back("Dataset is empty. Add items to begin analysis.");
        return alerts;
    }

    if (snapshot.mismatches.empty()) {
        alerts.push_back("No stock loss detected in the current dataset.");
    } else {
        alerts.push_back(to_string(snapshot.mismatches.size()) + " items have stock mismatches.");
    }

    if (snapshot.misplacedItems.empty()) {
        alerts.push_back("All items are currently stored in their expected locations.");
    } else {
        alerts.push_back(to_string(snapshot.misplacedItems.size()) + " items need location correction.");
    }

    if (snapshot.optimization.selectedItems.empty()) {
        alerts.push_back(snapshot.optimization.status);
    } else {
        alerts.push_back("Restock plan covers " +
                         to_string(snapshot.optimization.selectedItems.size()) +
                         " high-value items.");
    }

    bool suspiciousCluster = false;
    for (const auto& cluster : snapshot.clusters) {
        if (cluster.label != "stable cluster") {
            suspiciousCluster = true;
            break;
        }
    }

    if (suspiciousCluster) {
        alerts.push_back("Suspicious graph clusters were identified for follow-up.");
    } else {
        alerts.push_back("No suspicious graph clusters were detected.");
    }

    return alerts;
}

SummaryReport InventoryAnalysisService::buildSummary(
    const ItemList& items,
    const vector<Item>& misplacedItems,
    const vector<ClassifiedItem>& classified,
    double totalLoss,
    const vector<ActionEntry>& actions) const {
    SummaryReport summary;
    summary.totalItems = static_cast<int>(items.size());
    summary.lostItems = 0;
    summary.misplacedItems = static_cast<int>(misplacedItems.size());
    summary.highRiskItems = 0;
    summary.estimatedFinancialLoss = totalLoss;

    for (const auto& item : items) {
        if (item.isLost()) {
            summary.lostItems++;
        }
    }

    for (const auto& item : classified) {
        if (item.riskLevel == "HIGH") {
            summary.highRiskItems++;
        }
    }

    set<string> uniqueRecommendations;
    for (const auto& action : actions) {
        uniqueRecommendations.insert(action.message);
        if (uniqueRecommendations.size() ==
            static_cast<size_t>(ConfigService::instance().data().summaryRecommendationLimit)) {
            break;
        }
    }

    for (const auto& recommendation : uniqueRecommendations) {
        summary.recommendedActions.push_back(recommendation);
    }

    if (summary.recommendedActions.empty()) {
        summary.recommendedActions.push_back(
            "Continue routine audits and monitor normal inventory flow.");
    }

    return summary;
}

AnalysisSnapshot InventoryAnalysisService::analyze(
    ItemList& items,
    const AuditHistory& history,
    int budget) const {
    const auto startedAt = chrono::steady_clock::now();
    optimizationEngine_.refreshFrequencies(items, history);

    HashAlgo hashAlgo = optimizationEngine_.createHashAnalyzer(items);
    hashAlgo.trackFrequency();
    hashAlgo.detectMismatches();

    vector<RankedItem> topRisk = optimizationEngine_.extractTopRiskItems(items);

    GreedyAlgo greedyAlgo = optimizationEngine_.createGreedyAnalyzer(items);
    greedyAlgo.classify();

    DPAlgo dpAlgo = optimizationEngine_.createDynamicProgrammingAnalyzer(items, budget);
    KnapsackResult dpResult = dpAlgo.solve();

    WarehouseRelationshipAnalyzer relationshipAnalyzer =
        optimizationEngine_.createWarehouseRelationshipAnalyzer(items);
    vector<Cluster> clusters = relationshipAnalyzer.findWarehouseClusters();
    vector<TransferCandidate> transferCandidates = relationshipAnalyzer.findTransferCandidates();

    AnalysisSnapshot snapshot;
    snapshot.mismatches = hashAlgo.getMismatches();
    snapshot.topRiskItems = topRisk;
    snapshot.classification = greedyAlgo.getClassified();
    snapshot.optimization = dpResult;
    snapshot.clusters = clusters;
    snapshot.theftTiming = optimizationEngine_.detectTheftTiming(history);
    snapshot.misplacedItems = getMisplacedItems(items);
    snapshot.totalLoss = hashAlgo.getTotalLoss();
    snapshot.actionReport = buildActionReport(snapshot.misplacedItems,
                                              snapshot.classification,
                                              transferCandidates,
                                              snapshot.optimization,
                                              snapshot.clusters);
    snapshot.summary = buildSummary(items,
                                    snapshot.misplacedItems,
                                    snapshot.classification,
                                    snapshot.totalLoss,
                                    snapshot.actionReport);
    snapshot.alerts = buildAlerts(snapshot);
    MonitoringRegistry::recordAnalysisExecution(
        chrono::duration<double, milli>(chrono::steady_clock::now() - startedAt).count());
    return snapshot;
}
