#pragma once
#ifndef HEAP_ALGO_H
#define HEAP_ALGO_H

#include "item.h"
#include <vector>
#include <queue>
#include <string>

struct RankedItem {
    Item item;
    int rank;
    std::string badge;
};

struct RiskComparator {
    bool operator()(const Item& a, const Item& b) const {
        return a.riskScore < b.riskScore;
    }
};

class HeapAlgo {
public:
    explicit HeapAlgo(const ItemList& items);

    std::vector<RankedItem> extractTopK(int k);
    std::vector<RankedItem> getRankedList() const;
    void printTopK(int k) const;

private:
    const ItemList* items_;
    std::vector<RankedItem> ranked_;
    std::priority_queue<Item, std::vector<Item>, RiskComparator> pq_;

    void buildHeap();
    std::string assignBadge(double score, int rank) const;
    void log(const std::string& msg) const;
};

#endif
