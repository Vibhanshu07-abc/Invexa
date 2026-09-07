import { renderAlertStack, renderKpiStrip, renderSummaryTiles, renderWarehouseHealthCards } from "../components/cards.js";
import { renderBarChart, renderDonutChart } from "../charts/svg-charts.js";
import { formatCompact, formatCurrency, formatPercent } from "../utils/format.js";

export const ExecutiveDashboardPage = {
  id: "executive-dashboard",
  title: "Executive Dashboard",
  render(context) {
    const warehouseValueChart = renderBarChart("Warehouse Health", context.data.warehouseHealth.map(warehouse => ({
      label: warehouse.warehouseName,
      value: warehouse.healthScore,
      display: formatPercent(warehouse.healthScore)
    })));

    const riskMix = renderDonutChart("Risk Mix", [
      { label: "High", value: context.data.items.filter(item => item.riskLevel === "HIGH").length },
      { label: "Medium", value: context.data.items.filter(item => item.riskLevel === "MEDIUM").length },
      { label: "Low", value: context.data.items.filter(item => item.riskLevel === "LOW").length }
    ]);

    return `
      <section class="page-shell">
        <div class="page-hero">
          <div class="hero-copy">
            <span class="page-eyebrow">Executive command center</span>
            <h1>Warehouse Intelligence Platform</h1>
            <p>Monitor network health, financial exposure, forecast confidence, and cross-warehouse risk from one operating surface designed for a polished capstone demonstration.</p>
            <div class="hero-badges">
              <span class="chip">Operations Intelligence</span>
              <span class="chip">Decision Support</span>
              <span class="chip">Academic Showcase</span>
            </div>
          </div>
          <div class="hero-callout">
            <span>Operational Summary</span>
            <strong>${formatCompact(context.data.recommendations.length)} live recommendations</strong>
            <p>${context.data.ai.executiveSummary?.summary || "Warehouse intelligence summary is ready for review."}</p>
            ${context.session?.role === "Admin" ? '<button class="ghost-action" type="button" data-page="administration">Edit operational data</button>' : ""}
          </div>
        </div>
        ${renderKpiStrip(context.data)}
        ${renderSummaryTiles(context.data)}
        <div class="two-column">
          ${warehouseValueChart}
          ${riskMix}
        </div>
        <section class="panel">
          <div class="panel-heading">
            <div>
              <span class="panel-kicker">Warehouse Health</span>
              <h2>Network Health Overview</h2>
            </div>
            <div class="chip-row">
              <span class="chip">Inventory Value ${formatCurrency(context.data.inventoryValue)}</span>
              <span class="chip">Transfer Savings ${formatCurrency(context.data.transferSavings)}</span>
            </div>
          </div>
          ${renderWarehouseHealthCards(context.data.warehouseHealth)}
        </section>
        <section class="panel">
          <div class="panel-heading">
            <div>
              <span class="panel-kicker">Active Alerts</span>
              <h2>Operational Risks To Escalate</h2>
            </div>
          </div>
          ${renderAlertStack(context.data.alerts)}
        </section>
      </section>
    `;
  }
};
