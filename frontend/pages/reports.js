import { renderInsightCards, renderMetricCards } from "../components/cards.js";
import { renderTable } from "../components/tables.js";
import { formatCurrency } from "../utils/format.js";

export const ReportsPage = {
  id: "reports",
  title: "Reports",
  render(context) {
    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Reports</span>
            <h1>Reporting package for executives, operators, and analysts</h1>
          </div>
        </div>
        ${renderMetricCards([
          { label: "Inventory Report", value: context.data.items.length, footnote: "Item records available", tone: "brand" },
          { label: "Analytics Report", value: context.data.mismatches.length, footnote: "Mismatch entries available", tone: "warning" },
          { label: "Decision Report", value: context.data.recommendations.length, footnote: "Decision recommendations available", tone: "accent" },
          { label: "Summary Exposure", value: formatCurrency(context.data.summary.estimatedFinancialLoss), footnote: "Estimated current loss", tone: "danger" }
        ])}
        ${renderInsightCards([
          { kicker: "Operational Summary", title: "Executive reporting is current", body: context.data.ai.executiveSummary?.summary || "Executive summary output is available from the reporting layer." },
          { kicker: "Alert package", title: `${context.data.alerts.length} alerts included`, body: "The report bundle includes summary alerts for leadership review." }
        ])}
        ${renderTable({
          columns: [
            { key: "title", label: "Recommended Action" },
            { key: "priority", label: "Priority" },
            { key: "category", label: "Category" },
            { key: "rationale", label: "Rationale" }
          ],
          rows: context.data.recommendations
        })}
      </section>
    `;
  }
};
