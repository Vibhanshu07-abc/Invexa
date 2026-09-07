#include "../include/api_application_services.h"

#include <map>

using namespace std;
using nlohmann::json;

InventoryQueryService::InventoryQueryService(StorageRepository& repository)
    : repository_(repository) {}

ItemList InventoryQueryService::loadInventory() const {
    return repository_.loadInventory();
}

AuditHistory InventoryQueryService::loadAuditHistory() const {
    AuditHistory history;
    for (const auto& audit : repository_.loadAudits()) {
        history.addAudit(audit.timestamp, audit.items);
    }
    return history;
}

vector<UserAccount> InventoryQueryService::loadUsers() const {
    return repository_.loadUsers();
}

WarehouseList InventoryQueryService::deriveWarehouses(const ItemList& items) const {
    map<int, Warehouse> warehouseIndex;
    for (const auto& item : items) {
        warehouseIndex[item.warehouseId] = item.getWarehouseOwnership();
    }

    WarehouseList warehouses;
    for (const auto& entry : warehouseIndex) {
        warehouses.push_back(entry.second);
    }
    return warehouses;
}

AuthenticationService::AuthenticationService(InventoryQueryService& inventoryQueryService)
    : inventoryQueryService_(inventoryQueryService) {}

UserSession AuthenticationService::authenticate(const string& username,
                                                const string& password) const {
    LoginSystem loginSystem(inventoryQueryService_.loadUsers());
    return loginSystem.authenticate(username, password);
}

bool WarehouseApplicationService::createWarehouse(WarehouseList& warehouses,
                                                  const Warehouse& warehouse) const {
    return warehouseService_.createWarehouse(warehouses, warehouse);
}

bool WarehouseApplicationService::updateWarehouse(WarehouseList& warehouses,
                                                  const Warehouse& warehouse) const {
    return warehouseService_.updateWarehouse(warehouses, warehouse);
}

bool WarehouseApplicationService::deleteWarehouse(WarehouseList& warehouses,
                                                  const ItemList& items,
                                                  int warehouseId) const {
    return warehouseService_.deleteWarehouse(warehouses, items, warehouseId);
}

bool WarehouseApplicationService::assignInventoryToWarehouse(ItemList& items,
                                                             const WarehouseList& warehouses,
                                                             int itemId,
                                                             int warehouseId) const {
    return warehouseService_.assignInventoryToWarehouse(items, warehouses, itemId, warehouseId);
}

bool WarehouseApplicationService::assignInventoryToWarehouse(ItemList& items,
                                                             const WarehouseList& warehouses,
                                                             const vector<int>& itemIds,
                                                             int warehouseId) const {
    return warehouseService_.assignInventoryToWarehouse(items, warehouses, itemIds, warehouseId);
}

AnalyticsApplicationService::AnalyticsApplicationService(int topK)
    : inventoryAnalysisService_(topK) {}

AnalysisSnapshot AnalyticsApplicationService::analyze(ItemList& items,
                                                      const AuditHistory& history,
                                                      int budget) const {
    return inventoryAnalysisService_.analyze(items, history, budget);
}

vector<BusinessRecommendation> AnalyticsApplicationService::buildRecommendations(
    const AnalysisSnapshot& snapshot) const {
    return decisionEngine_.generateRecommendations(snapshot);
}

GeminiInsightBundle AnalyticsApplicationService::buildAIInsights(
    const ItemList& items,
    const AnalysisSnapshot& snapshot,
    const vector<BusinessRecommendation>& recommendations) const {
    return geminiService_.generateInsights(items, snapshot, recommendations);
}

vector<WarehouseHealthScore> AnalyticsApplicationService::buildWarehouseHealth(
    const ItemList& items,
    const WarehouseList& warehouses,
    const map<int, double>& supplierPerformanceOverrides) const {
    return warehouseHealthService_.calculateHealthScores(items, warehouses, supplierPerformanceOverrides);
}

AssistantAnswer AnalyticsApplicationService::askAssistant(
    const string& question,
    const ItemList& items,
    const AnalysisSnapshot& snapshot,
    const WarehouseList& warehouses,
    const map<int, double>& supplierPerformanceOverrides) const {
    return aiWarehouseAssistant_.answerQuestion(question,
                                                items,
                                                snapshot,
                                                warehouses,
                                                supplierPerformanceOverrides);
}

json DashboardApplicationService::buildDashboard(const string& generatedAt,
                                                 const string& mode,
                                                 int budget,
                                                 const ItemList& items,
                                                 const AuditHistory& history,
                                                 const AnalysisSnapshot& snapshot) const {
    return dashboardSerializer_.serialize(generatedAt, mode, budget, items, history, snapshot);
}

json ReportApplicationService::buildWarehouseReport(const ItemList& items) const {
    return {
        {"warehouses", reportSerializer_.serializeWarehouses(items)}
    };
}

json ReportApplicationService::buildInventoryReport(const ItemList& items) const {
    return {
        {"warehouses", reportSerializer_.serializeWarehouses(items)},
        {"items", reportSerializer_.serializeItems(items)}
    };
}

json ReportApplicationService::buildAnalyticsReport(const AnalysisSnapshot& snapshot) const {
    return {
        {"mismatches", reportSerializer_.serializeMismatches(snapshot.mismatches)},
        {"misplacedItems", reportSerializer_.serializeMisplacedItems(snapshot.misplacedItems)},
        {"topRiskItems", reportSerializer_.serializeTopRiskItems(snapshot.topRiskItems)},
        {"classification", reportSerializer_.serializeClassification(snapshot.classification)},
        {"optimization", reportSerializer_.serializeOptimization(snapshot.optimization)},
        {"clusters", reportSerializer_.serializeClusters(snapshot.clusters)},
        {"theftTiming", reportSerializer_.serializeTheftTiming(snapshot.theftTiming)}
    };
}

json ReportApplicationService::buildDecisionCenterReport(
    const vector<BusinessRecommendation>& recommendations,
    const GeminiInsightBundle& insights) const {
    return {
        {"recommendationCount", static_cast<int>(recommendations.size())},
        {"recommendations", reportSerializer_.serializeBusinessRecommendations(recommendations)},
        {"ai", reportSerializer_.serializeGeminiInsights(insights)}
    };
}

json ReportApplicationService::buildSummaryReport(const SummaryReport& summary,
                                                  const vector<string>& alerts) const {
    return {
        {"summary", reportSerializer_.serializeSummary(summary)},
        {"alerts", reportSerializer_.serializeAlerts(alerts)}
    };
}
