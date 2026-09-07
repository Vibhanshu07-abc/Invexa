#include "../include/monitoring_registry.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

using namespace std;
using nlohmann::json;

namespace {

struct MonitoringState {
    string updatedAt;
    double analysisExecutionMs = 0.0;
    double decisionEngineExecutionMs = 0.0;
    double forecastExecutionMs = 0.0;
    size_t decisionEngineRecommendationCount = 0;
    size_t forecastCount = 0;
    size_t inventoryCount = 0;
    size_t warehouseCount = 0;
    unsigned long long decisionEngineFailuresTotal = 0;
    unsigned long long forecastFailuresTotal = 0;
};

MonitoringState& state() {
    static MonitoringState monitoringState;
    return monitoringState;
}

string timestampNowUtc() {
    const auto now = chrono::system_clock::now();
    const time_t currentTime = chrono::system_clock::to_time_t(now);
    tm utcTime{};
#ifdef _WIN32
    gmtime_s(&utcTime, &currentTime);
#else
    gmtime_r(&currentTime, &utcTime);
#endif

    ostringstream output;
    output << put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

double sanitizeDuration(double durationMs) {
    return max(0.0, durationMs);
}

}  // namespace

void MonitoringRegistry::recordInventorySnapshot(const ItemList& items) {
    auto& current = state();
    current.inventoryCount = items.size();

    set<int> warehouseIds;
    for (const auto& item : items) {
        warehouseIds.insert(item.warehouseId);
    }

    current.warehouseCount = warehouseIds.size();
    current.updatedAt = timestampNowUtc();
}

void MonitoringRegistry::recordAnalysisExecution(double durationMs) {
    auto& current = state();
    current.analysisExecutionMs = sanitizeDuration(durationMs);
    current.updatedAt = timestampNowUtc();
}

void MonitoringRegistry::recordDecisionEngineExecution(double durationMs,
                                                       bool success,
                                                       size_t recommendationCount) {
    auto& current = state();
    current.decisionEngineExecutionMs = sanitizeDuration(durationMs);
    current.decisionEngineRecommendationCount = recommendationCount;
    if (!success) {
        current.decisionEngineFailuresTotal += 1;
    }
    current.updatedAt = timestampNowUtc();
}

void MonitoringRegistry::recordForecastExecution(double durationMs,
                                                 bool success,
                                                 size_t forecastCount) {
    auto& current = state();
    current.forecastExecutionMs = sanitizeDuration(durationMs);
    current.forecastCount = forecastCount;
    if (!success) {
        current.forecastFailuresTotal += 1;
    }
    current.updatedAt = timestampNowUtc();
}

json MonitoringRegistry::snapshot() {
    const auto& current = state();
    return {
        {"updatedAt", current.updatedAt},
        {"inventoryCount", current.inventoryCount},
        {"warehouseCount", current.warehouseCount},
        {"analysisExecutionMs", current.analysisExecutionMs},
        {"decisionEngineExecutionMs", current.decisionEngineExecutionMs},
        {"decisionEngineRecommendationCount", current.decisionEngineRecommendationCount},
        {"decisionEngineFailuresTotal", current.decisionEngineFailuresTotal},
        {"forecastExecutionMs", current.forecastExecutionMs},
        {"forecastCount", current.forecastCount},
        {"forecastFailuresTotal", current.forecastFailuresTotal}
    };
}
