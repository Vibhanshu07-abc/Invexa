#include "../include/rest_api_controllers.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test_support;

TEST(ApiTests, InventoryAndWarehouseControllersReturnStructuredPayloads) {
    FakeStorageRepository repository;
    InventoryQueryService queryService(repository);
    WarehouseApplicationService warehouseService;
    InventoryController inventoryController(queryService);
    WarehouseController warehouseController(queryService, warehouseService);

    WarehouseList warehouses = sampleWarehouses();
    ItemList items = sampleItems();

    const auto inventoryResponse = inventoryController.getInventory();
    const auto warehouseResponse = warehouseController.getWarehouses();
    const auto assignResponse = warehouseController.assignInventory(items, warehouses, 1, 2);

    EXPECT_EQ(inventoryResponse.statusCode, 200);
    EXPECT_EQ(warehouseResponse.statusCode, 200);
    EXPECT_EQ(assignResponse.statusCode, 200);
    EXPECT_EQ(assignResponse.data["warehouseId"].as_int64(), 2);
    EXPECT_EQ(items.front().warehouseId, 2);
}

TEST(ApiTests, AnalyticsAndDashboardControllersExposeDerivedData) {
    FakeStorageRepository repository;
    InventoryQueryService queryService(repository);
    AnalyticsApplicationService analyticsService(3);
    DashboardController dashboardController(queryService, analyticsService);
    AnalyticsController analyticsController(queryService, analyticsService);

    const auto dashboardResponse =
        dashboardController.getDashboard("2026-08-05T12:00:00", "Demo", 1500);
    const auto analyticsResponse = analyticsController.getAnalytics(1500);
    const auto healthResponse = analyticsController.getWarehouseHealth();

    EXPECT_EQ(dashboardResponse.statusCode, 200);
    EXPECT_EQ(analyticsResponse.statusCode, 200);
    EXPECT_EQ(healthResponse.statusCode, 200);
    EXPECT_FALSE(dashboardResponse.data["items"].empty());
    EXPECT_FALSE(analyticsResponse.data["mismatches"].empty());
    EXPECT_FALSE(healthResponse.data["warehouses"].empty());
}

TEST(ApiTests, DecisionAndReportsControllersReturnRecommendationPayloads) {
    FakeStorageRepository repository;
    InventoryQueryService queryService(repository);
    AnalyticsApplicationService analyticsService(3);
    DecisionCenterController decisionController(queryService, analyticsService);
    ReportsController reportsController(queryService, analyticsService);

    const auto decisionResponse = decisionController.getDecisionCenter(1500);
    const auto copilotResponse =
        decisionController.askAssistant("Which warehouse needs attention?", 1500);
    const auto reportsResponse = reportsController.getReports(1500);

    EXPECT_EQ(decisionResponse.statusCode, 200);
    EXPECT_EQ(copilotResponse.statusCode, 200);
    EXPECT_EQ(reportsResponse.statusCode, 200);
    EXPECT_GT(decisionResponse.data["recommendationCount"].as_int64(), 0);
    EXPECT_EQ(copilotResponse.data["status"].as_string(), "answered");
    EXPECT_TRUE(reportsResponse.data.contains("summary"));
}
