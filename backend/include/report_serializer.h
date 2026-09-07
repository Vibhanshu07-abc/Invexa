#pragma once

#ifndef REPORT_SERIALIZER_H
#define REPORT_SERIALIZER_H

#include "audit_history.h"
#include "decision_engine.h"
#include "gemini_service.h"
#include "inventory_analysis_service.h"
#include "item.h"

#include <nlohmann/json.hpp>

class ReportSerializer {
public:
    nlohmann::json serializeWarehouses(const ItemList& items) const;
    nlohmann::json serializeItems(const ItemList& items) const;
    nlohmann::json serializeAudits(const AuditHistory& history) const;
    nlohmann::json serializeMismatches(const std::vector<MismatchRecord>& mismatches) const;
    nlohmann::json serializeMisplacedItems(const std::vector<Item>& misplacedItems) const;
    nlohmann::json serializeTopRiskItems(const std::vector<RankedItem>& topRiskItems) const;
    nlohmann::json serializeClassification(const std::vector<ClassifiedItem>& classified) const;
    nlohmann::json serializeOptimization(const KnapsackResult& result) const;
    nlohmann::json serializeClusters(const std::vector<Cluster>& clusters) const;
    nlohmann::json serializeTheftTiming(const std::vector<TheftTimingRecord>& timing) const;
    nlohmann::json serializeActionReport(const std::vector<ActionEntry>& actions) const;
    nlohmann::json serializeBusinessRecommendations(
        const std::vector<BusinessRecommendation>& recommendations) const;
    nlohmann::json serializeGeminiInsights(const GeminiInsightBundle& insights) const;
    nlohmann::json serializeSummary(const SummaryReport& summary) const;
    nlohmann::json serializeAlerts(const std::vector<std::string>& alerts) const;

private:
    double roundTo(double value, int precision = 2) const;
};

#endif
