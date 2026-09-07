#include "../include/ai_warehouse_assistant.h"

#include <algorithm>
#include <cctype>
#include <sstream>

using namespace std;

AssistantAnswer AIWarehouseAssistant::answerQuestion(
    const string& question,
    const ItemList& items,
    const AnalysisSnapshot& snapshot,
    const WarehouseList& warehouses,
    const map<int, double>& supplierPerformanceOverrides) const {
    const string normalizedQuestion = normalize(question);

    if (normalizedQuestion == "which warehouse needs attention?") {
        return answerWarehouseAttention(question,
                                        items,
                                        snapshot,
                                        warehouses,
                                        supplierPerformanceOverrides);
    }

    if (normalizedQuestion == "why is warehouse health low?") {
        return answerLowHealthReason(question,
                                     items,
                                     snapshot,
                                     warehouses,
                                     supplierPerformanceOverrides);
    }

    if (normalizedQuestion == "what products should be restocked?") {
        return answerRestockProducts(question, snapshot);
    }

    if (normalizedQuestion == "what demand changes are expected?") {
        return answerDemandChanges(question, items, snapshot);
    }

    AssistantAnswer answer;
    answer.question = question;
    answer.answer =
        "Supported questions are: Which warehouse needs attention?, Why is warehouse health low?, "
        "What products should be restocked?, and What demand changes are expected?";
    answer.sourceOfTruth = "Existing warehouse intelligence services";
    answer.status = "unsupported";
    return answer;
}

AssistantAnswer AIWarehouseAssistant::answerWarehouseAttention(
    const string& question,
    const ItemList& items,
    const AnalysisSnapshot& snapshot,
    const WarehouseList& warehouses,
    const map<int, double>& supplierPerformanceOverrides) const {
    AssistantAnswer answer;
    answer.question = question;
    answer.sourceOfTruth = "WarehouseHealthService plus deterministic analysis outputs";

    vector<WarehouseHealthScore> scores =
        warehouseHealthService_.calculateHealthScores(items, warehouses, supplierPerformanceOverrides);

    if (scores.empty()) {
        answer.answer = "No warehouse health data is available yet.";
        answer.status = "no-data";
        return answer;
    }

    const auto weakest = min_element(scores.begin(), scores.end(),
        [](const WarehouseHealthScore& a, const WarehouseHealthScore& b) {
            if (a.healthScore == b.healthScore) {
                return a.warehouse.id < b.warehouse.id;
            }
            return a.healthScore < b.healthScore;
        });

    ostringstream response;
    response << weakest->warehouse.name << " needs the most attention with a health score of "
             << weakest->healthScore << " and status " << weakest->status << ".";
    answer.answer = response.str();
    answer.evidence.push_back("Inventory accuracy: " + to_string(static_cast<int>(weakest->inventoryAccuracy)) + "%");
    answer.evidence.push_back("Average risk: " + to_string(static_cast<int>(weakest->averageRisk)));
    answer.evidence.push_back("Capacity utilization: " + to_string(static_cast<int>(weakest->capacityUtilization)) + "%");
    answer.evidence.push_back("Supplier performance: " + to_string(static_cast<int>(weakest->supplierPerformance)) + "%");
    answer.evidence.push_back("High-risk items in system: " + to_string(snapshot.summary.highRiskItems));
    answer.status = "answered";
    return answer;
}

AssistantAnswer AIWarehouseAssistant::answerLowHealthReason(
    const string& question,
    const ItemList& items,
    const AnalysisSnapshot& snapshot,
    const WarehouseList& warehouses,
    const map<int, double>& supplierPerformanceOverrides) const {
    AssistantAnswer answer;
    answer.question = question;
    answer.sourceOfTruth = "WarehouseHealthService and DecisionEngine";

    vector<WarehouseHealthScore> scores =
        warehouseHealthService_.calculateHealthScores(items, warehouses, supplierPerformanceOverrides);

    if (scores.empty()) {
        answer.answer = "Warehouse health cannot be explained because no warehouse data is available.";
        answer.status = "no-data";
        return answer;
    }

    const auto weakest = min_element(scores.begin(), scores.end(),
        [](const WarehouseHealthScore& a, const WarehouseHealthScore& b) {
            return a.healthScore < b.healthScore;
        });

    vector<string> reasons;
    if (weakest->inventoryAccuracy < 85.0) {
        reasons.push_back("inventory accuracy is below target");
        answer.evidence.push_back("Inventory accuracy is " + to_string(static_cast<int>(weakest->inventoryAccuracy)) + "%");
    }
    if (weakest->averageRisk > 45.0) {
        reasons.push_back("average risk remains elevated");
        answer.evidence.push_back("Average risk is " + to_string(static_cast<int>(weakest->averageRisk)));
    }
    if (weakest->capacityUtilization < 65.0 || weakest->capacityUtilization > 95.0) {
        reasons.push_back("capacity utilization is unbalanced");
        answer.evidence.push_back("Capacity utilization is " + to_string(static_cast<int>(weakest->capacityUtilization)) + "%");
    }
    if (weakest->supplierPerformance < 75.0) {
        reasons.push_back("supplier performance is weakening support");
        answer.evidence.push_back("Supplier performance is " + to_string(static_cast<int>(weakest->supplierPerformance)) + "%");
    }

    const auto recommendations = decisionEngine_.generateRecommendations(snapshot);
    for (const auto& recommendation : recommendations) {
        if (recommendation.category == "inventory-control" ||
            recommendation.category == "warehouse-network") {
            answer.evidence.push_back(recommendation.title);
        }
    }

    if (reasons.empty()) {
        reasons.push_back("multiple metrics are trending below healthy range");
    }

    ostringstream response;
    response << weakest->warehouse.name << " has low health mainly because ";
    for (size_t index = 0; index < reasons.size(); ++index) {
        response << reasons[index];
        if (index + 2 == reasons.size()) {
            response << " and ";
        } else if (index + 1 < reasons.size()) {
            response << ", ";
        }
    }
    response << ".";

    answer.answer = response.str();
    answer.status = "answered";
    return answer;
}

AssistantAnswer AIWarehouseAssistant::answerRestockProducts(
    const string& question,
    const AnalysisSnapshot& snapshot) const {
    AssistantAnswer answer;
    answer.question = question;
    answer.sourceOfTruth = "Dynamic Programming optimization and DecisionEngine";

    if (snapshot.optimization.selectedItems.empty()) {
        answer.answer = "No products are currently selected for deterministic restock optimization.";
        answer.status = "no-data";
        return answer;
    }

    ostringstream response;
    response << "The current restock plan prioritizes ";
    for (size_t index = 0; index < snapshot.optimization.selectedItems.size(); ++index) {
        const auto& item = snapshot.optimization.selectedItems[index];
        if (index > 0) {
            response << ", ";
        }
        response << item.name;
        if (index == 2 || index + 1 == snapshot.optimization.selectedItems.size()) {
            break;
        }
    }
    response << ".";
    answer.answer = response.str();

    for (const auto& item : snapshot.optimization.selectedItems) {
        if (answer.evidence.size() == 5) {
            break;
        }
        answer.evidence.push_back(
            item.name + " | priority " + item.priority +
            " | restock cost " + to_string(item.restockCost) +
            " | shortage " + to_string(item.shortage));
    }

    answer.status = "answered";
    return answer;
}

AssistantAnswer AIWarehouseAssistant::answerDemandChanges(
    const string& question,
    const ItemList& items,
    const AnalysisSnapshot& snapshot) const {
    AssistantAnswer answer;
    answer.question = question;
    answer.sourceOfTruth = "GeminiService demand forecast with deterministic fallback";

    const auto recommendations = decisionEngine_.generateRecommendations(snapshot);
    const GeminiInsightBundle insights =
        geminiService_.generateInsights(items, snapshot, recommendations);

    if (insights.demandForecasts.empty()) {
        answer.answer = "No demand forecast data is available.";
        answer.status = "no-data";
        return answer;
    }

    ostringstream response;
    response << "Expected demand changes are concentrated in ";
    for (size_t index = 0; index < insights.demandForecasts.size(); ++index) {
        const auto& forecast = insights.demandForecasts[index];
        if (index > 0) {
            response << ", ";
        }
        response << (forecast.itemName.empty() ? ("item " + to_string(forecast.itemId)) : forecast.itemName);
        if (index == 2 || index + 1 == insights.demandForecasts.size()) {
            break;
        }
    }
    response << ".";
    answer.answer = response.str();

    for (const auto& forecast : insights.demandForecasts) {
        if (answer.evidence.size() == 5) {
            break;
        }
        const string itemLabel = forecast.itemName.empty()
            ? ("Item " + to_string(forecast.itemId))
            : forecast.itemName;
        answer.evidence.push_back(
            itemLabel + " | projected demand " + to_string(forecast.projectedDemand) +
            " | confidence " + forecast.confidence +
            " | " + forecast.explanation);
    }

    answer.status = insights.enabled ? "answered-with-ai" : "answered-with-fallback";
    return answer;
}

string AIWarehouseAssistant::normalize(const string& value) {
    string normalized;
    normalized.reserve(value.size());

    for (char ch : value) {
        normalized.push_back(static_cast<char>(tolower(static_cast<unsigned char>(ch))));
    }

    return normalized;
}
