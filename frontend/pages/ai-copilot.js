import { COPILOT_SUGGESTIONS } from "../utils/constants.js";
import { escapeHtml } from "../utils/dom.js";

export const AICopilotPage = {
  id: "ai-copilot",
  title: "AI Copilot",
  render(context) {
    const response = context.store.getState().copilotResponse;
    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">AI copilot</span>
            <h1>Ask the platform for warehouse intelligence guidance</h1>
          </div>
        </div>
        <section class="panel copilot-panel">
          <form id="copilotForm" class="copilot-form">
            <label>
              Question
              <textarea id="copilotQuestion" rows="4" placeholder="Ask about warehouse attention, shortages, anomalies, or decision priorities."></textarea>
            </label>
            <div class="copilot-actions">
              <button class="primary-action" type="submit">Ask Copilot</button>
              <div class="suggestion-row">
                ${COPILOT_SUGGESTIONS.map(question => `<button type="button" class="ghost-action" data-copilot-question="${escapeHtml(question)}">${escapeHtml(question)}</button>`).join("")}
              </div>
            </div>
          </form>
          <div class="copilot-response">
            <h3>Response</h3>
            ${response ? `
              <article class="drawer-card">
                <strong>${escapeHtml(response.question)}</strong>
                <p>${escapeHtml(response.answer)}</p>
                <p><strong>Source of truth:</strong> ${escapeHtml(response.sourceOfTruth || "REST service")}</p>
                ${Array.isArray(response.evidence) && response.evidence.length ? `<ul>${response.evidence.map(item => `<li>${escapeHtml(item)}</li>`).join("")}</ul>` : ""}
              </article>
            ` : `<p>No copilot query has been submitted yet.</p>`}
          </div>
        </section>
      </section>
    `;
  }
};
