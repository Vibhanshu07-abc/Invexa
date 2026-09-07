import { renderInsightCards, renderMetricCards } from "../components/cards.js";
import { renderTable } from "../components/tables.js";
import { formatDate } from "../utils/format.js";

export const InventoryAnomalyDetectionPage = {
  id: "inventory-anomaly-detection",
  title: "Inventory Anomaly Detection",
  render(context) {
    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Inventory anomaly detection</span>
            <h1>Abnormal inventory behavior without assuming a single root cause</h1>
          </div>
        </div>
        ${renderMetricCards([
          { label: "Active Anomalies", value: context.data.anomalyTimeline.length, footnote: "Detected from audit history", tone: "danger" },
          { label: "Inventory Mismatches", value: context.data.mismatches.length, footnote: "Items with stock variance", tone: "warning" },
          { label: "Unusual Movement", value: context.data.misplacedItems.length, footnote: "Items outside expected location", tone: "accent" },
          { label: "Repeated Signals", value: context.data.items.filter(item => Number(item.frequency || 0) >= 2).length, footnote: "Items recurring in anomaly history", tone: "brand" }
        ])}
        ${renderInsightCards([
          { kicker: "Possible causes", title: "Inventory mismatch", body: "Stock counts differ from expected levels and need reconciliation." },
          { kicker: "Possible causes", title: "Damaged goods", body: "Unexpected count or placement changes can result from damaged or quarantined stock." },
          { kicker: "Possible causes", title: "Delayed updates", body: "Audit history indicates some records may lag physical movement." },
          { kicker: "Possible causes", title: "Scanning errors or unusual movement", body: "Location drift and repeated anomalies often point to workflow issues before loss." }
        ])}
        ${renderTable({
          columns: [
            { key: "name", label: "Product" },
            { key: "previousAudit", label: "Previous Audit", render: row => formatDate(row.previousAudit) },
            { key: "mismatchAudit", label: "Detected Audit", render: row => formatDate(row.mismatchAudit) },
            { key: "likelyCauses", label: "Likely Causes", render: row => row.likelyCauses.join(", ") },
            { key: "message", label: "Observation" }
          ],
          rows: context.data.anomalyTimeline,
          emptyMessage: "No anomaly records were returned from audit history."
        })}
      </section>
    `;
  }
};
