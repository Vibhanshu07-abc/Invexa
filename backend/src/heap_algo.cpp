#include "../include/heap_algo.h"
#include "../include/config_service.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

HeapAlgo::HeapAlgo(const ItemList& items)
    : items_(&items)
{
    log("Initialized with " + to_string(items_->size()) + " items.");
}

void HeapAlgo::log(const string& msg) const {
    cout << "[HEAP] " << msg << "\n";
}

string HeapAlgo::assignBadge(double score, int rank) const {
    const auto& config = ConfigService::instance().data();
    if (rank == 1 || score >= config.heapCriticalScore) return "CRITICAL";
    if (score >= config.heapWarningScore) return "WARNING";
    if (score >= config.heapWatchScore) return "WATCH";
    return "STABLE";
}

void HeapAlgo::buildHeap() {
    while (!pq_.empty()) pq_.pop();
    ranked_.clear();

    for (const auto& item : *items_) {
        pq_.push(item);
    }

    int rank = 1;
    while (!pq_.empty()) {
        RankedItem ri;
        ri.item = pq_.top();
        pq_.pop();
        ri.rank = rank;
        ri.badge = assignBadge(ri.item.riskScore, rank);
        ranked_.push_back(ri);
        rank++;
    }
}

vector<RankedItem> HeapAlgo::extractTopK(int k) {
    if (ranked_.empty()) buildHeap();
    int limit = min(k, (int)ranked_.size());
    return vector<RankedItem>(ranked_.begin(), ranked_.begin() + limit);
}

vector<RankedItem> HeapAlgo::getRankedList() const {
    return ranked_;
}

void HeapAlgo::printTopK(int k) const {
    cout << "\n====== HIGH RISK ITEMS ======\n";
    if (ranked_.empty()) {
        cout << "No ranked items available.\n";
        cout << "=============================\n\n";
        return;
    }

    int limit = min(k, (int)ranked_.size());
    cout << left
         << setw(6) << "Rank"
         << setw(6) << "ID"
         << setw(18) << "Name"
         << setw(12) << "RiskScore"
         << setw(10) << "Mismatch"
         << setw(12) << "Loss($)"
         << "Badge\n";
    cout << string(78, '-') << "\n";

    for (int i = 0; i < limit; ++i) {
        const auto& ri = ranked_[i];
        cout << left
             << setw(6) << ri.rank
             << setw(6) << ri.item.id
             << setw(18) << ri.item.name.substr(0, 17)
             << setw(12) << fixed << setprecision(2) << ri.item.riskScore
             << setw(10) << ri.item.mismatch
             << setw(12) << ri.item.financialLoss()
             << ri.badge << "\n";
    }

    cout << "=============================\n\n";
}
