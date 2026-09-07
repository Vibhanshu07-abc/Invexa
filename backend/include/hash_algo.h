#pragma once
#ifndef HASH_ALGO_H
#define HASH_ALGO_H

#include "item.h"
#include <unordered_map>
#include <string>
#include <vector>

struct MismatchRecord {
    int itemId;
    std::string itemName;
    int expected;
    int actual;
    int delta;
    double price;
    double loss;
    int frequency;
    std::string riskLevel;
    std::string action;
};

class HashAlgo {
public:
    explicit HashAlgo(const ItemList& items);

    void detectMismatches();
    void trackFrequency();
    void printReport() const;

    const std::vector<MismatchRecord>& getMismatches() const;
    const std::unordered_map<int, int>& getFrequencyMap() const;
    double getTotalLoss() const;
    int getMismatchCount() const;

private:
    const ItemList* items_;
    std::unordered_map<int, int> frequencyMap_;
    std::vector<MismatchRecord> mismatches_;
    double totalLoss_;

    void log(const std::string& msg) const;
    std::string severityLabel(const Item& item) const;
    std::string recommendedAction(const Item& item) const;
};

#endif
