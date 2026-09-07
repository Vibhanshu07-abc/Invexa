#include "../include/report_serializer.h"

#include <algorithm>
#include <cmath>
#include <map>

using namespace std;
using nlohmann::json;

double ReportSerializer::roundTo(double value, int precision) const {
    const double scale = pow(10.0, precision);
    return round(value * scale) / scale;
}

json ReportSerializer::serializeWarehouses(const ItemList& items) const {
    map<int, Warehouse> warehouses;
    map<int, int> itemCounts;
    map<int, double> totalLossByWarehouse;

    for (const auto& item : items) {
        if (!warehouses.count(item.warehouseId)) {
            warehouses[item.warehouseId] = Warehouse(item.warehouseId, item.warehouseName);
        }
        itemCounts[item.warehouseId]++;
        totalLossByWarehouse[item.warehouseId] += item.financialLoss();
    }

    json payload = json::array();
    for (const auto& entry : warehouses) {
        const auto& warehouse = entry.second;
        payload.push_back({
            {"id", warehouse.id},
            {"name", warehouse.name},
            {"code", warehouse.code},
            {"location", warehouse.location},
            {"itemCount", itemCounts[warehouse.id]},
            {"totalLoss", roundTo(totalLossByWarehouse[warehouse.id])}
        });
    }

    return payload;
}

json ReportSerializer::serializeItems(const ItemList& items) const {
    json payload = json::array();

    for (const auto& item : items) {
        payload.push_back({
            {"id", item.id},
            {"name", item.name},
            {"category", item.category},
            {"warehouseId", item.warehouseId},
            {"warehouseName", item.warehouseName},
            {"expected", item.expected},
            {"actual", item.actual},
            {"price", roundTo(item.price)},
            {"demand", item.demand},
            {"expectedLocation", item.expectedLocation},
            {"currentLocation", item.currentLocation},
            {"mismatch", item.mismatch},
            {"riskScore", roundTo(item.riskScore)},
            {"riskLevel", item.riskLevel},
            {"frequency", item.frequency},
            {"restockCost", item.restockCost},
            {"profitValue", item.profitValue}
        });
    }

    return payload;
}

json ReportSerializer::serializeAudits(const AuditHistory& history) const {
    json payload = json::array();

    for (const auto& audit : history.getAudits()) {
        json auditItems = json::array();
        for (const auto& item : audit.items) {
            auditItems.push_back({
                {"id", item.id},
                {"actual", item.actual},
                {"mismatch", item.mismatch},
                {"currentLocation", item.currentLocation}
            });
        }

        payload.push_back({
            {"timestamp", audit.timestamp},
            {"items", auditItems}
        });
    }

    return payload;
}

json ReportSerializer::serializeMismatches(const vector<MismatchRecord>& mismatches) const {
    json payload = json::array();

    for (const auto& mismatch : mismatches) {
        payload.push_back({
            {"id", mismatch.itemId},
            {"name", mismatch.itemName},
            {"expected", mismatch.expected},
            {"actual", mismatch.actual},
            {"delta", mismatch.delta},
            {"price", roundTo(mismatch.price)},
            {"loss", roundTo(mismatch.loss)},
            {"frequency", mismatch.frequency},
            {"riskLevel", mismatch.riskLevel},
            {"action", mismatch.action}
        });
    }

    return payload;
}

json ReportSerializer::serializeMisplacedItems(const vector<Item>& misplacedItems) const {
    json payload = json::array();

    for (const auto& item : misplacedItems) {
        payload.push_back({
            {"id", item.id},
            {"name", item.name},
            {"warehouseId", item.warehouseId},
            {"warehouseName", item.warehouseName},
            {"currentLocation", item.currentLocation},
            {"correctLocation", item.expectedLocation},
            {"action", "Move item to expected location"},
            {"riskLevel", item.riskLevel}
        });
    }

    return payload;
}

json ReportSerializer::serializeTopRiskItems(const vector<RankedItem>& topRiskItems) const {
    json payload = json::array();

    for (const auto& rankedItem : topRiskItems) {
        payload.push_back({
            {"rank", rankedItem.rank},
            {"id", rankedItem.item.id},
            {"name", rankedItem.item.name},
            {"warehouseId", rankedItem.item.warehouseId},
            {"warehouseName", rankedItem.item.warehouseName},
            {"riskScore", roundTo(rankedItem.item.riskScore)},
            {"expected", rankedItem.item.expected},
            {"actual", rankedItem.item.actual},
            {"mismatch", rankedItem.item.mismatch},
            {"loss", roundTo(rankedItem.item.financialLoss())},
            {"riskLevel", rankedItem.item.riskLevel},
            {"badge", rankedItem.badge}
        });
    }

    return payload;
}

json ReportSerializer::serializeClassification(const vector<ClassifiedItem>& classified) const {
    json payload = json::array();

    for (const auto& item : classified) {
        payload.push_back({
            {"id", item.item.id},
            {"name", item.item.name},
            {"warehouseId", item.item.warehouseId},
            {"warehouseName", item.item.warehouseName},
            {"riskLevel", item.riskLevel},
            {"riskScore", roundTo(item.item.riskScore)},
            {"urgency", roundTo(item.urgencyScore)},
            {"mismatch", item.item.mismatch},
            {"loss", roundTo(item.item.financialLoss())},
            {"explanation", item.explanation}
        });
    }

    return payload;
}

json ReportSerializer::serializeOptimization(const KnapsackResult& result) const {
    json selectedItems = json::array();

    for (const auto& item : result.selectedItems) {
        selectedItems.push_back({
            {"id", item.itemId},
            {"name", item.name},
            {"category", item.category},
            {"restockCost", item.restockCost},
            {"profitValue", item.value},
            {"shortage", item.shortage},
            {"loss", roundTo(item.loss)},
            {"riskLevel", item.riskLevel},
            {"priority", item.priority}
        });
    }

    return {
        {"budget", result.budget},
        {"budgetUsed", result.totalWeight},
        {"totalValue", result.totalValue},
        {"totalLoss", roundTo(result.totalLoss)},
        {"efficiency", roundTo(result.efficiency)},
        {"status", result.status},
        {"selectedItems", selectedItems}
    };
}

json ReportSerializer::serializeClusters(const vector<Cluster>& clusters) const {
    json payload = json::array();

    for (const auto& cluster : clusters) {
        payload.push_back({
            {"clusterId", cluster.clusterId},
            {"label", cluster.label},
            {"severity", cluster.severity},
            {"avgRisk", roundTo(cluster.avgRisk)},
            {"size", cluster.itemIds.size()},
            {"description", cluster.description},
            {"itemIds", cluster.itemIds}
        });
    }

    return payload;
}

json ReportSerializer::serializeTheftTiming(const vector<TheftTimingRecord>& timing) const {
    json payload = json::array();

    for (const auto& record : timing) {
        payload.push_back({
            {"id", record.itemId},
            {"name", record.itemName},
            {"previousAudit", record.previousAudit},
            {"mismatchAudit", record.mismatchAudit},
            {"message", record.message}
        });
    }

    return payload;
}

json ReportSerializer::serializeActionReport(const vector<ActionEntry>& actions) const {
    json payload = json::array();

    for (const auto& action : actions) {
        payload.push_back({
            {"type", action.type},
            {"id", action.itemId},
            {"name", action.itemName},
            {"priority", action.priority},
            {"message", action.message}
        });
    }

    return payload;
}

json ReportSerializer::serializeBusinessRecommendations(
    const vector<BusinessRecommendation>& recommendations) const {
    json payload = json::array();

    for (const auto& recommendation : recommendations) {
        payload.push_back({
            {"category", recommendation.category},
            {"priority", recommendation.priority},
            {"title", recommendation.title},
            {"rationale", recommendation.rationale},
            {"relatedItemIds", recommendation.relatedItemIds},
            {"actions", recommendation.actions}
        });
    }

    return payload;
}

json ReportSerializer::serializeGeminiInsights(const GeminiInsightBundle& insights) const {
    json explanations = json::array();
    for (const auto& explanation : insights.recommendationExplanations) {
        explanations.push_back({
            {"category", explanation.category},
            {"title", explanation.title},
            {"explanation", explanation.explanation},
            {"status", explanation.status}
        });
    }

    json forecasts = json::array();
    for (const auto& forecast : insights.demandForecasts) {
        forecasts.push_back({
            {"id", forecast.itemId},
            {"name", forecast.itemName},
            {"currentDemand", forecast.currentDemand},
            {"projectedDemand", forecast.projectedDemand},
            {"confidence", forecast.confidence},
            {"explanation", forecast.explanation},
            {"status", forecast.status}
        });
    }

    return {
        {"provider", insights.provider},
        {"enabled", insights.enabled},
        {"sourceOfTruth", insights.sourceOfTruth},
        {"recommendationExplanations", explanations},
        {"executiveSummary", {
            {"summary", insights.executiveSummary.summary},
            {"status", insights.executiveSummary.status}
        }},
        {"demandForecasts", forecasts}
    };
}

json ReportSerializer::serializeSummary(const SummaryReport& summary) const {
    return {
        {"totalItems", summary.totalItems},
        {"lostItems", summary.lostItems},
        {"misplacedItems", summary.misplacedItems},
        {"highRiskItems", summary.highRiskItems},
        {"estimatedFinancialLoss", roundTo(summary.estimatedFinancialLoss)},
        {"recommendedActions", summary.recommendedActions}
    };
}

json ReportSerializer::serializeAlerts(const vector<string>& alerts) const {
    return json(alerts);
}
