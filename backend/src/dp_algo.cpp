#include "../include/dp_algo.h"
#include "../include/config_service.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

DPAlgo::DPAlgo(const ItemList& items, int budget)
    : items_(&items), budget_(budget), solved_(false)
{
    result_.budget = budget_;
    result_.totalWeight = 0;
    result_.totalValue = 0;
    result_.totalLoss = 0.0;
    result_.efficiency = 0.0;
    result_.status = "Not solved";
}

void DPAlgo::log(const string& msg) const {
    cout << "[DP] " << msg << "\n";
}

string DPAlgo::priorityLabel(const KnapsackItem& item) const {
    const auto& config = ConfigService::instance().data();
    if (item.riskScore >= config.dpImmediateRiskScore || item.loss >= config.dpImmediateLoss) return "Immediate";
    if (item.riskScore >= config.dpHighRiskScore || item.loss >= config.dpHighLoss) return "High";
    return "Planned";
}

void DPAlgo::buildKnapsackItems() {
    knapsackItems_.clear();

    for (const auto& item : *items_) {
        if (!item.isLost()) continue;

        KnapsackItem ki;
        ki.itemId = item.id;
        ki.name = item.name;
        ki.category = item.category;
        ki.weight = item.restockCost;
        ki.value = item.profitValue;
        ki.price = item.price;
        ki.loss = item.financialLoss();
        ki.demand = item.demand;
        ki.shortage = item.shortageUnits();
        ki.restockCost = item.restockCost;
        ki.riskScore = item.riskScore;
        ki.riskLevel = item.riskLevel;
        ki.priority = "";
        ki.selected = false;
        knapsackItems_.push_back(ki);
    }

    sort(knapsackItems_.begin(), knapsackItems_.end(),
        [](const KnapsackItem& a, const KnapsackItem& b) {
            if (a.value == b.value) return a.loss > b.loss;
            return a.value > b.value;
        });
}

void DPAlgo::runKnapsack() {
    int n = (int)knapsackItems_.size();
    dpTable_.assign(n + 1, vector<int>(budget_ + 1, 0));

    for (int i = 1; i <= n; ++i) {
        for (int w = 0; w <= budget_; ++w) {
            dpTable_[i][w] = dpTable_[i - 1][w];
            if (knapsackItems_[i - 1].weight <= w) {
                dpTable_[i][w] = max(
                    dpTable_[i][w],
                    dpTable_[i - 1][w - knapsackItems_[i - 1].weight] + knapsackItems_[i - 1].value
                );
            }
        }
    }
}

void DPAlgo::backtrack() {
    result_.selectedItems.clear();
    result_.totalWeight = 0;
    result_.totalValue = 0;
    result_.totalLoss = 0.0;

    int w = budget_;
    for (int i = (int)knapsackItems_.size(); i >= 1; --i) {
        if (dpTable_[i][w] == dpTable_[i - 1][w]) continue;

        KnapsackItem chosen = knapsackItems_[i - 1];
        chosen.selected = true;
        chosen.priority = priorityLabel(chosen);
        result_.selectedItems.push_back(chosen);
        result_.totalWeight += chosen.weight;
        result_.totalValue += chosen.value;
        result_.totalLoss += chosen.loss;
        w -= chosen.weight;
    }

    sort(result_.selectedItems.begin(), result_.selectedItems.end(),
        [](const KnapsackItem& a, const KnapsackItem& b) {
            if (a.value == b.value) return a.riskScore > b.riskScore;
            return a.value > b.value;
        });

    result_.efficiency = result_.totalWeight == 0
        ? 0.0
        : (double)result_.totalValue / result_.totalWeight;
}

KnapsackResult DPAlgo::solve() {
    result_.budget = budget_;
    buildKnapsackItems();

    if (knapsackItems_.empty()) {
        result_.status = "No lost items available for restock optimization.";
        solved_ = true;
        return result_;
    }

    int minCost = knapsackItems_[0].weight;
    for (const auto& item : knapsackItems_) minCost = min(minCost, item.weight);
    if (budget_ < minCost) {
        result_.status = "Budget is too small to restock any lost item.";
        solved_ = true;
        return result_;
    }

    runKnapsack();
    backtrack();
    result_.status = result_.selectedItems.empty()
        ? "No feasible restock plan found."
        : "Optimal restock plan generated.";
    solved_ = true;
    return result_;
}

void DPAlgo::printDPTable() const {
    cout << "\n====== DP SNAPSHOT ======\n";
    if (!solved_ || dpTable_.empty()) {
        cout << result_.status << "\n";
        cout << "=========================\n\n";
        return;
    }

    const auto& config = ConfigService::instance().data();
    int rows = min((int)dpTable_.size(), config.dpPreviewRows);
    int cols = min(budget_ + 1, config.dpPreviewCols);
    for (int i = 0; i < rows; ++i) {
        for (int w = 0; w < cols; ++w) {
            cout << setw(6) << dpTable_[i][w];
        }
        cout << "\n";
    }
    cout << "=========================\n\n";
}

void DPAlgo::printResult() const {
    cout << "\n====== RESTOCK PLAN ======\n";
    cout << result_.status << "\n";
    if (result_.selectedItems.empty()) {
        cout << "No items selected.\n";
        cout << "==========================\n\n";
        return;
    }

    cout << left
         << setw(6) << "ID"
         << setw(18) << "Name"
         << setw(10) << "Cost"
         << setw(10) << "Value"
         << setw(12) << "Loss($)"
         << "Priority\n";
    cout << string(64, '-') << "\n";

    for (const auto& item : result_.selectedItems) {
        cout << left
             << setw(6) << item.itemId
             << setw(18) << item.name.substr(0, 17)
             << setw(10) << item.weight
             << setw(10) << item.value
             << setw(12) << fixed << setprecision(2) << item.loss
             << item.priority << "\n";
    }

    cout << "Budget used: " << result_.totalWeight << " / " << result_.budget << "\n";
    cout << "==========================\n\n";
}
