import { escapeHtml } from "../utils/dom.js";

function renderOptions(options, selected) {
  return options.map(option => `
    <option value="${escapeHtml(option)}" ${option === selected ? "selected" : ""}>${escapeHtml(option)}</option>
  `).join("");
}

export function renderFilters(filterOptions, filters) {
  return `
    <div class="filter-bar">
      <label>
        Warehouse
        <select data-filter="warehouse">
          ${renderOptions(["all", ...filterOptions.warehouses], filters.warehouse)}
        </select>
      </label>
      <label>
        Category
        <select data-filter="category">
          ${renderOptions(["all", ...filterOptions.categories], filters.category)}
        </select>
      </label>
      <label>
        Risk
        <select data-filter="risk">
          ${renderOptions(["all", ...filterOptions.risks], filters.risk)}
        </select>
      </label>
    </div>
  `;
}
