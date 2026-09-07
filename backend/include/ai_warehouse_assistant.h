#pragma once

#ifndef AI_WAREHOUSE_ASSISTANT_H
#define AI_WAREHOUSE_ASSISTANT_H

#include "decision_engine.h"
#include "gemini_service.h"
#include "warehouse_health_service.h"

#include <map>
#include <string>
#include <vector>

struct AssistantAnswer {
    std::string question;
    std::string answer;
    std::vector<std::string> evidence;
    std::string sourceOfTruth;
    std::string status;
};

class AIWarehouseAssistant {
public:
    AssistantAnswer answerQuestion(const std::string& question,
                                   const ItemList& items,
                                   const AnalysisSnapshot& snapshot,
                                   const WarehouseList& warehouses = WarehouseList{},
                                   const std::map<int, double>& supplierPerformanceOverrides = {}) const;

private:
    WarehouseHealthService warehouseHealthService_;
    DecisionEngine decisionEngine_;
    GeminiService geminiService_;

    AssistantAnswer answerWarehouseAttention(const std::string& question,
                                             const ItemList& items,
                                             const AnalysisSnapshot& snapshot,
                                             const WarehouseList& warehouses,
                                             const std::map<int, double>& supplierPerformanceOverrides) const;
    AssistantAnswer answerLowHealthReason(const std::string& question,
                                          const ItemList& items,
                                          const AnalysisSnapshot& snapshot,
                                          const WarehouseList& warehouses,
                                          const std::map<int, double>& supplierPerformanceOverrides) const;
    AssistantAnswer answerRestockProducts(const std::string& question,
                                          const AnalysisSnapshot& snapshot) const;
    AssistantAnswer answerDemandChanges(const std::string& question,
                                        const ItemList& items,
                                        const AnalysisSnapshot& snapshot) const;
    static std::string normalize(const std::string& value);
};

#endif
