import { PAGE_DEFINITIONS } from "./constants.js";

function toArray(value) {
  return Array.isArray(value) ? value : [];
}

function confidenceToScore(confidence) {
  const label = String(confidence || "").toLowerCase();
  if (label.includes("high")) return 91;
  if (label.includes("medium")) return 76;
  if (label.includes("low")) return 58;
  return 70;
}

function unique(values) {
  return [...new Set(values.filter(Boolean))];
}

export function normalizePlatformData(payloads) {
  const dashboard = payloads.dashboard || {};
  const inventoryData = payloads.inventory?.data || payloads.inventory || {};
  const analyticsData = payloads.analytics?.data || payloads.analytics || {};
  const decisionCenter = payloads.decisionCenter?.data || payloads.decisionCenter || {};
  const reports = payloads.reports?.data || payloads.reports || {};
  const warehouseHealth = toArray(payloads.warehouseHealth?.data?.warehouses || payloads.warehouseHealth?.warehouses);

  const items = toArray(dashboard.items || inventoryData.items);
  const warehouses = toArray(dashboard.warehouses || inventoryData.warehouses);
  const mismatches = toArray(dashboard.mismatches || analyticsData.mismatches);
  const misplacedItems = toArray(dashboard.misplacedItems || analyticsData.misplacedItems);
  const topRiskItems = toArray(dashboard.topKItems || analyticsData.topRiskItems);
  const classification = toArray(dashboard.classification || analyticsData.classification);
  const optimization = dashboard.optimization || analyticsData.optimization || { selectedItems: [] };
  const clusters = toArray(dashboard.clusters || analyticsData.clusters).map(cluster => ({
    ...cluster,
    label: String(cluster.label || "inventory cluster").replace(/theft/gi, "inventory anomaly"),
    description: String(cluster.description || "").replace(/shrink/gi, "inventory anomaly")
  }));
  const anomalyTimeline = toArray(dashboard.theftTiming || analyticsData.theftTiming).map(entry => ({
    ...entry,
    label: "Inventory Anomaly Detection",
    message: String(entry.message || "")
      .replace(/^Item\s+/i, "")
      .replace(/\swas lost between\s/i, " showed abnormal inventory behavior between "),
    likelyCauses: inferAnomalyCauses(entry, items)
  }));
  const actions = toArray(dashboard.actionReport).map(action => ({
    ...action,
    message: String(action.message || "").replace(/shrink indicators/gi, "inventory anomaly indicators")
  }));
  const alerts = toArray(dashboard.alerts);
  const audits = toArray(dashboard.audits);
  const ai = dashboard.ai || decisionCenter.ai || {};
  const forecasts = toArray(ai.demandForecasts);
  const recommendations = toArray(decisionCenter.recommendations || dashboard.decisionCenter?.recommendations);
  const fallbackRecommendations = actions.slice(0, 8).map((action, index) => ({
    title: action.name || `Action ${index + 1}`,
    priority: action.priority || "Planned",
    category: action.type || "operations",
    rationale: action.message || "Operational follow-up required.",
    actions: [action.type || "review"]
  }));
  const summary = dashboard.summary || { recommendedActions: [] };

  const inventoryValue = items.reduce((sum, item) => sum + Number(item.actual || 0) * Number(item.price || 0), 0);
  const expectedInventoryValue = items.reduce((sum, item) => sum + Number(item.expected || 0) * Number(item.price || 0), 0);
  const riskIndex = items.length
    ? items.reduce((sum, item) => sum + Number(item.riskScore || 0), 0) / items.length
    : 0;
  const forecastAccuracy = forecasts.length
    ? forecasts.reduce((sum, forecast) => sum + confidenceToScore(forecast.confidence), 0) / forecasts.length
    : 0;
  const transferSavings = actions
    .filter(action => action.type === "move" || action.type === "transfer")
    .reduce((sum, action) => {
      const match = items.find(item => Number(item.id) === Number(action.id));
      return sum + Number(match?.price || 0) * Math.max(1, Number(match?.mismatch || 0));
    }, 0);

  return {
    dashboard,
    reports,
    inventoryValue,
    expectedInventoryValue,
    riskIndex,
    forecastAccuracy,
    transferSavings,
    items,
    warehouses,
    mismatches,
    misplacedItems,
    topRiskItems,
    classification,
    optimization,
    clusters,
    anomalyTimeline,
    actions,
    alerts,
    audits,
    ai,
    forecasts,
    recommendations: recommendations.length ? recommendations : fallbackRecommendations,
    summary,
    warehouseHealth
  };
}

export function getAccessiblePages(role) {
  return PAGE_DEFINITIONS.filter(page => page.roles.includes(role));
}

export function buildGlobalSearchIndex(data) {
  const itemEntries = data.items.map(item => ({
    id: `item-${item.id}`,
    page: "inventory-intelligence",
    label: item.name,
    meta: `${item.category} · ${item.warehouseName || `Warehouse ${item.warehouseId || 1}`}`,
    tags: [item.id, item.category, item.riskLevel, item.warehouseName]
  }));

  const warehouseEntries = data.warehouseHealth.map(warehouse => ({
    id: `warehouse-${warehouse.warehouseId}`,
    page: "warehouse-operations",
    label: warehouse.warehouseName,
    meta: `Health ${warehouse.healthScore} · ${warehouse.status}`,
    tags: [warehouse.status, warehouse.warehouseId]
  }));

  const recommendationEntries = data.recommendations.map((recommendation, index) => ({
    id: `rec-${index}`,
    page: "decision-center",
    label: recommendation.title,
    meta: recommendation.category,
    tags: [recommendation.priority, ...(recommendation.actions || [])]
  }));

  return [...itemEntries, ...warehouseEntries, ...recommendationEntries];
}

export function filterItems(items, filters) {
  return items.filter(item => {
    const warehouseName = item.warehouseName || `Warehouse ${item.warehouseId || 1}`;
    const matchesWarehouse = filters.warehouse === "all" || warehouseName === filters.warehouse;
    const matchesCategory = filters.category === "all" || item.category === filters.category;
    const matchesRisk = filters.risk === "all" || item.riskLevel === filters.risk;
    return matchesWarehouse && matchesCategory && matchesRisk;
  });
}

export function buildFilterOptions(data) {
  return {
    warehouses: unique(data.items.map(item => item.warehouseName || `Warehouse ${item.warehouseId || 1}`)),
    categories: unique(data.items.map(item => item.category)),
    risks: unique(data.items.map(item => item.riskLevel))
  };
}

export function groupItemsByWarehouse(items) {
  return items.reduce((groups, item) => {
    const key = item.warehouseName || `Warehouse ${item.warehouseId || 1}`;
    if (!groups[key]) groups[key] = [];
    groups[key].push(item);
    return groups;
  }, {});
}

export function getInventoryIntelligenceBuckets(items) {
  const sortedByDemand = [...items].sort((a, b) => Number(b.demand || 0) - Number(a.demand || 0));
  const fastMoving = sortedByDemand.slice(0, 6);
  const slowMoving = [...sortedByDemand].reverse().slice(0, 6);
  const deadStock = items.filter(item => Number(item.actual || 0) > 0 && Number(item.demand || 0) <= 15);
  const overstock = items.filter(item => Number(item.actual || 0) > Number(item.expected || 0));
  const understock = items.filter(item => Number(item.actual || 0) < Number(item.expected || 0));
  const aging = items
    .filter(item => Number(item.frequency || 0) >= 2 || Number(item.mismatch || 0) !== 0)
    .sort((a, b) => Number(b.frequency || 0) - Number(a.frequency || 0));

  return {
    fastMoving,
    slowMoving,
    deadStock,
    overstock,
    understock,
    aging
  };
}

export function buildWarehouseNetwork(data) {
  let nodes = data.warehouseHealth.map(warehouse => ({
    id: String(warehouse.warehouseId),
    label: warehouse.warehouseName,
    status: warehouse.status,
    healthScore: warehouse.healthScore
  }));

  const grouped = groupItemsByWarehouse(data.items);
  const labels = Object.keys(grouped);
  const edges = [];

  for (let index = 0; index < labels.length; index += 1) {
    for (let offset = index + 1; offset < labels.length; offset += 1) {
      const sourceName = labels[index];
      const targetName = labels[offset];
      const sourceItems = grouped[sourceName];
      const targetItems = grouped[targetName];
      const sharedCategories = unique(
        sourceItems.map(item => item.category).filter(category =>
          targetItems.some(targetItem => targetItem.category === category)
        )
      );

      if (!sharedCategories.length) continue;

      const loadGap = Math.abs(
        sourceItems.reduce((sum, item) => sum + Number(item.actual || 0), 0) -
        targetItems.reduce((sum, item) => sum + Number(item.actual || 0), 0)
      );

      edges.push({
        source: String(data.warehouseHealth.find(warehouse => warehouse.warehouseName === sourceName)?.warehouseId || sourceName),
        target: String(data.warehouseHealth.find(warehouse => warehouse.warehouseName === targetName)?.warehouseId || targetName),
        weight: sharedCategories.length,
        label: sharedCategories.join(", "),
        loadGap
      });
    }
  }

  if (nodes.length <= 1 && !edges.length) {
    const locations = unique(data.items.flatMap(item => [item.expectedLocation, item.currentLocation]));
    nodes = locations.map((location, index) => ({
      id: `zone-${index + 1}`,
      label: location,
      status: "Inventory movement",
      healthScore: 75
    }));
    data.items.forEach(item => {
      if (item.expectedLocation && item.currentLocation && item.expectedLocation !== item.currentLocation) {
        edges.push({
          source: nodes.find(node => node.label === item.currentLocation)?.id,
          target: nodes.find(node => node.label === item.expectedLocation)?.id,
          weight: Math.max(1, Math.abs(Number(item.mismatch || 0))),
          label: item.name,
          loadGap: Math.abs(Number(item.mismatch || 0))
        });
      }
    });
  }

  return { nodes, edges: edges.filter(edge => edge.source && edge.target) };
}

export function buildCapacityTrend(audits, items) {
  const warehouseNames = unique(items.map(item => item.warehouseName || `Warehouse ${item.warehouseId || 1}`));
  return audits.map((audit, index) => {
    const entry = { label: audit.timestamp || `Checkpoint ${index + 1}` };
    warehouseNames.forEach(name => {
      const count = (audit.items || []).length ? Math.round(((audit.items || []).length / items.length) * 100) : 0;
      entry[name] = count;
    });
    return entry;
  });
}

export function inferAnomalyCauses(entry, items) {
  const item = items.find(candidate => Number(candidate.id) === Number(entry.id));
  const causes = [];
  if (!item) {
    return ["Inventory mismatch"];
  }
  if (Number(item.mismatch || 0) !== 0) causes.push("Inventory mismatch");
  if (item.currentLocation && item.expectedLocation && item.currentLocation !== item.expectedLocation) {
    causes.push("Unusual stock movement");
    causes.push("Scanning errors");
  }
  if (Number(item.frequency || 0) >= 2) causes.push("Delayed updates");
  if (Number(item.actual || 0) < Number(item.expected || 0) && Number(item.price || 0) > 50) {
    causes.push("Possible theft");
  }
  if (Number(item.actual || 0) > Number(item.expected || 0)) causes.push("Damaged goods");
  return unique(causes.length ? causes : ["Inventory mismatch"]);
}
