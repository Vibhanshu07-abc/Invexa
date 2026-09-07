import { escapeHtml } from "../utils/dom.js";
import { formatCompact, formatCurrency, formatNumber, formatPercent } from "../utils/format.js";

export function renderMetricCards(metrics) {
  return `
    <div class="metric-grid">
      ${metrics.map(metric => `
        <article class="metric-card tone-${escapeHtml(metric.tone || "neutral")}">
          <span class="metric-label">${escapeHtml(metric.label)}</span>
          <strong class="metric-value">${escapeHtml(metric.value)}</strong>
          <p class="metric-footnote">${escapeHtml(metric.footnote || "")}</p>
        </article>
      `).join("")}
    </div>
  `;
}

export function renderInsightCards(cards) {
  return `
    <div class="insight-grid">
      ${cards.map(card => `
        <article class="panel subtle-card">
          <div class="panel-kicker">${escapeHtml(card.kicker || "")}</div>
          <h3>${escapeHtml(card.title)}</h3>
          <p>${escapeHtml(card.body || "")}</p>
          ${card.meta ? `<span class="chip">${escapeHtml(card.meta)}</span>` : ""}
        </article>
      `).join("")}
    </div>
  `;
}

export function renderKpiStrip(data) {
  const metrics = [
    { label: "Inventory Value", value: formatCurrency(data.inventoryValue), footnote: "Current stock valuation", tone: "brand" },
    { label: "Risk Index", value: formatPercent(data.riskIndex, 1), footnote: "Average risk across active inventory", tone: "warning" },
    { label: "Forecast Accuracy", value: formatPercent(data.forecastAccuracy, 0), footnote: "AI forecast confidence proxy", tone: "success" },
    { label: "Transfer Savings", value: formatCurrency(data.transferSavings), footnote: "Opportunity from recommended moves", tone: "accent" }
  ];
  return renderMetricCards(metrics);
}

export function renderWarehouseHealthCards(warehouses) {
  return `
    <div class="health-grid">
      ${warehouses.map(warehouse => `
        <article class="panel health-card">
          <div class="health-score">${formatPercent(warehouse.healthScore)}</div>
          <div>
            <h3>${escapeHtml(warehouse.warehouseName)}</h3>
            <p>${escapeHtml(warehouse.status)}</p>
          </div>
          <dl class="health-list">
            <div><dt>Capacity</dt><dd>${formatPercent(warehouse.capacityUtilization)}</dd></div>
            <div><dt>Accuracy</dt><dd>${formatPercent(warehouse.inventoryAccuracy)}</dd></div>
            <div><dt>Supplier</dt><dd>${formatPercent(warehouse.supplierPerformance)}</dd></div>
          </dl>
        </article>
      `).join("")}
    </div>
  `;
}

export function renderAlertStack(alerts) {
  return `
    <div class="alert-stack">
      ${alerts.map(alert => `
        <article class="alert-card">
          <span class="alert-badge">Alert</span>
          <p>${escapeHtml(alert)}</p>
        </article>
      `).join("")}
    </div>
  `;
}

export function renderSummaryTiles(data) {
  const tiles = [
    { label: "Total Items", value: formatNumber(data.summary.totalItems) },
    { label: "Lost Items", value: formatNumber(data.summary.lostItems) },
    { label: "Misplaced Items", value: formatNumber(data.summary.misplacedItems) },
    { label: "High Risk Items", value: formatNumber(data.summary.highRiskItems) },
    { label: "Expected Inventory Value", value: formatCurrency(data.expectedInventoryValue) },
    { label: "Recommended Actions", value: formatCompact((data.summary.recommendedActions || []).length) }
  ];

  return `
    <div class="summary-tiles">
      ${tiles.map(tile => `
        <div class="summary-tile">
          <span>${escapeHtml(tile.label)}</span>
          <strong>${escapeHtml(tile.value)}</strong>
        </div>
      `).join("")}
    </div>
  `;
}
