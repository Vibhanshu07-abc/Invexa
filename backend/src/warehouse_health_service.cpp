#include "../include/warehouse_health_service.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

using namespace std;

namespace {
ItemList itemsForWarehouse(const ItemList& items, int warehouseId) {
    ItemList warehouseItems;
    for (const auto& item : items) {
        if (item.warehouseId == warehouseId) {
            warehouseItems.push_back(item);
        }
    }
    return warehouseItems;
}
}

vector<WarehouseHealthScore> WarehouseHealthService::calculateHealthScores(
    const ItemList& items,
    const WarehouseList& warehouses,
    const map<int, double>& supplierPerformanceOverrides) const {
    map<int, Warehouse> warehouseIndex;

    for (const auto& warehouse : warehouses) {
        warehouseIndex[warehouse.id] = warehouse;
    }

    for (const auto& item : items) {
        if (!warehouseIndex.count(item.warehouseId)) {
            warehouseIndex[item.warehouseId] =
                Warehouse(item.warehouseId, item.warehouseName);
        }
    }

    vector<WarehouseHealthScore> scores;
    for (const auto& entry : warehouseIndex) {
        scores.push_back(calculateHealthScore(entry.first,
                                              items,
                                              warehouses,
                                              supplierPerformanceOverrides));
    }

    sort(scores.begin(), scores.end(),
        [](const WarehouseHealthScore& a, const WarehouseHealthScore& b) {
            if (a.healthScore == b.healthScore) {
                return a.warehouse.id < b.warehouse.id;
            }
            return a.healthScore > b.healthScore;
        });

    return scores;
}

WarehouseHealthScore WarehouseHealthService::calculateHealthScore(
    int warehouseId,
    const ItemList& items,
    const WarehouseList& warehouses,
    const map<int, double>& supplierPerformanceOverrides) const {
    WarehouseHealthScore score{};
    score.warehouse = resolveWarehouse(warehouseId, items, warehouses);

    ItemList warehouseItems = itemsForWarehouse(items, warehouseId);
    score.totalItems = static_cast<int>(warehouseItems.size());
    score.inventoryAccuracy = calculateInventoryAccuracy(warehouseItems);
    score.averageRisk = calculateAverageRisk(warehouseItems);
    score.capacityUtilization = calculateCapacityUtilization(warehouseItems);
    score.supplierPerformance = calculateSupplierPerformance(warehouseItems,
                                                             supplierPerformanceOverrides,
                                                             warehouseId);

    const double accuracyComponent = score.inventoryAccuracy * 0.35;
    const double riskComponent = (100.0 - score.averageRisk) * 0.30;
    const double utilizationBalance = max(0.0, 100.0 - abs(score.capacityUtilization - 85.0) * 2.0);
    const double utilizationComponent = utilizationBalance * 0.20;
    const double supplierComponent = score.supplierPerformance * 0.15;

    score.healthScore = roundTo(clampPercentage(accuracyComponent +
                                                riskComponent +
                                                utilizationComponent +
                                                supplierComponent));
    score.status = healthStatus(score.healthScore);
    return score;
}

double WarehouseHealthService::clampPercentage(double value) {
    return max(0.0, min(value, 100.0));
}

double WarehouseHealthService::roundTo(double value) {
    return round(value * 100.0) / 100.0;
}

string WarehouseHealthService::healthStatus(double score) {
    if (score >= 80.0) {
        return "HEALTHY";
    }
    if (score >= 60.0) {
        return "WATCH";
    }
    return "CRITICAL";
}

Warehouse WarehouseHealthService::resolveWarehouse(int warehouseId,
                                                   const ItemList& items,
                                                   const WarehouseList& warehouses) {
    for (const auto& warehouse : warehouses) {
        if (warehouse.id == warehouseId) {
            return warehouse;
        }
    }

    for (const auto& item : items) {
        if (item.warehouseId == warehouseId) {
            return item.getWarehouseOwnership();
        }
    }

    return Warehouse(warehouseId, "Warehouse " + to_string(warehouseId));
}

double WarehouseHealthService::calculateInventoryAccuracy(const ItemList& warehouseItems) {
    if (warehouseItems.empty()) {
        return 0.0;
    }

    double totalAccuracy = 0.0;
    for (const auto& item : warehouseItems) {
        if (item.expected <= 0) {
            totalAccuracy += item.actual <= 0 ? 100.0 : 0.0;
            continue;
        }

        const double variance = min(abs((double)item.expected - item.actual) /
                                        max(1, item.expected),
                                    1.0);
        totalAccuracy += (1.0 - variance) * 100.0;
    }

    return roundTo(totalAccuracy / warehouseItems.size());
}

double WarehouseHealthService::calculateAverageRisk(const ItemList& warehouseItems) {
    if (warehouseItems.empty()) {
        return 100.0;
    }

    double totalRisk = 0.0;
    for (const auto& item : warehouseItems) {
        totalRisk += item.riskScore;
    }

    return roundTo(clampPercentage(totalRisk / warehouseItems.size()));
}

double WarehouseHealthService::calculateCapacityUtilization(const ItemList& warehouseItems) {
    if (warehouseItems.empty()) {
        return 0.0;
    }

    double expectedUnits = 0.0;
    double actualUnits = 0.0;
    for (const auto& item : warehouseItems) {
        expectedUnits += max(item.expected, 0);
        actualUnits += max(item.actual, 0);
    }

    if (expectedUnits <= 0.0) {
        return actualUnits <= 0.0 ? 0.0 : 100.0;
    }

    return roundTo(clampPercentage((actualUnits / expectedUnits) * 100.0));
}

double WarehouseHealthService::calculateSupplierPerformance(
    const ItemList& warehouseItems,
    const map<int, double>& supplierPerformanceOverrides,
    int warehouseId) {
    const auto overrideIt = supplierPerformanceOverrides.find(warehouseId);
    if (overrideIt != supplierPerformanceOverrides.end()) {
        return roundTo(clampPercentage(overrideIt->second));
    }

    if (warehouseItems.empty()) {
        return 0.0;
    }

    double totalScore = 0.0;
    for (const auto& item : warehouseItems) {
        const double fulfillmentScore = item.expected <= 0
            ? (item.actual <= 0 ? 100.0 : 0.0)
            : clampPercentage((static_cast<double>(max(item.actual, 0)) / max(1, item.expected)) * 100.0);
        const double stabilityScore =
            100.0 - min(item.frequency * 12.5, 100.0);
        const double riskAdjustedScore =
            100.0 - clampPercentage(item.riskScore);

        totalScore += fulfillmentScore * 0.50 +
                      stabilityScore * 0.25 +
                      riskAdjustedScore * 0.25;
    }

    return roundTo(clampPercentage(totalScore / warehouseItems.size()));
}
