#pragma once

#ifndef REST_API_CONTROLLERS_H
#define REST_API_CONTROLLERS_H

#include "api_application_services.h"

#include <nlohmann/json.hpp>
#include <string>

struct ApiResponse {
    int statusCode;
    std::string message;
    nlohmann::json data;
};

class InventoryController {
public:
    explicit InventoryController(InventoryQueryService& inventoryQueryService);

    ApiResponse getInventory() const;

private:
    InventoryQueryService& inventoryQueryService_;
    ReportApplicationService reportApplicationService_;
};

class WarehouseController {
public:
    WarehouseController(InventoryQueryService& inventoryQueryService,
                        WarehouseApplicationService& warehouseApplicationService);

    ApiResponse getWarehouses() const;
    ApiResponse createWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const;
    ApiResponse updateWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const;
    ApiResponse deleteWarehouse(WarehouseList& warehouses, ItemList& items, int warehouseId) const;
    ApiResponse assignInventory(ItemList& items,
                                const WarehouseList& warehouses,
                                int itemId,
                                int warehouseId) const;

private:
    InventoryQueryService& inventoryQueryService_;
    WarehouseApplicationService& warehouseApplicationService_;
    ReportApplicationService reportApplicationService_;
};

class DashboardController {
public:
    DashboardController(InventoryQueryService& inventoryQueryService,
                        AnalyticsApplicationService& analyticsApplicationService);

    ApiResponse getDashboard(const std::string& generatedAt,
                             const std::string& mode,
                             int budget) const;

private:
    InventoryQueryService& inventoryQueryService_;
    AnalyticsApplicationService& analyticsApplicationService_;
    DashboardApplicationService dashboardApplicationService_;
};

class AnalyticsController {
public:
    AnalyticsController(InventoryQueryService& inventoryQueryService,
                        AnalyticsApplicationService& analyticsApplicationService);

    ApiResponse getAnalytics(int budget) const;
    ApiResponse getWarehouseHealth() const;

private:
    InventoryQueryService& inventoryQueryService_;
    AnalyticsApplicationService& analyticsApplicationService_;
    ReportApplicationService reportApplicationService_;
};

class DecisionCenterController {
public:
    DecisionCenterController(InventoryQueryService& inventoryQueryService,
                             AnalyticsApplicationService& analyticsApplicationService);

    ApiResponse getDecisionCenter(int budget) const;
    ApiResponse askAssistant(const std::string& question, int budget) const;

private:
    InventoryQueryService& inventoryQueryService_;
    AnalyticsApplicationService& analyticsApplicationService_;
    ReportApplicationService reportApplicationService_;
};

class ReportsController {
public:
    ReportsController(InventoryQueryService& inventoryQueryService,
                      AnalyticsApplicationService& analyticsApplicationService);

    ApiResponse getReports(int budget) const;

private:
    InventoryQueryService& inventoryQueryService_;
    AnalyticsApplicationService& analyticsApplicationService_;
    ReportApplicationService reportApplicationService_;
};

class AuthenticationController {
public:
    explicit AuthenticationController(AuthenticationService& authenticationService);

    ApiResponse login(const std::string& username, const std::string& password) const;

private:
    AuthenticationService& authenticationService_;
};

#endif
