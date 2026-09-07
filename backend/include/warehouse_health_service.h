#pragma once

#ifndef WAREHOUSE_HEALTH_SERVICE_H
#define WAREHOUSE_HEALTH_SERVICE_H

#include "item.h"

#include <map>
#include <string>
#include <vector>

struct WarehouseHealthScore {
    Warehouse warehouse;
    int totalItems;
    double inventoryAccuracy;
    double averageRisk;
    double capacityUtilization;
    double supplierPerformance;
    double healthScore;
    std::string status;
};

class WarehouseHealthService {
public:
    std::vector<WarehouseHealthScore> calculateHealthScores(
        const ItemList& items,
        const WarehouseList& warehouses = WarehouseList{},
        const std::map<int, double>& supplierPerformanceOverrides = {}) const;

    WarehouseHealthScore calculateHealthScore(
        int warehouseId,
        const ItemList& items,
        const WarehouseList& warehouses = WarehouseList{},
        const std::map<int, double>& supplierPerformanceOverrides = {}) const;

private:
    static double clampPercentage(double value);
    static double roundTo(double value);
    static std::string healthStatus(double score);
    static Warehouse resolveWarehouse(int warehouseId,
                                      const ItemList& items,
                                      const WarehouseList& warehouses);
    static double calculateInventoryAccuracy(const ItemList& warehouseItems);
    static double calculateAverageRisk(const ItemList& warehouseItems);
    static double calculateCapacityUtilization(const ItemList& warehouseItems);
    static double calculateSupplierPerformance(const ItemList& warehouseItems,
                                               const std::map<int, double>& supplierPerformanceOverrides,
                                               int warehouseId);
};

#endif
