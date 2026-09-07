import { escapeHtml } from "../utils/dom.js";

export function renderNotifications(alerts, recommendations) {
  const items = [
    ...alerts.map(alert => ({ title: "Operational Alert", body: alert })),
    ...recommendations.slice(0, 4).map(rec => ({ title: rec.title, body: rec.rationale || rec.category }))
  ];

  return `
    <section class="notification-drawer">
      <div class="drawer-head">
        <h3>Notification Center</h3>
        <span>${items.length} active</span>
      </div>
      <div class="drawer-list">
        ${items.map(item => `
          <article class="drawer-card">
            <strong>${escapeHtml(item.title)}</strong>
            <p>${escapeHtml(item.body)}</p>
          </article>
        `).join("")}
      </div>
    </section>
  `;
}
