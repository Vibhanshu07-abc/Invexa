#include "../include/api_application_services.h"
#include "../include/inventory_analysis_service.h"
#include "../include/warehouse_health_service.h"
#include "../include/warehouse_service.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test_support;

TEST(ServiceTests, WarehouseServiceValidatesCreateUpdateDeleteAndBatchAssign) {
    WarehouseService service;
    WarehouseList warehouses = sampleWarehouses();
    ItemList items = sampleItems();

    EXPECT_FALSE(service.createWarehouse(warehouses, Warehouse(1, "Duplicate", "WH-N", "Delhi")));
    EXPECT_TRUE(service.createWarehouse(warehouses, Warehouse(3, "Central Hub", "WH-C", "Mumbai")));
    EXPECT_TRUE(service.updateWarehouse(warehouses, Warehouse(3, "Central Hub Updated", "WH-C1", "Pune")));
    EXPECT_TRUE(service.assignInventoryToWarehouse(items, warehouses, std::vector<int>{1, 2}, 3));
    EXPECT_EQ(items[0].warehouseId, 3);
    EXPECT_EQ(items[1].warehouseName, "Central Hub Updated");
    EXPECT_FALSE(service.deleteWarehouse(warehouses, items, 3));
}

TEST(ServiceTests, WarehouseHealthServiceBuildsSortedScores) {
    WarehouseHealthService service;
    const auto scores = service.calculateHealthScores(sampleItems(), sampleWarehouses());

    ASSERT_EQ(scores.size(), 2);
    EXPECT_GE(scores.front().healthScore, scores.back().healthScore);
    EXPECT_FALSE(scores.front().status.empty());
    EXPECT_GE(scores.front().inventoryAccuracy, 0.0);
}

TEST(ServiceTests, InventoryAnalysisServiceProducesActionableSnapshot) {
    ItemList items = sampleItems();
    const AuditHistory history = sampleAuditHistory();
    InventoryAnalysisService service(3);

    const auto snapshot = service.analyze(items, history, 1500);

    EXPECT_FALSE(snapshot.mismatches.empty());
    EXPECT_FALSE(snapshot.topRiskItems.empty());
    EXPECT_FALSE(snapshot.classification.empty());
    EXPECT_FALSE(snapshot.clusters.empty());
    EXPECT_FALSE(snapshot.theftTiming.empty());
    EXPECT_FALSE(snapshot.actionReport.empty());
    EXPECT_GT(snapshot.summary.totalItems, 0);
}

TEST(ServiceTests, AnalyticsApplicationServiceBuildsRecommendationsAndInsights) {
    ItemList items = sampleItems();
    const AuditHistory history = sampleAuditHistory();
    AnalyticsApplicationService service(3);

    const auto snapshot = service.analyze(items, history, 1500);
    const auto recommendations = service.buildRecommendations(snapshot);
    const auto insights = service.buildAIInsights(items, snapshot, recommendations);
    const auto health = service.buildWarehouseHealth(items, sampleWarehouses());

    EXPECT_FALSE(recommendations.empty());
    EXPECT_FALSE(insights.executiveSummary.summary.empty());
    EXPECT_FALSE(insights.demandForecasts.empty());
    EXPECT_EQ(insights.executiveSummary.status, "fallback");
    EXPECT_EQ(health.size(), 2);
}
