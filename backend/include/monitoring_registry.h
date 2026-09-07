#pragma once

#ifndef MONITORING_REGISTRY_H
#define MONITORING_REGISTRY_H

#include "item.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>

class MonitoringRegistry {
public:
    static void recordInventorySnapshot(const ItemList& items);
    static void recordAnalysisExecution(double durationMs);
    static void recordDecisionEngineExecution(double durationMs,
                                              bool success,
                                              std::size_t recommendationCount);
    static void recordForecastExecution(double durationMs,
                                        bool success,
                                        std::size_t forecastCount);
    static nlohmann::json snapshot();

private:
    MonitoringRegistry() = default;
};

#endif
