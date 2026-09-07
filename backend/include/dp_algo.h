#pragma once
#ifndef DP_ALGO_H
#define DP_ALGO_H

#include "item.h"
#include <vector>
#include <string>

struct KnapsackItem {
    int itemId;
    std::string name;
    std::string category;
    int weight;
    int value;
    double price;
    double loss;
    int demand;
    int shortage;
    int restockCost;
    double riskScore;
    std::string riskLevel;
    std::string priority;
    bool selected;
};

struct KnapsackResult {
    std::vector<KnapsackItem> selectedItems;
    int totalWeight;
    int totalValue;
    double totalLoss;
    int budget;
    double efficiency;
    std::string status;
};

class DPAlgo {
public:
    explicit DPAlgo(const ItemList& items, int budget);

    KnapsackResult solve();
    void printDPTable() const;
    void printResult() const;

private:
    const ItemList* items_;
    int budget_;
    std::vector<KnapsackItem> knapsackItems_;
    std::vector<std::vector<int>> dpTable_;
    KnapsackResult result_;
    bool solved_;

    void buildKnapsackItems();
    void runKnapsack();
    void backtrack();
    std::string priorityLabel(const KnapsackItem& item) const;
    void log(const std::string& msg) const;
};

#endif
