import { escapeHtml } from "../utils/dom.js";

export function renderNavigation(pages, currentPage) {
  return `
    <nav class="sidebar-nav">
      ${pages.map(page => `
        <button class="nav-link ${page.id === currentPage ? "is-active" : ""}" data-page="${escapeHtml(page.id)}">
          <span class="nav-icon">${escapeHtml(page.icon)}</span>
          <span>${escapeHtml(page.label)}</span>
        </button>
      `).join("")}
    </nav>
  `;
}
