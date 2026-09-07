#include "../include/warehouse_relationship_analyzer.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>

using namespace std;

WarehouseRelationshipAnalyzer::WarehouseRelationshipAnalyzer(const ItemList& items)
    : items_(&items), graphAlgo_(items), graphBuilt_(false) {}

void WarehouseRelationshipAnalyzer::ensureGraphReady() const {
    if (graphBuilt_) {
        return;
    }

    graphAlgo_.buildGraph();
    graphAlgo_.detectClusters();
    graphBuilt_ = true;
}

const Item* WarehouseRelationshipAnalyzer::findItem(int itemId) const {
    auto it = find_if(items_->begin(), items_->end(),
        [&](const Item& item) {
            return item.id == itemId;
        });

    if (it == items_->end()) {
        return nullptr;
    }

    return &(*it);
}

vector<Cluster> WarehouseRelationshipAnalyzer::findWarehouseClusters() {
    ensureGraphReady();
    return graphAlgo_.getClusters();
}

vector<TransferCandidate> WarehouseRelationshipAnalyzer::findTransferCandidates() const {
    ensureGraphReady();

    vector<TransferCandidate> candidates;
    set<pair<int, int>> seenPairs;

    for (const auto& edge : graphAlgo_.getEdges()) {
        const Item* from = findItem(edge.from);
        const Item* to = findItem(edge.to);
        if (from == nullptr || to == nullptr) {
            continue;
        }

        if (from->warehouseId == to->warehouseId) {
            continue;
        }

        pair<int, int> itemPair = minmax(from->id, to->id);
        if (!seenPairs.insert(itemPair).second) {
            continue;
        }

        const Item* source = from;
        const Item* target = to;
        if (target->riskScore > source->riskScore) {
            source = to;
            target = from;
        }

        TransferCandidate candidate;
        candidate.itemId = source->id;
        candidate.itemName = source->name;
        candidate.fromWarehouseId = source->warehouseId;
        candidate.fromWarehouseName = source->warehouseName;
        candidate.toWarehouseId = target->warehouseId;
        candidate.toWarehouseName = target->warehouseName;
        candidate.reason = edge.reason;
        candidate.relationshipStrength = edge.weight;
        candidates.push_back(candidate);
    }

    sort(candidates.begin(), candidates.end(),
        [](const TransferCandidate& a, const TransferCandidate& b) {
            if (a.relationshipStrength == b.relationshipStrength) {
                return a.itemId < b.itemId;
            }
            return a.relationshipStrength > b.relationshipStrength;
        });

    return candidates;
}

vector<WarehouseDependency> WarehouseRelationshipAnalyzer::findWarehouseDependencies() const {
    ensureGraphReady();

    struct DependencyAccumulator {
        int linkedItems = 0;
        double totalWeight = 0.0;
        set<string> reasons;
    };

    map<pair<int, int>, DependencyAccumulator> dependencies;

    for (const auto& edge : graphAlgo_.getEdges()) {
        const Item* from = findItem(edge.from);
        const Item* to = findItem(edge.to);
        if (from == nullptr || to == nullptr) {
            continue;
        }

        if (from->warehouseId == to->warehouseId) {
            continue;
        }

        const pair<int, int> key{from->warehouseId, to->warehouseId};
        auto& dependency = dependencies[key];
        dependency.linkedItems++;
        dependency.totalWeight += edge.weight;
        dependency.reasons.insert(edge.reason);
    }

    vector<WarehouseDependency> results;
    for (const auto& entry : dependencies) {
        WarehouseDependency dependency;
        dependency.sourceWarehouseId = entry.first.first;
        dependency.dependentWarehouseId = entry.first.second;

        const Item* sourceItem = nullptr;
        const Item* targetItem = nullptr;
        for (const auto& item : *items_) {
            if (sourceItem == nullptr && item.warehouseId == dependency.sourceWarehouseId) {
                sourceItem = &item;
            }
            if (targetItem == nullptr && item.warehouseId == dependency.dependentWarehouseId) {
                targetItem = &item;
            }
            if (sourceItem != nullptr && targetItem != nullptr) {
                break;
            }
        }

        dependency.sourceWarehouseName = sourceItem != nullptr ? sourceItem->warehouseName : "Unknown Warehouse";
        dependency.dependentWarehouseName = targetItem != nullptr ? targetItem->warehouseName : "Unknown Warehouse";
        dependency.linkedItems = entry.second.linkedItems;
        dependency.averageRelationshipStrength = entry.second.linkedItems == 0
            ? 0.0
            : entry.second.totalWeight / entry.second.linkedItems;
        dependency.reasons.assign(entry.second.reasons.begin(), entry.second.reasons.end());
        results.push_back(dependency);
    }

    sort(results.begin(), results.end(),
        [](const WarehouseDependency& a, const WarehouseDependency& b) {
            if (a.averageRelationshipStrength == b.averageRelationshipStrength) {
                return a.linkedItems > b.linkedItems;
            }
            return a.averageRelationshipStrength > b.averageRelationshipStrength;
        });

    return results;
}

void WarehouseRelationshipAnalyzer::printWarehouseRelationships() const {
    ensureGraphReady();
    graphAlgo_.printGraph();
}

void WarehouseRelationshipAnalyzer::printWarehouseClusters() const {
    ensureGraphReady();
    graphAlgo_.printClusters();
}
