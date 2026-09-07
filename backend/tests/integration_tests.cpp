#include "../include/api_application_services.h"
#include "../include/rest_api_controllers.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test_support;

TEST(IntegrationTests, RepositoryToAnalyticsToDashboardFlowProducesConsistentOutputs) {
    FakeStorageRepository repository;
    InventoryQueryService queryService(repository);
    AnalyticsApplicationService analyticsService(3);
    DashboardController dashboardController(queryService, analyticsService);
    ReportsController reportsController(queryService, analyticsService);

    ItemList items = queryService.loadInventory();
    const auto history = queryService.loadAuditHistory();
    const auto snapshot = analyticsService.analyze(items, history, 1500);
    const auto dashboardResponse =
        dashboardController.getDashboard("2026-08-05T12:30:00", "Demo", 1500);
    const auto reportsResponse = reportsController.getReports(1500);

    EXPECT_EQ(snapshot.summary.totalItems, static_cast<int>(items.size()));
    EXPECT_EQ(dashboardResponse.statusCode, 200);
    EXPECT_EQ(reportsResponse.statusCode, 200);
    EXPECT_EQ(dashboardResponse.data["summary"]["totalItems"].as_int64(), static_cast<long long>(items.size()));
    EXPECT_EQ(reportsResponse.data["summary"]["summary"]["totalItems"].as_int64(),
              static_cast<long long>(items.size()));
}

TEST(IntegrationTests, RepositoryAuthenticationAndDecisionFlowWorksEndToEnd) {
    FakeStorageRepository repository;
    InventoryQueryService queryService(repository);
    AuthenticationService authenticationService(queryService);
    AuthenticationController authenticationController(authenticationService);
    AnalyticsApplicationService analyticsService(3);
    DecisionCenterController decisionController(queryService, analyticsService);

    const auto authResponse = authenticationController.login("admin", "admin-secret");
    const auto decisionResponse = decisionController.getDecisionCenter(1500);
    const auto assistantResponse =
        decisionController.askAssistant("What products should be restocked?", 1500);

    EXPECT_EQ(authResponse.statusCode, 200);
    EXPECT_EQ(decisionResponse.statusCode, 200);
    EXPECT_EQ(assistantResponse.statusCode, 200);
    EXPECT_TRUE(authResponse.data["authenticated"].as_bool());
    EXPECT_GT(decisionResponse.data["recommendationCount"].as_int64(), 0);
    EXPECT_EQ(assistantResponse.data["status"].as_string(), "answered");
}
