import { renderTable } from "../components/tables.js";
import { renderBarChart, renderDonutChart, renderLineChart } from "../charts/svg-charts.js";
import { buildCapacityTrend, groupItemsByWarehouse } from "../utils/data-helpers.js";
import { formatCurrency, formatPercent } from "../utils/format.js";

export const WarehouseOperationsPage = {
  id: "warehouse-operations",
  title: "Warehouse Operations",
  render(context) {
    const warehouseGroups = groupItemsByWarehouse(context.filteredItems);
    const comparisonRows = context.data.warehouseHealth.map(warehouse => ({
      warehouseName: warehouse.warehouseName,
      capacity: formatPercent(warehouse.capacityUtilization),
      health: formatPercent(warehouse.healthScore),
      supplier: formatPercent(warehouse.supplierPerformance),
      risk: formatPercent(warehouse.averageRisk)
    }));

    const distributionChart = renderDonutChart("Inventory Distribution", Object.entries(warehouseGroups).map(([warehouseName, items]) => ({
      label: warehouseName,
      value: items.length
    })));

    const utilizationChart = renderBarChart("Capacity Utilization", context.data.warehouseHealth.map(warehouse => ({
      label: warehouse.warehouseName,
      value: warehouse.capacityUtilization,
      display: formatPercent(warehouse.capacityUtilization)
    })));

    const trendChart = renderLineChart(
      "Utilization Trends",
      buildCapacityTrend(context.data.audits, context.data.items).map(point => ({
        ...point,
        label: point.label.slice(11, 16)
      })),
      context.data.warehouseHealth.map(warehouse => warehouse.warehouseName)
    );

    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Warehouse operations</span>
            <h1>Capacity, balance, and flow across active warehouses</h1>
          </div>
          <div class="chip-row">
            <span class="chip">${context.data.warehouseHealth.length} warehouses</span>
            <span class="chip">${context.filteredItems.length} filtered inventory records</span>
          </div>
        </div>
        <div class="two-column">
          ${utilizationChart}
          ${distributionChart}
        </div>
        ${trendChart}
        <section class="panel">
          <div class="panel-heading">
            <div>
              <span class="panel-kicker">Warehouse Comparison</span>
              <h2>Operational Benchmarking</h2>
            </div>
          </div>
          ${renderTable({
            columns: [
              { key: "warehouseName", label: "Warehouse" },
              { key: "capacity", label: "Capacity Utilization" },
              { key: "health", label: "Health Score" },
              { key: "supplier", label: "Supplier Performance" },
              { key: "risk", label: "Average Risk" }
            ],
            rows: comparisonRows
          })}
        </section>
        <section class="panel">
          <div class="panel-heading">
            <div>
              <span class="panel-kicker">Warehouse Overview</span>
              <h2>Inventory Value by Warehouse</h2>
            </div>
          </div>
          ${renderTable({
            columns: [
              { key: "warehouse", label: "Warehouse" },
              { key: "items", label: "Items" },
              { key: "inventoryValue", label: "Inventory Value" },
              { key: "mismatches", label: "Mismatch Count" }
            ],
            rows: Object.entries(warehouseGroups).map(([warehouse, items]) => ({
              warehouse,
              items: items.length,
              inventoryValue: formatCurrency(items.reduce((sum, item) => sum + Number(item.actual || 0) * Number(item.price || 0), 0)),
              mismatches: items.filter(item => Number(item.mismatch || 0) !== 0).length
            }))
          })}
        </section>
      </section>
    `;
  }
};
