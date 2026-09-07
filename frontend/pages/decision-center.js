import { renderInsightCards, renderMetricCards } from "../components/cards.js";
import { renderTable } from "../components/tables.js";
import { formatCurrency, formatPercent } from "../utils/format.js";

export const DecisionCenterPage = {
  id: "decision-center",
  title: "Decision Center",
  render(context) {
    const criticalWarehouses = context.data.warehouseHealth.filter(warehouse => Number(warehouse.healthScore || 0) < 70);
    const supplierRisks = context.data.warehouseHealth.filter(warehouse => Number(warehouse.supplierPerformance || 0) < 75);

    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Decision center</span>
            <h1>Prioritized actions across transfers, restocks, and risk containment</h1>
          </div>
        </div>
        ${renderMetricCards([
          { label: "Recommended Transfers", value: context.data.actions.filter(action => action.type === "move").length, footnote: "Warehouse transfer and relocation actions", tone: "accent" },
          { label: "Predicted Shortages", value: context.data.forecasts.length, footnote: "Forecast lines under watch", tone: "warning" },
          { label: "Financial Impact", value: formatCurrency(context.data.summary.estimatedFinancialLoss), footnote: "Current estimated exposure", tone: "danger" },
          { label: "Estimated Savings", value: formatCurrency(context.data.transferSavings), footnote: "Opportunity from proposed moves", tone: "success" }
        ])}
        ${renderInsightCards([
          { kicker: "Critical warehouses", title: `${criticalWarehouses.length} require intervention`, body: criticalWarehouses.map(warehouse => warehouse.warehouseName).join(", ") || "No warehouses fall below the critical health threshold." },
          { kicker: "Supplier risks", title: `${supplierRisks.length} warehouses need supplier review`, body: supplierRisks.map(warehouse => `${warehouse.warehouseName} (${formatPercent(warehouse.supplierPerformance)})`).join(", ") || "Supplier support is within operating range." },
          { kicker: "Restock focus", title: `${context.data.optimization.selectedItems.length} products selected`, body: context.data.optimization.status || "Optimization status unavailable." }
        ])}
        ${renderTable({
          columns: [
            { key: "title", label: "Recommendation" },
            { key: "priority", label: "Priority" },
            { key: "category", label: "Category" },
            { key: "rationale", label: "Rationale" },
            { key: "actions", label: "Actions", render: row => (row.actions || []).join(", ") }
          ],
          rows: context.data.recommendations
        })}
      </section>
    `;
  }
};
