#pragma once
#ifndef ITEM_H
#define ITEM_H

#include "config_service.h"

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct Warehouse {
    int id;
    std::string name;
    std::string code;
    std::string location;

    Warehouse()
        : id(1), name("Main Warehouse"), code("WH-001"), location("Primary Hub") {}

    Warehouse(int id_,
              const std::string& name_,
              const std::string& code_ = "WH-001",
              const std::string& location_ = "Primary Hub")
        : id(id_), name(name_), code(code_), location(location_) {}
};

struct Item {
    int id;
    std::string name;
    std::string category;
    int warehouseId;
    std::string warehouseName;
    Warehouse warehouseOwner;
    int expected;
    int actual;
    double price;
    int demand;
    std::string expectedLocation;
    std::string currentLocation;
    int mismatch;
    double riskScore;
    std::string riskLevel;
    int frequency;
    int restockCost;
    int profitValue;

    Item()
        : id(0), name(""), category(""), warehouseId(1), warehouseName("Main Warehouse"),
          warehouseOwner(1, "Main Warehouse"),
          expected(0), actual(0), price(0.0), demand(0), expectedLocation(""),
          currentLocation(""), mismatch(0), riskScore(0.0), riskLevel("LOW"),
          frequency(0), restockCost(0), profitValue(0) {}

    Item(int id_,
         const std::string& name_,
         const std::string& category_,
         int expected_,
         int actual_,
         double price_,
         int demand_,
         const std::string& expectedLocation_,
         const std::string& currentLocation_,
         int frequency_ = 0,
         int warehouseId_ = 1,
         const std::string& warehouseName_ = "Main Warehouse")
        : id(id_), name(name_), category(category_), warehouseId(warehouseId_),
          warehouseName(warehouseName_), warehouseOwner(warehouseId_, warehouseName_),
          expected(expected_), actual(actual_), price(price_), demand(demand_),
          expectedLocation(expectedLocation_),
          currentLocation(currentLocation_), mismatch(0), riskScore(0.0),
          riskLevel("LOW"), frequency(frequency_), restockCost(0),
          profitValue(0)
    {
        refreshDerived();
    }

    void refreshDerived() {
        mismatch = expected - actual;
        restockCost = computeRestockCost();
        riskScore = computeRiskScore();
        riskLevel = classifyRisk();
        profitValue = computeProfitValue();
    }

    bool isLost() const {
        return mismatch > 0;
    }

    bool isMisplaced() const {
        return !expectedLocation.empty() &&
               !currentLocation.empty() &&
               expectedLocation != currentLocation;
    }

    double financialLoss() const {
        return std::max(mismatch, 0) * price;
    }

    int shortageUnits() const {
        return std::max(mismatch, 0);
    }

    int computeRestockCost() const {
        if (mismatch <= 0) return 0;
        const auto& config = ConfigService::instance().data();
        double base = shortageUnits() * price;
        double demandBuffer = demand * config.restockDemandBufferMultiplier;
        return std::max(1, (int)std::round(base + demandBuffer));
    }

    double computeRiskScore() const {
        const auto& config = ConfigService::instance().data();
        double stockImpact = 0.0;
        if (expected > 0) {
            stockImpact = std::min(std::abs((double)mismatch) / expected, 1.0) * config.stockImpactWeight;
        } else if (actual > 0) {
            stockImpact = config.noExpectedStockImpact;
        }

        double lossImpact = std::min(financialLoss() / config.riskLossDivisor, 1.0) * config.riskLossWeight;
        double demandImpact = std::min((double)demand / config.riskDemandDivisor, 1.0) * config.riskDemandWeight;
        double historyImpact = std::min((double)frequency / config.riskHistoryDivisor, 1.0) * config.riskHistoryWeight;
        double misplacedImpact = isMisplaced() ? config.riskMisplacedWeight : 0.0;
        double volatilityImpact = mismatch > 0
            ? std::min((double)mismatch / config.positiveVolatilityDivisor, 1.0) * config.positiveVolatilityWeight
            : std::min((double)std::abs(mismatch) / config.negativeVolatilityDivisor, 1.0) * config.negativeVolatilityWeight;

        return std::min(stockImpact + lossImpact + demandImpact +
                        historyImpact + misplacedImpact + volatilityImpact, 100.0);
    }

    std::string classifyRisk() const {
        const auto& config = ConfigService::instance().data();
        if (riskScore >= config.itemHighRiskScore ||
            mismatch >= config.itemHighMismatch ||
            financialLoss() >= config.itemHighLoss) return "HIGH";
        if (riskScore >= config.itemMediumRiskScore ||
            mismatch >= config.itemMediumMismatch ||
            financialLoss() >= config.itemMediumLoss ||
            isMisplaced()) return "MEDIUM";
        return "LOW";
    }

    int computeProfitValue() const {
        if (mismatch <= 0) return 0;
        const auto& config = ConfigService::instance().data();
        double value = financialLoss() * config.profitLossWeight +
                       riskScore * config.profitRiskWeight +
                       demand * config.profitDemandWeight;
        if (isMisplaced()) value += config.profitMisplacedBonus;
        value += frequency * config.profitFrequencyWeight;
        return std::max(1, (int)std::round(value));
    }

    bool operator<(const Item& o) const { return riskScore < o.riskScore; }
    bool operator>(const Item& o) const { return riskScore > o.riskScore; }

    void setWarehouseOwnership(const Warehouse& warehouse) {
        warehouseOwner = warehouse;
        warehouseId = warehouse.id;
        warehouseName = warehouse.name;
    }

    const Warehouse& getWarehouseOwnership() const {
        return warehouseOwner;
    }

    void syncWarehouseOwnership() {
        warehouseOwner.id = warehouseId;
        warehouseOwner.name = warehouseName;
    }
};

using ItemList = std::vector<Item>;
using WarehouseList = std::vector<Warehouse>;

#endif
