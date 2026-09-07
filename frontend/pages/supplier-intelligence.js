import { renderInsightCards, renderWarehouseHealthCards } from "../components/cards.js";
import { renderTable } from "../components/tables.js";
import { formatPercent } from "../utils/format.js";

export const SupplierIntelligencePage = {
  id: "supplier-intelligence",
  title: "Supplier Intelligence",
  render(context) {
    const supplierRows = context.data.warehouseHealth
      .map(warehouse => ({
        warehouseName: warehouse.warehouseName,
        supplierPerformance: formatPercent(warehouse.supplierPerformance),
        status: warehouse.supplierPerformance < 75 ? "At risk" : warehouse.supplierPerformance < 85 ? "Watch" : "Stable",
        accuracy: formatPercent(warehouse.inventoryAccuracy),
        risk: formatPercent(warehouse.averageRisk)
      }))
      .sort((a, b) => a.status.localeCompare(b.status));

    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Supplier intelligence</span>
            <h1>Supplier support quality through warehouse operating signals</h1>
          </div>
        </div>
        ${renderInsightCards([
          { kicker: "Supplier risks", title: `${supplierRows.filter(row => row.status === "At risk").length} warehouses need supplier escalation`, body: "Supplier performance is derived from warehouse health data and should be treated as an operating risk indicator." },
          { kicker: "Network resilience", title: `${supplierRows.filter(row => row.status === "Stable").length} warehouses are stable`, body: "These warehouses are maintaining healthier supply support and inventory accuracy." }
        ])}
        <section class="panel">
          <div class="panel-heading">
            <div>
              <span class="panel-kicker">Warehouse supplier health</span>
              <h2>Performance Ranking</h2>
            </div>
          </div>
          ${renderWarehouseHealthCards(context.data.warehouseHealth)}
        </section>
        ${renderTable({
          columns: [
            { key: "warehouseName", label: "Warehouse" },
            { key: "supplierPerformance", label: "Supplier Performance" },
            { key: "status", label: "Risk Status" },
            { key: "accuracy", label: "Inventory Accuracy" },
            { key: "risk", label: "Average Risk" }
          ],
          rows: supplierRows
        })}
      </section>
    `;
  }
};
