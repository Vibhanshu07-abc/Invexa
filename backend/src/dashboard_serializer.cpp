#include "../include/dashboard_serializer.h"
#include "../include/config_service.h"
#include "../include/decision_engine.h"
#include "../include/gemini_service.h"
#include "../include/monitoring_registry.h"

#include <chrono>

using nlohmann::json;

json DashboardSerializer::serialize(const std::string& generatedAt,
                                    const std::string& mode,
                                    int budget,
                                    const ItemList& items,
                                    const AuditHistory& history,
                                    const AnalysisSnapshot& snapshot) const {
    const auto& config = ConfigService::instance().data();
    DecisionEngine decisionEngine;
    const auto decisionStartedAt = std::chrono::steady_clock::now();
    std::vector<BusinessRecommendation> recommendations;
    try {
        recommendations = decisionEngine.generateRecommendations(snapshot);
        MonitoringRegistry::recordDecisionEngineExecution(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - decisionStartedAt).count(),
            true,
            recommendations.size());
    } catch (...) {
        MonitoringRegistry::recordDecisionEngineExecution(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - decisionStartedAt).count(),
            false,
            0);
        throw;
    }

    GeminiService geminiService;
    const auto forecastStartedAt = std::chrono::steady_clock::now();
    GeminiInsightBundle aiInsights;
    try {
        aiInsights = geminiService.generateInsights(items, snapshot, recommendations);
        MonitoringRegistry::recordForecastExecution(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - forecastStartedAt).count(),
            true,
            aiInsights.demandForecasts.size());
    } catch (...) {
        MonitoringRegistry::recordForecastExecution(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - forecastStartedAt).count(),
            false,
            0);
        throw;
    }

    return {
        {"generatedAt", generatedAt},
        {"mode", mode},
        {"budget", budget},
        {"warehouses", reportSerializer_.serializeWarehouses(items)},
        {"credentials", {
            {"employee", {
                {"username", config.employeeUser.username},
                {"password", ""}
            }},
            {"manager", {
                {"username", config.managerUser.username},
                {"password", ""}
            }}
        }},
        {"items", reportSerializer_.serializeItems(items)},
        {"audits", reportSerializer_.serializeAudits(history)},
        {"mismatches", reportSerializer_.serializeMismatches(snapshot.mismatches)},
        {"misplacedItems", reportSerializer_.serializeMisplacedItems(snapshot.misplacedItems)},
        {"topKItems", reportSerializer_.serializeTopRiskItems(snapshot.topRiskItems)},
        {"classification", reportSerializer_.serializeClassification(snapshot.classification)},
        {"optimization", reportSerializer_.serializeOptimization(snapshot.optimization)},
        {"clusters", reportSerializer_.serializeClusters(snapshot.clusters)},
        {"theftTiming", reportSerializer_.serializeTheftTiming(snapshot.theftTiming)},
        {"decisionCenter", {
            {"recommendationCount", recommendations.size()},
            {"recommendations", reportSerializer_.serializeBusinessRecommendations(recommendations)}
        }},
        {"ai", reportSerializer_.serializeGeminiInsights(aiInsights)},
        {"actionReport", reportSerializer_.serializeActionReport(snapshot.actionReport)},
        {"summary", reportSerializer_.serializeSummary(snapshot.summary)},
        {"alerts", reportSerializer_.serializeAlerts(snapshot.alerts)}
    };
}
