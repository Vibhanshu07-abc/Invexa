import { renderLineChart } from "../charts/svg-charts.js";
import { renderTable } from "../components/tables.js";
import { formatDate } from "../utils/format.js";

function buildAuditRows(audits) {
  return audits.map((audit, index) => ({
    checkpoint: `Audit ${index + 1}`,
    timestamp: audit.timestamp,
    auditedItems: (audit.items || []).length,
    mismatches: (audit.items || []).filter(item => Number(item.mismatch || 0) !== 0).length,
    locationIssues: (audit.items || []).filter(item => item.currentLocation && item.currentLocation.includes("Transit")).length
  }));
}

export const AuditTimelinePage = {
  id: "audit-timeline",
  title: "Audit Timeline",
  render(context) {
    const rows = buildAuditRows(context.data.audits);
    const trendChart = renderLineChart("Audit Progression", rows.map(row => ({
      label: row.checkpoint,
      mismatches: row.mismatches,
      locationIssues: row.locationIssues
    })), ["mismatches", "locationIssues"]);

    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Audit timeline</span>
            <h1>Historical audit progression and operational drift</h1>
          </div>
        </div>
        ${trendChart}
        ${renderTable({
          columns: [
            { key: "checkpoint", label: "Checkpoint" },
            { key: "timestamp", label: "Timestamp", render: row => formatDate(row.timestamp) },
            { key: "auditedItems", label: "Audited Items" },
            { key: "mismatches", label: "Mismatches" },
            { key: "locationIssues", label: "Transit or Location Issues" }
          ],
          rows
        })}
      </section>
    `;
  }
};
