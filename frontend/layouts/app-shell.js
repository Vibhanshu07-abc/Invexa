import { renderFilters } from "../components/filters.js";
import { renderNavigation } from "../components/navigation.js";
import { renderNotifications } from "../components/notifications.js";
import { renderEmptyState, renderErrorState, renderLoadingState } from "../components/states.js";
import { PlatformService } from "../services/platform-service.js";
import { PAGE_DEFINITIONS, ROLE_LABELS } from "../utils/constants.js";
import { buildFilterOptions, buildGlobalSearchIndex, filterItems, getAccessiblePages, normalizePlatformData } from "../utils/data-helpers.js";
import { createStore } from "../utils/state.js";
import { ExecutiveDashboardPage } from "../pages/executive-dashboard.js";
import { WarehouseOperationsPage } from "../pages/warehouse-operations.js";
import { InventoryIntelligencePage } from "../pages/inventory-intelligence.js";
import { WarehouseNetworkPage } from "../pages/warehouse-network.js";
import { DecisionCenterPage } from "../pages/decision-center.js";
import { DemandForecastPage } from "../pages/demand-forecast.js";
import { InventoryAnomalyDetectionPage } from "../pages/inventory-anomaly-detection.js";
import { SupplierIntelligencePage } from "../pages/supplier-intelligence.js";
import { AuditTimelinePage } from "../pages/audit-timeline.js";
import { ReportsPage } from "../pages/reports.js";
import { AICopilotPage } from "../pages/ai-copilot.js";
import { AdministrationPage } from "../pages/administration.js";

const pageRegistry = new Map([
  [ExecutiveDashboardPage.id, ExecutiveDashboardPage],
  [WarehouseOperationsPage.id, WarehouseOperationsPage],
  [InventoryIntelligencePage.id, InventoryIntelligencePage],
  [WarehouseNetworkPage.id, WarehouseNetworkPage],
  [DecisionCenterPage.id, DecisionCenterPage],
  [DemandForecastPage.id, DemandForecastPage],
  [InventoryAnomalyDetectionPage.id, InventoryAnomalyDetectionPage],
  [SupplierIntelligencePage.id, SupplierIntelligencePage],
  [AuditTimelinePage.id, AuditTimelinePage],
  [ReportsPage.id, ReportsPage],
  [AICopilotPage.id, AICopilotPage],
  [AdministrationPage.id, AdministrationPage]
]);

const store = createStore({
  session: null,
  dashboardCredentials: null,
  loading: true,
  error: "",
  data: null,
  filters: {
    warehouse: "all",
    category: "all",
    risk: "all"
  },
  ui: {
    currentPage: "executive-dashboard",
    theme: "dark",
    notificationsOpen: false,
    search: "",
    networkFocus: ""
  },
  searchResults: [],
  copilotResponse: null
});

async function loadPlatformData() {
  const budget = store.getState().data?.dashboard?.budget || 2500;
  const [dashboard, inventory, analytics, decisionCenter, reports, warehouseHealth] = await Promise.all([
    PlatformService.getDashboard(budget),
    PlatformService.getInventory(),
    PlatformService.getAnalytics(),
    PlatformService.getDecisionCenter(),
    PlatformService.getReports(),
    PlatformService.getWarehouseHealth()
  ]);

  return normalizePlatformData({
    dashboard: dashboard.data || dashboard,
    inventory,
    analytics,
    decisionCenter,
    reports,
    warehouseHealth
  });
}

function applyWarehouseOverrides(data) {
  const overrides = JSON.parse(localStorage.getItem("smartinventory.warehouseOverrides") || "{}");
  if (!Object.keys(overrides).length) return data;
  return {
    ...data,
    warehouseHealth: data.warehouseHealth.map(warehouse => ({ ...warehouse, ...(overrides[warehouse.warehouseId] || {}) }))
  };
}

async function bootstrap() {
  store.setState({ loading: true, error: "" });
  try {
    const data = applyWarehouseOverrides(await loadPlatformData());
    const credentials = data.dashboard.credentials || null;
    store.setState({
      data,
      dashboardCredentials: credentials,
      loading: false,
      error: "",
      searchResults: buildGlobalSearchIndex(data)
    });
  } catch (error) {
    store.setState({
      loading: false,
      error: error.message || "Unable to load the Warehouse Intelligence Platform."
    });
  }
}

function getPagesForState(state) {
  const role = state.session?.role || "Viewer";
  return getAccessiblePages(role);
}

function renderLogin(credentials) {
  return `
    <section class="login-shell">
      <div class="login-stage">
        <div class="login-showcase">
          <span class="page-eyebrow">Final Year Major Project</span>
          <h1>Warehouse Intelligence Platform</h1>
          <p>A decision-support workspace for warehouse managers, operations teams, and executives with modular frontend architecture and REST API integration.</p>
          <div class="showcase-chip-row">
            <span class="chip">REST API Driven</span>
            <span class="chip">Multi-Warehouse Analytics</span>
            <span class="chip">Role-Based Views</span>
          </div>
          <div class="showcase-grid">
            <article class="showcase-card">
              <span class="panel-kicker">Project Scope</span>
              <strong>12 operational modules</strong>
              <p>Dashboards, network intelligence, anomaly detection, forecasting, AI copilot, and reporting in one frontend.</p>
            </article>
            <article class="showcase-card">
              <span class="panel-kicker">Presentation Focus</span>
              <strong>Enterprise-inspired UX</strong>
              <p>Built to look credible for an academic capstone while still feeling practical for warehouse operations.</p>
            </article>
          </div>
        </div>
        <div class="login-card">
          <span class="page-eyebrow">Operations Workspace</span>
          <h2>Sign in to continue</h2>
          <p>Enterprise views are enabled by role and all frontend data is sourced from REST services only.</p>
          <form id="loginForm" class="login-form">
            <label>Username<input id="username" autocomplete="username" placeholder="Enter username"></label>
            <label>Password<input id="password" type="password" autocomplete="current-password" placeholder="Enter password"></label>
            <button class="primary-action" type="submit">Sign In</button>
            <div class="suggestion-row">
              <button class="ghost-action" type="button" data-fill-role="warehouse_manager">Warehouse Manager</button>
              <button class="ghost-action" type="button" data-fill-role="admin">Executive Admin</button>
            </div>
          </form>
          <div class="login-hint">
            Demo usernames: ${credentials?.employee?.username || "warehouse_manager"} and ${credentials?.manager?.username || "admin"}
          </div>
        </div>
      </div>
    </section>
  `;
}

function renderApp(state) {
  if (state.loading) return renderLoadingState();
  if (state.error) return renderErrorState("Platform unavailable", state.error);
  if (!state.data) return renderEmptyState("No data returned", "The frontend did not receive warehouse intelligence data from the REST layer.");
  if (!state.session) return renderLogin(state.dashboardCredentials);

  const pages = getPagesForState(state);
  const currentPage = pages.find(page => page.id === state.ui.currentPage) || pages[0] || PAGE_DEFINITIONS[0];
  const pageModule = pageRegistry.get(currentPage.id);
  const filteredItems = filterItems(state.data.items, state.filters);
  const filterOptions = buildFilterOptions(state.data);
  const results = state.ui.search
    ? state.searchResults.filter(entry => `${entry.label} ${entry.meta} ${(entry.tags || []).join(" ")}`.toLowerCase().includes(state.ui.search.toLowerCase())).slice(0, 8)
    : [];

  return `
    <div class="platform-shell" data-theme="${state.ui.theme}">
      <aside class="sidebar">
        <div class="brand-lockup">
          <div class="brand-mark">WI</div>
          <div class="brand-copy">
            <span>Warehouse Intelligence</span>
            <b>Major Project Interface</b>
            <strong>${ROLE_LABELS[state.session.role] || state.session.role}</strong>
          </div>
        </div>
        ${renderNavigation(pages, currentPage.id)}
      </aside>
      <div class="workspace">
        <header class="topbar">
          <div>
            <span class="page-eyebrow">Signed in as ${state.session.username}</span>
            <h2>${currentPage.label}</h2>
          </div>
          <div class="topbar-actions">
            <label class="search-box">
              <input id="globalSearch" type="search" value="${state.ui.search}" placeholder="Search warehouses, products, recommendations">
            </label>
            <button class="icon-button" id="toggleNotifications" type="button">Alerts</button>
            <button class="icon-button" id="toggleTheme" type="button">${state.ui.theme === "dark" ? "Light" : "Dark"}</button>
            <button class="icon-button" id="refreshData" type="button">Refresh</button>
            <button class="icon-button" id="signOut" type="button">Sign Out</button>
          </div>
        </header>
        ${results.length ? `
          <div class="search-results">
            ${results.map(result => `
              <button class="search-result" type="button" data-page="${result.page}">
                <strong>${result.label}</strong>
                <span>${result.meta}</span>
              </button>
            `).join("")}
          </div>
        ` : ""}
        ${renderFilters(filterOptions, state.filters)}
        <main class="page-content">
          ${pageModule.render({
            data: state.data,
            filteredItems,
            session: state.session,
            ui: state.ui,
            store
          })}
        </main>
      </div>
      ${state.ui.notificationsOpen ? renderNotifications(state.data.alerts, state.data.recommendations) : ""}
    </div>
  `;
}

async function submitLogin(form) {
  const username = form.querySelector("#username").value.trim();
  const password = form.querySelector("#password").value.trim();
  const response = await PlatformService.login(username, password);
  const session = response.data || response;
  if (!session.authenticated) {
    throw new Error("Authentication failed.");
  }
  store.setState({
    session,
    ui: { ...store.getState().ui, currentPage: getAccessiblePages(session.role)[0]?.id || "executive-dashboard" }
  });
}

function wireEvents(container) {
  container.addEventListener("click", async event => {
    const pageButton = event.target.closest("[data-page]");
    if (pageButton) {
      store.setState({ ui: { ...store.getState().ui, currentPage: pageButton.dataset.page, search: "" } });
      return;
    }

    const fillRole = event.target.closest("[data-fill-role]");
    if (fillRole) {
      const role = fillRole.dataset.fillRole;
      const credentials = store.getState().dashboardCredentials;
      const username = role === "admin" ? credentials?.manager?.username : credentials?.employee?.username;
      container.querySelector("#username").value = username || "";
      container.querySelector("#password").focus();
      return;
    }

    const questionButton = event.target.closest("[data-copilot-question]");
    if (questionButton) {
      const field = container.querySelector("#copilotQuestion");
      if (field) field.value = questionButton.dataset.copilotQuestion;
      return;
    }

    const nodeButton = event.target.closest("[data-node]");
    if (nodeButton) {
      store.setState({ ui: { ...store.getState().ui, networkFocus: nodeButton.dataset.node } });
      return;
    }

    if (event.target.id === "toggleNotifications") {
      store.setState({ ui: { ...store.getState().ui, notificationsOpen: !store.getState().ui.notificationsOpen } });
      return;
    }

    if (event.target.id === "toggleTheme") {
      store.setState({ ui: { ...store.getState().ui, theme: store.getState().ui.theme === "dark" ? "light" : "dark" } });
      return;
    }

    if (event.target.id === "refreshData") {
      await bootstrap();
      return;
    }

    if (event.target.id === "signOut") {
      store.setState({ session: null, copilotResponse: null });
    }

    if (event.target.id === "resetWarehouseOverrides") {
      localStorage.removeItem("smartinventory.warehouseOverrides");
      await bootstrap();
    }
  });

  container.addEventListener("change", event => {
    if (event.target.id === "warehouseOverrideId") {
      const warehouse = store.getState().data.warehouseHealth.find(entry => String(entry.warehouseId) === event.target.value);
      if (!warehouse) return;
      container.querySelector("#warehouseOverrideName").value = warehouse.warehouseName;
      container.querySelector("#warehouseOverrideHealth").value = warehouse.healthScore;
      container.querySelector("#warehouseOverrideAccuracy").value = warehouse.inventoryAccuracy;
      container.querySelector("#warehouseOverrideCapacity").value = warehouse.capacityUtilization;
      container.querySelector("#warehouseOverrideSupplier").value = warehouse.supplierPerformance;
      return;
    }
    const filter = event.target.closest("[data-filter]");
    if (!filter) return;
    store.setState({
      filters: {
        ...store.getState().filters,
        [filter.dataset.filter]: filter.value
      }
    });
  });

  container.addEventListener("input", event => {
    if (event.target.id === "globalSearch") {
      store.setState({ ui: { ...store.getState().ui, search: event.target.value } });
    }
  });

  container.addEventListener("submit", async event => {
    event.preventDefault();
    if (event.target.id === "loginForm") {
      try {
        await submitLogin(event.target);
      } catch (error) {
        store.setState({ error: error.message });
        setTimeout(() => store.setState({ error: "" }), 2500);
      }
      return;
    }

    if (event.target.id === "copilotForm") {
      const question = event.target.querySelector("#copilotQuestion").value.trim();
      if (!question) return;
      try {
        const response = await PlatformService.askCopilot(question);
        store.setState({ copilotResponse: response.data || response });
      } catch (error) {
        store.setState({ copilotResponse: { question, answer: error.message, evidence: [], sourceOfTruth: "REST service", status: "error" } });
      }
    }

    if (event.target.id === "warehouseOverrideForm") {
      const state = store.getState();
      if (state.session?.role !== "Admin") return;
      const warehouseId = Number(event.target.querySelector("#warehouseOverrideId").value);
      const updatedWarehouse = {
        warehouseName: event.target.querySelector("#warehouseOverrideName").value.trim(),
        healthScore: Number(event.target.querySelector("#warehouseOverrideHealth").value),
        inventoryAccuracy: Number(event.target.querySelector("#warehouseOverrideAccuracy").value),
        capacityUtilization: Number(event.target.querySelector("#warehouseOverrideCapacity").value),
        supplierPerformance: Number(event.target.querySelector("#warehouseOverrideSupplier").value)
      };
      const overrides = JSON.parse(localStorage.getItem("smartinventory.warehouseOverrides") || "{}");
      overrides[warehouseId] = updatedWarehouse;
      localStorage.setItem("smartinventory.warehouseOverrides", JSON.stringify(overrides));
      store.setState({
        data: {
          ...state.data,
          warehouseHealth: state.data.warehouseHealth.map(warehouse => warehouse.warehouseId === warehouseId ? { ...warehouse, ...updatedWarehouse } : warehouse)
        }
      });
    }
  });
}

export function createAppShell(container) {
  store.subscribe(state => {
    document.body.dataset.theme = state.ui?.theme || "dark";
    container.innerHTML = renderApp(state);
  });
  wireEvents(container);
  bootstrap();
}
