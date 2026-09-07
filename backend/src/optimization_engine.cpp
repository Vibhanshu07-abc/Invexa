#include "../include/optimization_engine.h"

using namespace std;

OptimizationEngine::OptimizationEngine(int topK)
    : topK_(topK) {}

void OptimizationEngine::refreshFrequencies(ItemList& items, const AuditHistory& history) const {
    for (auto& item : items) {
        item.frequency = history.mismatchFrequency(item.id);
        item.refreshDerived();
    }
}

HashAlgo OptimizationEngine::createHashAnalyzer(const ItemList& items) const {
    return HashAlgo(items);
}

HeapAlgo OptimizationEngine::createHeapAnalyzer(const ItemList& items) const {
    return HeapAlgo(items);
}

GreedyAlgo OptimizationEngine::createGreedyAnalyzer(const ItemList& items) const {
    return GreedyAlgo(items);
}

DPAlgo OptimizationEngine::createDynamicProgrammingAnalyzer(const ItemList& items, int budget) const {
    return DPAlgo(items, budget);
}

WarehouseRelationshipAnalyzer OptimizationEngine::createWarehouseRelationshipAnalyzer(const ItemList& items) const {
    return WarehouseRelationshipAnalyzer(items);
}

vector<RankedItem> OptimizationEngine::extractTopRiskItems(const ItemList& items) const {
    HeapAlgo heapAlgo = createHeapAnalyzer(items);
    return heapAlgo.extractTopK(topK_);
}

vector<TheftTimingRecord> OptimizationEngine::detectTheftTiming(const AuditHistory& history) const {
    return history.detectFirstMismatchMoments();
}
