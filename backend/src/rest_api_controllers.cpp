#include "../include/rest_api_controllers.h"

#include "../include/config_service.h"

using namespace std;
using nlohmann::json;

InventoryController::InventoryController(InventoryQueryService& inventoryQueryService)
    : inventoryQueryService_(inventoryQueryService) {}

ApiResponse InventoryController::getInventory() const {
    const ItemList items = inventoryQueryService_.loadInventory();
    return {200, "Inventory loaded.", reportApplicationService_.buildInventoryReport(items)};
}

WarehouseController::WarehouseController(InventoryQueryService& inventoryQueryService,
                                         WarehouseApplicationService& warehouseApplicationService)
    : inventoryQueryService_(inventoryQueryService),
      warehouseApplicationService_(warehouseApplicationService) {}

ApiResponse WarehouseController::getWarehouses() const {
    const ItemList items = inventoryQueryService_.loadInventory();
    return {200, "Warehouses loaded.", reportApplicationService_.buildWarehouseReport(items)};
}

ApiResponse WarehouseController::createWarehouse(WarehouseList& warehouses,
                                                 const Warehouse& warehouse) const {
    const bool created = warehouseApplicationService_.createWarehouse(warehouses, warehouse);
    return {
        created ? 201 : 400,
        created ? "Warehouse created." : "Warehouse could not be created.",
        {{"warehouses", warehouses.size()}}
    };
}

ApiResponse WarehouseController::updateWarehouse(WarehouseList& warehouses,
                                                 const Warehouse& warehouse) const {
    const bool updated = warehouseApplicationService_.updateWarehouse(warehouses, warehouse);
    return {
        updated ? 200 : 404,
        updated ? "Warehouse updated." : "Warehouse could not be updated.",
        {{"warehouseId", warehouse.id}}
    };
}

ApiResponse WarehouseController::deleteWarehouse(WarehouseList& warehouses,
                                                 ItemList& items,
                                                 int warehouseId) const {
    const bool removed = warehouseApplicationService_.deleteWarehouse(warehouses, items, warehouseId);
    return {
        removed ? 200 : 409,
        removed ? "Warehouse deleted." : "Warehouse could not be deleted.",
        {{"warehouseId", warehouseId}}
    };
}

ApiResponse WarehouseController::assignInventory(ItemList& items,
                                                 const WarehouseList& warehouses,
                                                 int itemId,
                                                 int warehouseId) const {
    const bool assigned =
        warehouseApplicationService_.assignInventoryToWarehouse(items, warehouses, itemId, warehouseId);
    return {
        assigned ? 200 : 404,
        assigned ? "Inventory assigned to warehouse." : "Inventory assignment failed.",
        {{"itemId", itemId}, {"warehouseId", warehouseId}}
    };
}

DashboardController::DashboardController(InventoryQueryService& inventoryQueryService,
                                         AnalyticsApplicationService& analyticsApplicationService)
    : inventoryQueryService_(inventoryQueryService),
      analyticsApplicationService_(analyticsApplicationService) {}

ApiResponse DashboardController::getDashboard(const string& generatedAt,
                                              const string& mode,
                                              int budget) const {
    ItemList items = inventoryQueryService_.loadInventory();
    const AuditHistory history = inventoryQueryService_.loadAuditHistory();
    const AnalysisSnapshot snapshot = analyticsApplicationService_.analyze(items, history, budget);

    return {
        200,
        "Dashboard payload generated.",
        dashboardApplicationService_.buildDashboard(generatedAt, mode, budget, items, history, snapshot)
    };
}

AnalyticsController::AnalyticsController(InventoryQueryService& inventoryQueryService,
                                         AnalyticsApplicationService& analyticsApplicationService)
    : inventoryQueryService_(inventoryQueryService),
      analyticsApplicationService_(analyticsApplicationService) {}

ApiResponse AnalyticsController::getAnalytics(int budget) const {
    ItemList items = inventoryQueryService_.loadInventory();
    const AuditHistory history = inventoryQueryService_.loadAuditHistory();
    const AnalysisSnapshot snapshot = analyticsApplicationService_.analyze(items, history, budget);

    return {200, "Analytics report generated.", reportApplicationService_.buildAnalyticsReport(snapshot)};
}

ApiResponse AnalyticsController::getWarehouseHealth() const {
    const ItemList items = inventoryQueryService_.loadInventory();
    const WarehouseList warehouses = inventoryQueryService_.deriveWarehouses(items);
    const auto scores = analyticsApplicationService_.buildWarehouseHealth(items, warehouses);

    json payload = json::array();
    for (const auto& score : scores) {
        payload.push_back({
            {"warehouseId", score.warehouse.id},
            {"warehouseName", score.warehouse.name},
            {"healthScore", score.healthScore},
            {"status", score.status},
            {"inventoryAccuracy", score.inventoryAccuracy},
            {"averageRisk", score.averageRisk},
            {"capacityUtilization", score.capacityUtilization},
            {"supplierPerformance", score.supplierPerformance}
        });
    }

    return {200, "Warehouse health calculated.", {{"warehouses", payload}}};
}

DecisionCenterController::DecisionCenterController(InventoryQueryService& inventoryQueryService,
                                                   AnalyticsApplicationService& analyticsApplicationService)
    : inventoryQueryService_(inventoryQueryService),
      analyticsApplicationService_(analyticsApplicationService) {}

ApiResponse DecisionCenterController::getDecisionCenter(int budget) const {
    ItemList items = inventoryQueryService_.loadInventory();
    const AuditHistory history = inventoryQueryService_.loadAuditHistory();
    const AnalysisSnapshot snapshot = analyticsApplicationService_.analyze(items, history, budget);
    const auto recommendations = analyticsApplicationService_.buildRecommendations(snapshot);
    const auto insights = analyticsApplicationService_.buildAIInsights(items, snapshot, recommendations);

    return {
        200,
        "Decision Center payload generated.",
        reportApplicationService_.buildDecisionCenterReport(recommendations, insights)
    };
}

ApiResponse DecisionCenterController::askAssistant(const string& question, int budget) const {
    ItemList items = inventoryQueryService_.loadInventory();
    const AuditHistory history = inventoryQueryService_.loadAuditHistory();
    const AnalysisSnapshot snapshot = analyticsApplicationService_.analyze(items, history, budget);
    const WarehouseList warehouses = inventoryQueryService_.deriveWarehouses(items);
    const AssistantAnswer answer =
        analyticsApplicationService_.askAssistant(question, items, snapshot, warehouses);

    return {
        answer.status == "unsupported" ? 400 : 200,
        "Assistant response generated.",
        {
            {"question", answer.question},
            {"answer", answer.answer},
            {"evidence", answer.evidence},
            {"sourceOfTruth", answer.sourceOfTruth},
            {"status", answer.status}
        }
    };
}

ReportsController::ReportsController(InventoryQueryService& inventoryQueryService,
                                     AnalyticsApplicationService& analyticsApplicationService)
    : inventoryQueryService_(inventoryQueryService),
      analyticsApplicationService_(analyticsApplicationService) {}

ApiResponse ReportsController::getReports(int budget) const {
    ItemList items = inventoryQueryService_.loadInventory();
    const AuditHistory history = inventoryQueryService_.loadAuditHistory();
    const AnalysisSnapshot snapshot = analyticsApplicationService_.analyze(items, history, budget);
    const auto recommendations = analyticsApplicationService_.buildRecommendations(snapshot);
    const auto insights = analyticsApplicationService_.buildAIInsights(items, snapshot, recommendations);

    return {
        200,
        "Reports payload generated.",
        {
            {"inventory", reportApplicationService_.buildInventoryReport(items)},
            {"analytics", reportApplicationService_.buildAnalyticsReport(snapshot)},
            {"decisionCenter", reportApplicationService_.buildDecisionCenterReport(recommendations, insights)},
            {"summary", reportApplicationService_.buildSummaryReport(snapshot.summary, snapshot.alerts)}
        }
    };
}

AuthenticationController::AuthenticationController(AuthenticationService& authenticationService)
    : authenticationService_(authenticationService) {}

ApiResponse AuthenticationController::login(const string& username, const string& password) const {
    const UserSession session = authenticationService_.authenticate(username, password);
    return {
        session.authenticated ? 200 : 401,
        session.authenticated ? "Authentication successful." : "Authentication failed.",
        {
            {"username", session.username},
            {"role", session.role},
            {"authenticated", session.authenticated}
        }
    };
}
