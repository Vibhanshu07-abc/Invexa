#pragma once
#ifndef GRAPH_ALGO_H
#define GRAPH_ALGO_H

#include "item.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <set>

struct Edge {
    int from;
    int to;
    double weight;
    std::string reason;
};

struct Cluster {
    int clusterId;
    std::vector<int> itemIds;
    std::string label;
    std::string severity;
    double avgRisk;
    std::string description;
};

class GraphAlgo {
public:
    explicit GraphAlgo(const ItemList& items);

    void buildGraph();
    std::vector<int> bfs(int startId) const;
    std::vector<int> dfs(int startId) const;
    std::vector<Cluster> detectClusters();
    void printGraph() const;
    void printClusters() const;

    const std::vector<Cluster>& getClusters() const;
    const std::vector<Edge>& getEdges() const;

private:
    const ItemList* items_;
    std::unordered_map<int, Item> itemMap_;
    std::unordered_map<int, std::vector<Edge>> adjList_;
    std::vector<Edge> edges_;
    std::vector<Cluster> clusters_;

    bool shouldConnect(const Item& a, const Item& b, std::string& reason) const;
    double edgeWeight(const Item& a, const Item& b) const;
    void dfsUtil(int id, std::set<int>& visited, std::vector<int>& result) const;
    std::string clusterSeverity(double avgRisk) const;
    Cluster buildCluster(int clusterId, const std::vector<int>& ids) const;
    void log(const std::string& msg) const;
};

#endif
