#pragma once

#ifndef WAREHOUSE_RELATIONSHIP_ANALYZER_H
#define WAREHOUSE_RELATIONSHIP_ANALYZER_H

#include "graph_algo.h"

#include <string>
#include <vector>

struct TransferCandidate {
    int itemId;
    std::string itemName;
    int fromWarehouseId;
    std::string fromWarehouseName;
    int toWarehouseId;
    std::string toWarehouseName;
    std::string reason;
    double relationshipStrength;
};

struct WarehouseDependency {
    int sourceWarehouseId;
    std::string sourceWarehouseName;
    int dependentWarehouseId;
    std::string dependentWarehouseName;
    int linkedItems;
    double averageRelationshipStrength;
    std::vector<std::string> reasons;
};

class WarehouseRelationshipAnalyzer {
public:
    explicit WarehouseRelationshipAnalyzer(const ItemList& items);

    std::vector<Cluster> findWarehouseClusters();
    std::vector<TransferCandidate> findTransferCandidates() const;
    std::vector<WarehouseDependency> findWarehouseDependencies() const;
    void printWarehouseRelationships() const;
    void printWarehouseClusters() const;

private:
    const ItemList* items_;
    mutable GraphAlgo graphAlgo_;
    mutable bool graphBuilt_;

    void ensureGraphReady() const;
    const Item* findItem(int itemId) const;
};

#endif
