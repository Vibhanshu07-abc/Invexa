#pragma once

#ifndef OPTIMIZATION_ENGINE_H
#define OPTIMIZATION_ENGINE_H

#include "audit_history.h"
#include "dp_algo.h"
#include "greedy_algo.h"
#include "hash_algo.h"
#include "heap_algo.h"
#include "item.h"
#include "warehouse_relationship_analyzer.h"

#include <vector>

class OptimizationEngine {
public:
    explicit OptimizationEngine(int topK = 5);

    void refreshFrequencies(ItemList& items, const AuditHistory& history) const;

    HashAlgo createHashAnalyzer(const ItemList& items) const;
    HeapAlgo createHeapAnalyzer(const ItemList& items) const;
    GreedyAlgo createGreedyAnalyzer(const ItemList& items) const;
    DPAlgo createDynamicProgrammingAnalyzer(const ItemList& items, int budget) const;
    WarehouseRelationshipAnalyzer createWarehouseRelationshipAnalyzer(const ItemList& items) const;

    std::vector<RankedItem> extractTopRiskItems(const ItemList& items) const;
    std::vector<TheftTimingRecord> detectTheftTiming(const AuditHistory& history) const;

private:
    int topK_;
};

#endif
