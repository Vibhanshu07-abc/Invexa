import { renderMetricCards } from "../components/cards.js";
import { renderTable } from "../components/tables.js";
import { escapeHtml } from "../utils/dom.js";

export const AdministrationPage = {
  id: "administration",
  title: "Administration",
  render(context) {
    const session = context.session || { role: "Unauthenticated", username: "-" };
    const canEdit = session.role === "Admin";
    const warehouses = context.data.warehouseHealth || [];
    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Administration</span>
            <h1>Environment visibility, role access, and platform controls</h1>
          </div>
        </div>
        ${renderMetricCards([
          { label: "Current Role", value: session.role, footnote: "Role-based navigation is active", tone: "brand" },
          { label: "User", value: session.username, footnote: "Authenticated account", tone: "accent" },
          { label: "Warehouses", value: context.data.warehouseHealth.length, footnote: "Warehouse entities visible to the frontend", tone: "success" },
          { label: "REST Surfaces", value: 8, footnote: "Dashboard, inventory, analytics, decision center, reports, health, auth, copilot", tone: "warning" }
        ])}
        <section class="panel">
          <div class="panel-heading">
            <div>
              <span class="panel-kicker">Executive data controls</span>
              <h2>Edit warehouse dashboard values</h2>
              <p>${canEdit ? "Changes update this browser immediately and remain available after refresh. They are dashboard overrides; the CSV remains the system of record." : "Only an Admin can edit dashboard values."}</p>
            </div>
          </div>
          ${canEdit ? `
            <form id="warehouseOverrideForm" class="admin-form">
              <label>Warehouse
                <select id="warehouseOverrideId">
                  ${warehouses.map(warehouse => `<option value="${warehouse.warehouseId}">${escapeHtml(warehouse.warehouseName)}</option>`).join("")}
                </select>
              </label>
              <label>Display name<input id="warehouseOverrideName" required value="${escapeHtml(warehouses[0]?.warehouseName || "")}"></label>
              <label>Health score<input id="warehouseOverrideHealth" required type="number" min="0" max="100" step="0.1" value="${warehouses[0]?.healthScore || 0}"></label>
              <label>Inventory accuracy<input id="warehouseOverrideAccuracy" required type="number" min="0" max="100" step="0.1" value="${warehouses[0]?.inventoryAccuracy || 0}"></label>
              <label>Capacity utilization<input id="warehouseOverrideCapacity" required type="number" min="0" max="100" step="0.1" value="${warehouses[0]?.capacityUtilization || 0}"></label>
              <label>Supplier performance<input id="warehouseOverrideSupplier" required type="number" min="0" max="100" step="0.1" value="${warehouses[0]?.supplierPerformance || 0}"></label>
              <div class="admin-form-actions">
                <button class="primary-action" type="submit">Save dashboard override</button>
                <button class="ghost-action" id="resetWarehouseOverrides" type="button">Reset local edits</button>
              </div>
            </form>
          ` : ""}
        </section>
        ${renderTable({
          columns: [
            { key: "endpoint", label: "Endpoint" },
            { key: "purpose", label: "Purpose" },
            { key: "method", label: "Method" }
          ],
          rows: [
            { endpoint: "/api/dashboard", purpose: "Executive and cross-platform aggregate payload", method: "GET" },
            { endpoint: "/api/inventory", purpose: "Inventory and warehouse records", method: "GET" },
            { endpoint: "/api/analytics", purpose: "Risk, anomaly, and optimization analytics", method: "GET" },
            { endpoint: "/api/decision-center", purpose: "Recommendations and AI insight bundle", method: "GET" },
            { endpoint: "/api/reports", purpose: "Report package payload", method: "GET" },
            { endpoint: "/api/warehouse-health", purpose: "Warehouse health cards and benchmarking", method: "GET" },
            { endpoint: "/api/auth/login", purpose: "Authentication", method: "POST" },
            { endpoint: "/api/copilot", purpose: "AI copilot question answering", method: "POST" }
          ]
        })}
      </section>
    `;
  }
};
