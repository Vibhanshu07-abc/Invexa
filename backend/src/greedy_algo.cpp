#include "../include/greedy_algo.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace std;

GreedyAlgo::GreedyAlgo(const ItemList& items, ThresholdConfig cfg)
    : items_(&items), cfg_(cfg)
{
    log("Initialized with " + to_string(items_->size()) + " items.");
}

void GreedyAlgo::log(const string& msg) const {
    cout << "[GREEDY] " << msg << "\n";
}

string GreedyAlgo::determineRisk(const Item& it) const {
    if (it.riskScore >= cfg_.highRiskScore || it.mismatch >= cfg_.highMismatch || it.financialLoss() >= cfg_.highLossThresh) return "HIGH";
    if (it.riskScore >= cfg_.mediumRiskScore || it.mismatch >= cfg_.mediumMismatch || it.financialLoss() >= cfg_.mediumLossThresh || it.isMisplaced()) return "MEDIUM";
    return "LOW";
}

string GreedyAlgo::buildExplanation(const Item& it, const string& risk) const {
    ostringstream oss;
    if (risk == "HIGH") {
        oss << "High loss exposure with urgent stock gap";
        if (it.isMisplaced()) oss << " and location inconsistency";
    } else if (risk == "MEDIUM") {
        oss << "Needs review due to moderate stock or placement anomaly";
    } else {
        oss << "Operating within acceptable thresholds";
    }
    return oss.str();
}

double GreedyAlgo::computeUrgency(const Item& it) const {
    const auto& config = ConfigService::instance().data();
    double lossWeight = min(it.financialLoss() / config.greedyLossUrgencyDivisor, 1.0) * config.greedyLossUrgencyWeight;
    double riskWeight = (it.riskScore / 100.0) * config.greedyRiskUrgencyWeight;
    double demandWeight = min((double)it.demand / config.greedyDemandUrgencyDivisor, 1.0) * config.greedyDemandUrgencyWeight;
    double freqWeight = min((double)it.frequency / config.greedyFrequencyUrgencyDivisor, 1.0) * config.greedyFrequencyUrgencyWeight;
    double misplacedWeight = it.isMisplaced() ? config.greedyMisplacedUrgencyWeight : 0.0;
    return lossWeight + riskWeight + demandWeight + freqWeight + misplacedWeight;
}

void GreedyAlgo::classify() {
    classified_.clear();

    for (const auto& item : *items_) {
        ClassifiedItem ci;
        ci.item = item;
        ci.riskLevel = determineRisk(item);
        ci.explanation = buildExplanation(item, ci.riskLevel);
        ci.urgencyScore = computeUrgency(item);
        classified_.push_back(ci);
    }

    sort(classified_.begin(), classified_.end(),
        [](const ClassifiedItem& a, const ClassifiedItem& b) {
            return a.urgencyScore > b.urgencyScore;
        });
}

void GreedyAlgo::printClassification() const {
    cout << "\n====== RISK CLASSIFICATION ======\n";
    if (classified_.empty()) {
        cout << "No items available for classification.\n";
        cout << "=================================\n\n";
        return;
    }

    cout << left
         << setw(6) << "ID"
         << setw(18) << "Name"
         << setw(10) << "Risk"
         << setw(12) << "Score"
         << setw(12) << "Urgency"
         << "Explanation\n";
    cout << string(82, '-') << "\n";

    for (const auto& ci : classified_) {
        cout << left
             << setw(6) << ci.item.id
             << setw(18) << ci.item.name.substr(0, 17)
             << setw(10) << ci.riskLevel
             << setw(12) << fixed << setprecision(2) << ci.item.riskScore
             << setw(12) << ci.urgencyScore
             << ci.explanation << "\n";
    }

    cout << "=================================\n\n";
}

const vector<ClassifiedItem>& GreedyAlgo::getClassified() const {
    return classified_;
}

vector<ClassifiedItem> GreedyAlgo::getByRisk(const string& level) const {
    vector<ClassifiedItem> result;
    for (const auto& ci : classified_) {
        if (ci.riskLevel == level) result.push_back(ci);
    }
    return result;
}

map<string, int> GreedyAlgo::riskCounts() const {
    map<string, int> counts;
    for (const auto& ci : classified_) counts[ci.riskLevel]++;
    return counts;
}

map<string, double> GreedyAlgo::riskTotalLoss() const {
    map<string, double> totals;
    for (const auto& ci : classified_) totals[ci.riskLevel] += ci.item.financialLoss();
    return totals;
}
