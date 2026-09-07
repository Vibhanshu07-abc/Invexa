import { renderTable } from "../components/tables.js";
import { renderDonutChart } from "../charts/svg-charts.js";
import { getInventoryIntelligenceBuckets } from "../utils/data-helpers.js";
import { formatCurrency } from "../utils/format.js";

export const InventoryIntelligencePage = {
  id: "inventory-intelligence",
  title: "Inventory Intelligence",
  render(context) {
    const buckets = getInventoryIntelligenceBuckets(context.filteredItems);
    const valueChart = renderDonutChart("Inventory Value", [
      { label: "Fast Moving", value: buckets.fastMoving.reduce((sum, item) => sum + Number(item.actual || 0) * Number(item.price || 0), 0) },
      { label: "Slow Moving", value: buckets.slowMoving.reduce((sum, item) => sum + Number(item.actual || 0) * Number(item.price || 0), 0) },
      { label: "Dead Stock", value: buckets.deadStock.reduce((sum, item) => sum + Number(item.actual || 0) * Number(item.price || 0), 0) }
    ]);

    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Inventory intelligence</span>
            <h1>Movement, aging, value, and imbalance analysis</h1>
          </div>
        </div>
        <div class="two-column">
          ${valueChart}
          <section class="panel">
            <div class="panel-heading">
              <div>
                <span class="panel-kicker">Bucket Summary</span>
                <h2>Current Inventory Segments</h2>
              </div>
            </div>
            <div class="summary-tiles">
              <div class="summary-tile"><span>Fast Moving Products</span><strong>${buckets.fastMoving.length}</strong></div>
              <div class="summary-tile"><span>Slow Moving Products</span><strong>${buckets.slowMoving.length}</strong></div>
              <div class="summary-tile"><span>Dead Stock</span><strong>${buckets.deadStock.length}</strong></div>
              <div class="summary-tile"><span>Overstock</span><strong>${buckets.overstock.length}</strong></div>
              <div class="summary-tile"><span>Understock</span><strong>${buckets.understock.length}</strong></div>
              <div class="summary-tile"><span>Inventory Aging</span><strong>${buckets.aging.length}</strong></div>
            </div>
          </section>
        </div>
        ${renderTable({
          columns: [
            { key: "name", label: "Product" },
            { key: "category", label: "Category" },
            { key: "demand", label: "Demand" },
            { key: "mismatch", label: "Mismatch" },
            { key: "riskLevel", label: "Risk" },
            { key: "inventoryValue", label: "Inventory Value" }
          ],
          rows: context.filteredItems.map(item => ({
            ...item,
            inventoryValue: formatCurrency(Number(item.actual || 0) * Number(item.price || 0))
          })),
          emptyMessage: "No inventory records match the active filters."
        })}
      </section>
    `;
  }
};
