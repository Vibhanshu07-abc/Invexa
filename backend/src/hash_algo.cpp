#include "../include/hash_algo.h"
#include "../include/config_service.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

HashAlgo::HashAlgo(const ItemList& items)
    : items_(&items), totalLoss_(0.0)
{
    log("Initialized with " + to_string(items_->size()) + " items.");
}

void HashAlgo::log(const string& msg) const {
    cout << "[HASH] " << msg << "\n";
}

string HashAlgo::severityLabel(const Item& item) const {
    const auto& config = ConfigService::instance().data();
    if (item.riskScore >= config.hashHighRiskScore || item.mismatch >= config.hashHighMismatch) return "HIGH";
    if (item.riskScore >= config.hashMediumRiskScore || item.mismatch >= config.hashMediumMismatch) return "MEDIUM";
    return "LOW";
}

string HashAlgo::recommendedAction(const Item& item) const {
    if (item.mismatch > 0) return "Restock and verify shrink source";
    if (item.mismatch < 0) return "Validate over-count and reconcile stock";
    return "No stock correction needed";
}

void HashAlgo::trackFrequency() {
    const auto& config = ConfigService::instance().data();
    frequencyMap_.clear();
    for (const auto& item : *items_) {
        int freq = item.frequency;
        if (freq == 0 && item.mismatch != 0) {
            int absDelta = abs(item.mismatch);
            if (absDelta >= config.hashFrequencySevereDelta) freq = config.hashFrequencySevereValue;
            else if (absDelta >= config.hashFrequencyHighDelta) freq = config.hashFrequencyHighValue;
            else if (absDelta >= config.hashFrequencyMediumDelta) freq = config.hashFrequencyMediumValue;
            else freq = config.hashFrequencyLowValue;
        }
        if (freq > 0) frequencyMap_[item.id] = freq;
    }
}

void HashAlgo::detectMismatches() {
    mismatches_.clear();
    totalLoss_ = 0.0;

    for (const auto& item : *items_) {
        if (item.expected == item.actual) continue;

        MismatchRecord rec;
        rec.itemId = item.id;
        rec.itemName = item.name;
        rec.expected = item.expected;
        rec.actual = item.actual;
        rec.delta = item.mismatch;
        rec.price = item.price;
        rec.loss = item.financialLoss();
        rec.frequency = frequencyMap_.count(item.id) ? frequencyMap_[item.id] : item.frequency;
        rec.riskLevel = severityLabel(item);
        rec.action = recommendedAction(item);

        mismatches_.push_back(rec);
        totalLoss_ += rec.loss;
    }

    sort(mismatches_.begin(), mismatches_.end(),
        [](const MismatchRecord& a, const MismatchRecord& b) {
            if (a.loss == b.loss) return abs(a.delta) > abs(b.delta);
            return a.loss > b.loss;
        });

    log("Mismatch count: " + to_string(mismatches_.size()));
}

void HashAlgo::printReport() const {
    cout << "\n====== STOCK MISMATCH REPORT ======\n";
    if (mismatches_.empty()) {
        cout << "All stock counts match expected values.\n";
        cout << "===================================\n\n";
        return;
    }

    cout << left
         << setw(6) << "ID"
         << setw(18) << "Name"
         << setw(10) << "Expected"
         << setw(10) << "Actual"
         << setw(10) << "Delta"
         << setw(12) << "Loss($)"
         << setw(10) << "Risk"
         << "Action\n";
    cout << string(92, '-') << "\n";

    for (const auto& rec : mismatches_) {
        cout << left
             << setw(6) << rec.itemId
             << setw(18) << rec.itemName.substr(0, 17)
             << setw(10) << rec.expected
             << setw(10) << rec.actual
             << setw(10) << rec.delta
             << setw(12) << fixed << setprecision(2) << rec.loss
             << setw(10) << rec.riskLevel
             << rec.action << "\n";
    }

    cout << string(92, '-') << "\n";
    cout << "Total loss: $" << fixed << setprecision(2) << totalLoss_ << "\n";
    cout << "===================================\n\n";
}

const vector<MismatchRecord>& HashAlgo::getMismatches() const {
    return mismatches_;
}

const unordered_map<int, int>& HashAlgo::getFrequencyMap() const {
    return frequencyMap_;
}

double HashAlgo::getTotalLoss() const {
    return totalLoss_;
}

int HashAlgo::getMismatchCount() const {
    return (int)mismatches_.size();
}
