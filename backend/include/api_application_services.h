#pragma once

#ifndef API_APPLICATION_SERVICES_H
#define API_APPLICATION_SERVICES_H

#include "ai_warehouse_assistant.h"
#include "dashboard_serializer.h"
#include "decision_engine.h"
#include "gemini_service.h"
#include "inventory_analysis_service.h"
#include "login.h"
#include "report_serializer.h"
#include "storage_repository.h"
#include "warehouse_health_service.h"
#include "warehouse_service.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class InventoryQueryService {
public:
    explicit InventoryQueryService(StorageRepository& repository);

    ItemList loadInventory() const;
    AuditHistory loadAuditHistory() const;
    std::vector<UserAccount> loadUsers() const;
    WarehouseList deriveWarehouses(const ItemList& items) const;

private:
    StorageRepository& repository_;
};

class AuthenticationService {
public:
    explicit AuthenticationService(InventoryQueryService& inventoryQueryService);

    UserSession authenticate(const std::string& username,
                             const std::string& password) const;

private:
    InventoryQueryService& inventoryQueryService_;
};

class WarehouseApplicationService {
public:
    bool createWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const;
    bool updateWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const;
    bool deleteWarehouse(WarehouseList& warehouses, const ItemList& items, int warehouseId) const;
    bool assignInventoryToWarehouse(ItemList& items,
                                    const WarehouseList& warehouses,
                                    int itemId,
                                    int warehouseId) const;
    bool assignInventoryToWarehouse(ItemList& items,
                                    const WarehouseList& warehouses,
                                    const std::vector<int>& itemIds,
                                    int warehouseId) const;

private:
    WarehouseService warehouseService_;
};

class AnalyticsApplicationService {
public:
    explicit AnalyticsApplicationService(int topK = 5);

    AnalysisSnapshot analyze(ItemList& items, const AuditHistory& history, int budget) const;
    std::vector<BusinessRecommendation> buildRecommendations(const AnalysisSnapshot& snapshot) const;
    GeminiInsightBundle buildAIInsights(const ItemList& items,
                                        const AnalysisSnapshot& snapshot,
                                        const std::vector<BusinessRecommendation>& recommendations) const;
    std::vector<WarehouseHealthScore> buildWarehouseHealth(const ItemList& items,
                                                           const WarehouseList& warehouses = WarehouseList{},
                                                           const std::map<int, double>& supplierPerformanceOverrides = {}) const;
    AssistantAnswer askAssistant(const std::string& question,
                                 const ItemList& items,
                                 const AnalysisSnapshot& snapshot,
                                 const WarehouseList& warehouses = WarehouseList{},
                                 const std::map<int, double>& supplierPerformanceOverrides = {}) const;

private:
    InventoryAnalysisService inventoryAnalysisService_;
    DecisionEngine decisionEngine_;
    GeminiService geminiService_;
    WarehouseHealthService warehouseHealthService_;
    AIWarehouseAssistant aiWarehouseAssistant_;
};

class DashboardApplicationService {
public:
    nlohmann::json buildDashboard(const std::string& generatedAt,
                                  const std::string& mode,
                                  int budget,
                                  const ItemList& items,
                                  const AuditHistory& history,
                                  const AnalysisSnapshot& snapshot) const;

private:
    DashboardSerializer dashboardSerializer_;
};

class ReportApplicationService {
public:
    nlohmann::json buildWarehouseReport(const ItemList& items) const;
    nlohmann::json buildInventoryReport(const ItemList& items) const;
    nlohmann::json buildAnalyticsReport(const AnalysisSnapshot& snapshot) const;
    nlohmann::json buildDecisionCenterReport(const std::vector<BusinessRecommendation>& recommendations,
                                             const GeminiInsightBundle& insights) const;
    nlohmann::json buildSummaryReport(const SummaryReport& summary,
                                      const std::vector<std::string>& alerts) const;

private:
    ReportSerializer reportSerializer_;
};

#endif
