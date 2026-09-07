import { escapeHtml } from "../utils/dom.js";

export function renderTable({ columns, rows, emptyMessage = "No records available." }) {
  if (!rows.length) {
    return `<div class="empty-inline">${escapeHtml(emptyMessage)}</div>`;
  }

  return `
    <div class="table-shell">
      <table class="data-table">
        <thead>
          <tr>${columns.map(column => `<th>${escapeHtml(column.label)}</th>`).join("")}</tr>
        </thead>
        <tbody>
          ${rows.map(row => `
            <tr>
              ${columns.map(column => `<td>${column.render ? column.render(row) : escapeHtml(row[column.key])}</td>`).join("")}
            </tr>
          `).join("")}
        </tbody>
      </table>
    </div>
  `;
}
