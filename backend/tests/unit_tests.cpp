#include "../include/monitoring_registry.h"
#include "../include/password_hasher.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test_support;

TEST(UnitTests, PasswordHasherHashesAndVerifiesPasswords) {
    const std::string password = "inventory-secret";
    const std::string hash = PasswordHasher::hashPassword(password);

    EXPECT_FALSE(hash.empty());
    EXPECT_TRUE(PasswordHasher::verifyPassword(password, hash));
    EXPECT_FALSE(PasswordHasher::verifyPassword("wrong-password", hash));
}

TEST(UnitTests, ItemRefreshDerivedCalculatesRiskAndProfitFields) {
    Item item = makeItem(10, "Switch", "Electronics", 120, 80, 15.0, 50, "Zone-A", "Zone-B", 2);

    EXPECT_EQ(item.mismatch, 40);
    EXPECT_TRUE(item.isLost());
    EXPECT_TRUE(item.isMisplaced());
    EXPECT_GT(item.financialLoss(), 0.0);
    EXPECT_GT(item.riskScore, 0.0);
    EXPECT_FALSE(item.riskLevel.empty());
    EXPECT_GT(item.restockCost, 0);
    EXPECT_GT(item.profitValue, 0);
}

TEST(UnitTests, MonitoringRegistrySnapshotTracksRecordedValues) {
    const auto before = MonitoringRegistry::snapshot();

    MonitoringRegistry::recordInventorySnapshot(sampleItems());
    MonitoringRegistry::recordAnalysisExecution(12.5);
    MonitoringRegistry::recordDecisionEngineExecution(8.25, true, 4);
    MonitoringRegistry::recordForecastExecution(15.75, false, 3);

    const auto snapshot = MonitoringRegistry::snapshot();

    EXPECT_EQ(snapshot["inventoryCount"].as_int64(), 4);
    EXPECT_EQ(snapshot["warehouseCount"].as_int64(), 2);
    EXPECT_DOUBLE_EQ(snapshot["analysisExecutionMs"].as_double(), 12.5);
    EXPECT_DOUBLE_EQ(snapshot["decisionEngineExecutionMs"].as_double(), 8.25);
    EXPECT_EQ(snapshot["decisionEngineRecommendationCount"].as_int64(), 4);
    EXPECT_DOUBLE_EQ(snapshot["forecastExecutionMs"].as_double(), 15.75);
    EXPECT_EQ(snapshot["forecastCount"].as_int64(), 3);
    EXPECT_EQ(snapshot["forecastFailuresTotal"].as_int64(),
              before["forecastFailuresTotal"].as_int64() + 1);
}
