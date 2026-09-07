#pragma once
#ifndef GREEDY_ALGO_H
#define GREEDY_ALGO_H

#include "config_service.h"
#include "item.h"
#include <vector>
#include <string>
#include <map>

struct ClassifiedItem {
    Item item;
    std::string riskLevel;
    std::string explanation;
    double urgencyScore;
};

struct ThresholdConfig {
    double highRiskScore;
    double mediumRiskScore;
    int highMismatch;
    int mediumMismatch;
    double highLossThresh;
    double mediumLossThresh;

    ThresholdConfig()
        : highRiskScore(ConfigService::instance().data().itemHighRiskScore),
          mediumRiskScore(ConfigService::instance().data().itemMediumRiskScore),
          highMismatch(ConfigService::instance().data().itemHighMismatch),
          mediumMismatch(ConfigService::instance().data().itemMediumMismatch),
          highLossThresh(ConfigService::instance().data().itemHighLoss),
          mediumLossThresh(ConfigService::instance().data().itemMediumLoss) {}
};

class GreedyAlgo {
public:
    explicit GreedyAlgo(const ItemList& items,
                        ThresholdConfig cfg = ThresholdConfig{});

    void classify();
    void printClassification() const;

    const std::vector<ClassifiedItem>& getClassified() const;
    std::vector<ClassifiedItem> getByRisk(const std::string& level) const;
    std::map<std::string, int> riskCounts() const;
    std::map<std::string, double> riskTotalLoss() const;

private:
    const ItemList* items_;
    ThresholdConfig cfg_;
    std::vector<ClassifiedItem> classified_;

    std::string determineRisk(const Item& it) const;
    std::string buildExplanation(const Item& it, const std::string& risk) const;
    double computeUrgency(const Item& it) const;
    void log(const std::string& msg) const;
};

#endif
