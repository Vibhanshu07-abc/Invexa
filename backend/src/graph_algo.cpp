#include "../include/graph_algo.h"
#include "../include/config_service.h"
#include <iostream>
#include <iomanip>
#include <queue>
#include <algorithm>
#include <map>

using namespace std;

GraphAlgo::GraphAlgo(const ItemList& items)
    : items_(&items)
{
    log("Initialized with " + to_string(items_->size()) + " items.");
}

void GraphAlgo::log(const string& msg) const {
    cout << "[GRAPH] " << msg << "\n";
}

bool GraphAlgo::shouldConnect(const Item& a, const Item& b, string& reason) const {
    const auto& config = ConfigService::instance().data();
    if (a.category == b.category) {
        reason = "same-category";
        return true;
    }
    if (a.expectedLocation == b.expectedLocation) {
        reason = "same-location";
        return true;
    }
    if (abs(a.riskScore - b.riskScore) <= config.graphSimilarRiskThreshold) {
        reason = "similar-risk";
        return true;
    }
    return false;
}

double GraphAlgo::edgeWeight(const Item& a, const Item& b) const {
    const auto& config = ConfigService::instance().data();
    double weight = 0.0;
    if (a.category == b.category) weight += config.graphSameCategoryWeight;
    if (a.expectedLocation == b.expectedLocation) weight += config.graphSameLocationWeight;
    weight += max(0.0, config.graphRiskWeightBase -
        min(abs(a.riskScore - b.riskScore), config.graphRiskDifferenceCap) / config.graphRiskDifferenceDivisor);
    return min(weight, 1.0);
}

void GraphAlgo::buildGraph() {
    itemMap_.clear();
    adjList_.clear();
    edges_.clear();
    clusters_.clear();

    for (const auto& item : *items_) {
        itemMap_[item.id] = item;
        adjList_[item.id] = {};
    }

    for (size_t i = 0; i < items_->size(); ++i) {
        for (size_t j = i + 1; j < items_->size(); ++j) {
            string reason;
            const Item& a = (*items_)[i];
            const Item& b = (*items_)[j];

            if (!shouldConnect(a, b, reason)) continue;

            Edge ab{a.id, b.id, edgeWeight(a, b), reason};
            Edge ba{b.id, a.id, ab.weight, reason};

            adjList_[a.id].push_back(ab);
            adjList_[b.id].push_back(ba);
            edges_.push_back(ab);
        }
    }
}

vector<int> GraphAlgo::bfs(int startId) const {
    vector<int> order;
    if (!adjList_.count(startId)) return order;

    set<int> visited;
    queue<int> q;
    visited.insert(startId);
    q.push(startId);

    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        order.push_back(cur);

        auto it = adjList_.find(cur);
        if (it == adjList_.end()) continue;

        for (const auto& edge : it->second) {
            if (!visited.count(edge.to)) {
                visited.insert(edge.to);
                q.push(edge.to);
            }
        }
    }

    return order;
}

void GraphAlgo::dfsUtil(int id, set<int>& visited, vector<int>& result) const {
    visited.insert(id);
    result.push_back(id);

    auto it = adjList_.find(id);
    if (it == adjList_.end()) return;

    for (const auto& edge : it->second) {
        if (!visited.count(edge.to)) dfsUtil(edge.to, visited, result);
    }
}

vector<int> GraphAlgo::dfs(int startId) const {
    vector<int> order;
    if (!adjList_.count(startId)) return order;
    set<int> visited;
    dfsUtil(startId, visited, order);
    return order;
}

string GraphAlgo::clusterSeverity(double avgRisk) const {
    const auto& config = ConfigService::instance().data();
    if (avgRisk >= config.graphSeverityHighRisk) return "HIGH";
    if (avgRisk >= config.graphSeverityMediumRisk) return "MEDIUM";
    return "LOW";
}

Cluster GraphAlgo::buildCluster(int clusterId, const vector<int>& ids) const {
    Cluster cluster;
    cluster.clusterId = clusterId;
    cluster.itemIds = ids;
    cluster.avgRisk = 0.0;

    map<string, int> locationCount;
    int highRiskCount = 0;
    int misplacedCount = 0;

    for (int id : ids) {
        const Item& item = itemMap_.at(id);
        cluster.avgRisk += item.riskScore;
        locationCount[item.expectedLocation]++;
        if (item.riskLevel == "HIGH") highRiskCount++;
        if (item.isMisplaced()) misplacedCount++;
    }

    cluster.avgRisk = ids.empty() ? 0.0 : cluster.avgRisk / ids.size();
    cluster.severity = clusterSeverity(cluster.avgRisk);

    int topLocationCount = 0;
    for (const auto& pair : locationCount) topLocationCount = max(topLocationCount, pair.second);

    const auto& config = ConfigService::instance().data();
    if (cluster.avgRisk >= config.graphTheftClusterAvgRisk ||
        highRiskCount >= max(1, (int)ids.size() / config.graphHighRiskClusterDivisor)) {
        cluster.label = "theft-prone cluster";
        cluster.description = "High-risk items are tightly linked and should be audited for shrink patterns.";
    } else if (misplacedCount > 0 || topLocationCount >= config.graphMinSharedLocationCount) {
        cluster.label = "location-based issue cluster";
        cluster.description = "Items share storage patterns that suggest shelf movement or placement drift.";
    } else {
        cluster.label = "stable cluster";
        cluster.description = "Connected items share attributes but show comparatively stable behavior.";
    }

    return cluster;
}

vector<Cluster> GraphAlgo::detectClusters() {
    clusters_.clear();
    set<int> visited;
    int clusterId = 1;

    for (const auto& pair : adjList_) {
        int startId = pair.first;
        if (visited.count(startId)) continue;

        vector<int> component;
        dfsUtil(startId, visited, component);
        if (!component.empty()) {
            clusters_.push_back(buildCluster(clusterId, component));
            clusterId++;
        }
    }

    sort(clusters_.begin(), clusters_.end(),
        [](const Cluster& a, const Cluster& b) {
            if (a.avgRisk == b.avgRisk) return a.itemIds.size() > b.itemIds.size();
            return a.avgRisk > b.avgRisk;
        });

    return clusters_;
}

void GraphAlgo::printGraph() const {
    cout << "\n====== GRAPH SUMMARY ======\n";
    cout << "Nodes: " << items_->size() << "\n";
    cout << "Edges: " << edges_.size() << "\n";
    if (edges_.empty()) {
        cout << "No meaningful relationships found.\n";
    } else {
        for (size_t i = 0; i < edges_.size() && i < 12; ++i) {
            const auto& edge = edges_[i];
            cout << edge.from << " -> " << edge.to
                 << " | " << edge.reason
                 << " | weight=" << fixed << setprecision(2) << edge.weight << "\n";
        }
    }
    cout << "===========================\n\n";
}

void GraphAlgo::printClusters() const {
    cout << "\n====== GRAPH CLUSTERS ======\n";
    if (clusters_.empty()) {
        cout << "No clusters found.\n";
        cout << "============================\n\n";
        return;
    }

    for (const auto& cluster : clusters_) {
        cout << "Cluster #" << cluster.clusterId
             << " | " << cluster.label
             << " | AvgRisk=" << fixed << setprecision(2) << cluster.avgRisk
             << " | Items=" << cluster.itemIds.size() << "\n";
        cout << "  " << cluster.description << "\n";
    }
    cout << "============================\n\n";
}

const vector<Cluster>& GraphAlgo::getClusters() const {
    return clusters_;
}

const vector<Edge>& GraphAlgo::getEdges() const {
    return edges_;
}
