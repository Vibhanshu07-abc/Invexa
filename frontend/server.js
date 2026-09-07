const http = require("http");
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");
const net = require("net");
const { URL } = require("url");

function envOrDefault(name, fallback) {
  const value = process.env[name];
  return value === undefined || value === "" ? fallback : value;
}

function envToBool(name, fallback = false) {
  const value = process.env[name];
  if (value === undefined || value === "") return fallback;
  return ["1", "true", "yes", "on"].includes(String(value).toLowerCase());
}

const ROOT = envOrDefault("SMARTINVENTORY_FRONTEND_ROOT", __dirname);
const BACKEND_ROOT = envOrDefault("SMARTINVENTORY_BACKEND_ROOT", path.resolve(ROOT, "..", "backend"));
const CONFIG_PATH = envOrDefault("SMARTINVENTORY_CONFIG_PATH", path.join(BACKEND_ROOT, "config", "config.json"));
const RESULT_PATH = envOrDefault("SMARTINVENTORY_RESULT_PATH", path.join(BACKEND_ROOT, "output", "result.json"));
const MONITORING_PATH = envOrDefault("SMARTINVENTORY_MONITORING_PATH", path.join(path.dirname(RESULT_PATH), "monitoring.json"));
const PORT = Number(envOrDefault("PORT", "8080"));
const APP_ENV = envOrDefault("NODE_ENV", "development");
const APP_VERSION = envOrDefault("SMARTINVENTORY_VERSION", "dev");
const READINESS_PROBE_TIMEOUT_MS = Number(envOrDefault("SMARTINVENTORY_DB_PROBE_TIMEOUT_MS", "1500"));
const READINESS_PROBE_CACHE_MS = Number(envOrDefault("SMARTINVENTORY_DB_PROBE_CACHE_MS", "10000"));
const LATENCY_BUCKETS = [0.05, 0.1, 0.25, 0.5, 1, 2, 5];

const MIME_TYPES = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".json": "application/json; charset=utf-8"
};

const metrics = {
  startedAt: Date.now(),
  requestsTotal: 0,
  readinessFailures: 0,
  routeCounts: new Map(),
  routeStatusCounts: new Map(),
  routeDuration: new Map(),
  authCounts: new Map([
    ["success", 0],
    ["failure", 0]
  ]),
  lastReady: false,
  lastReadyReason: "Server starting",
  lastDatabaseProbe: null
};

let databaseProbeCache = {
  expiresAt: 0,
  snapshot: {
    backend: "csv",
    available: true,
    latencySeconds: 0,
    reason: "Database probe pending"
  }
};

function log(level, event, fields = {}) {
  const payload = {
    timestamp: new Date().toISOString(),
    level,
    event,
    service: "smartinventory-frontend",
    environment: APP_ENV,
    ...fields
  };
  console.log(JSON.stringify(payload));
}

function readJson(filePath, fallback) {
  try {
    return JSON.parse(fs.readFileSync(filePath, "utf8"));
  } catch (error) {
    return fallback;
  }
}

function sha256(value) {
  return crypto.createHash("sha256").update(value).digest("hex");
}

function normalizeConfig(raw) {
  return {
    repositoryBackend: envOrDefault("SMARTINVENTORY_REPOSITORY_BACKEND", raw.repositoryBackend || "csv"),
    postgresHost: envOrDefault("SMARTINVENTORY_POSTGRES_HOST", raw.postgresHost || "localhost"),
    postgresPort: Number(envOrDefault("SMARTINVENTORY_POSTGRES_PORT", String(raw.postgresPort || 5432))),
    employeeUser: {
      username: envOrDefault("SMARTINVENTORY_EMPLOYEE_USERNAME", raw.employeeUsername || raw.warehouseManagerUsername || "warehouse_manager"),
      passwordHash: envOrDefault("SMARTINVENTORY_EMPLOYEE_PASSWORD_HASH", raw.employeePasswordHash || raw.warehouseManagerPasswordHash || ""),
      role: envOrDefault("SMARTINVENTORY_EMPLOYEE_ROLE", raw.employeeRole || raw.warehouseManagerRole || "Warehouse Manager")
    },
    managerUser: {
      username: envOrDefault("SMARTINVENTORY_MANAGER_USERNAME", raw.managerUsername || raw.adminUsername || "admin"),
      passwordHash: envOrDefault("SMARTINVENTORY_MANAGER_PASSWORD_HASH", raw.managerPasswordHash || raw.adminPasswordHash || ""),
      role: envOrDefault("SMARTINVENTORY_MANAGER_ROLE", raw.managerRole || raw.adminRole || "Admin")
    },
    accounts: [
      {
        username: envOrDefault("SMARTINVENTORY_ADMIN_USERNAME", raw.adminUsername || "admin"),
        passwordHash: envOrDefault("SMARTINVENTORY_ADMIN_PASSWORD_HASH", raw.adminPasswordHash || ""),
        role: envOrDefault("SMARTINVENTORY_ADMIN_ROLE", raw.adminRole || "Admin")
      },
      {
        username: envOrDefault("SMARTINVENTORY_WAREHOUSE_MANAGER_USERNAME", raw.warehouseManagerUsername || "warehouse_manager"),
        passwordHash: envOrDefault("SMARTINVENTORY_WAREHOUSE_MANAGER_PASSWORD_HASH", raw.warehouseManagerPasswordHash || ""),
        role: envOrDefault("SMARTINVENTORY_WAREHOUSE_MANAGER_ROLE", raw.warehouseManagerRole || "Warehouse Manager")
      },
      {
        username: envOrDefault("SMARTINVENTORY_ANALYST_USERNAME", raw.analystUsername || "analyst"),
        passwordHash: envOrDefault("SMARTINVENTORY_ANALYST_PASSWORD_HASH", raw.analystPasswordHash || ""),
        role: envOrDefault("SMARTINVENTORY_ANALYST_ROLE", raw.analystRole || "Analyst")
      },
      {
        username: envOrDefault("SMARTINVENTORY_VIEWER_USERNAME", raw.viewerUsername || "viewer"),
        passwordHash: envOrDefault("SMARTINVENTORY_VIEWER_PASSWORD_HASH", raw.viewerPasswordHash || ""),
        role: envOrDefault("SMARTINVENTORY_VIEWER_ROLE", raw.viewerRole || "Viewer")
      }
    ]
  };
}

function readDashboardPayload(config) {
  const payload = readJson(RESULT_PATH, null);
  if (!payload) {
    return {
      generatedAt: "",
      mode: "Backend Output Missing",
      budget: 0,
      warehouses: [],
      credentials: {
        employee: { username: config.employeeUser.username, password: "", role: config.employeeUser.role },
        manager: { username: config.managerUser.username, password: "", role: config.managerUser.role }
      },
      items: [],
      audits: [],
      mismatches: [],
      misplacedItems: [],
      topKItems: [],
      classification: [],
      optimization: { budget: 0, budgetUsed: 0, totalValue: 0, selectedItems: [], status: "Run the backend once to generate dashboard data." },
      clusters: [],
      theftTiming: [],
      decisionCenter: { recommendationCount: 0, recommendations: [] },
      ai: { recommendationExplanations: [], executiveSummary: { summary: "" }, demandForecasts: [] },
      actionReport: [],
      summary: {
        totalItems: 0,
        lostItems: 0,
        misplacedItems: 0,
        highRiskItems: 0,
        estimatedFinancialLoss: 0,
        recommendedActions: []
      },
      alerts: ["Run the backend once to generate dashboard data."]
    };
  }

  return {
    ...payload,
    credentials: {
      employee: { username: config.employeeUser.username, password: "", role: config.employeeUser.role },
      manager: { username: config.managerUser.username, password: "", role: config.managerUser.role }
    }
  };
}

function readMonitoringState() {
  return readJson(MONITORING_PATH, {
    updatedAt: "",
    inventoryCount: 0,
    warehouseCount: 0,
    analysisExecutionMs: 0,
    decisionEngineExecutionMs: 0,
    decisionEngineRecommendationCount: 0,
    decisionEngineFailuresTotal: 0,
    forecastExecutionMs: 0,
    forecastCount: 0,
    forecastFailuresTotal: 0
  });
}

function buildWarehouseHealth(dashboard) {
  const items = Array.isArray(dashboard.items) ? dashboard.items : [];
  const grouped = items.reduce((map, item) => {
    const warehouseId = item.warehouseId || 1;
    const warehouseName = item.warehouseName || `Warehouse ${warehouseId}`;
    if (!map.has(warehouseId)) {
      map.set(warehouseId, {
        warehouseId,
        warehouseName,
        items: []
      });
    }
    map.get(warehouseId).items.push(item);
    return map;
  }, new Map());

  return [...grouped.values()].map(group => {
    const totalExpected = group.items.reduce((sum, item) => sum + Number(item.expected || 0), 0);
    const totalActual = group.items.reduce((sum, item) => sum + Number(item.actual || 0), 0);
    const mismatchCount = group.items.filter(item => Number(item.mismatch || 0) !== 0).length;
    const misplacedCount = group.items.filter(item => item.currentLocation && item.expectedLocation && item.currentLocation !== item.expectedLocation).length;
    const averageRisk = group.items.length
      ? group.items.reduce((sum, item) => sum + Number(item.riskScore || 0), 0) / group.items.length
      : 0;
    const inventoryAccuracy = totalExpected > 0 ? Math.max(0, Math.min(100, (totalActual / totalExpected) * 100)) : 100;
    const capacityUtilization = Math.max(35, Math.min(98, totalExpected > 0 ? (totalActual / totalExpected) * 92 : 60));
    const supplierPerformance = Math.max(55, Math.min(99, 100 - averageRisk * 0.45 - misplacedCount * 3));
    const healthScore = Math.max(0, Math.min(100, inventoryAccuracy * 0.4 + (100 - averageRisk) * 0.25 + capacityUtilization * 0.2 + supplierPerformance * 0.15));
    return {
      warehouseId: group.warehouseId,
      warehouseName: group.warehouseName,
      healthScore: Number(healthScore.toFixed(2)),
      status: healthScore < 70 ? "Needs attention" : healthScore < 85 ? "Watch" : "Stable",
      inventoryAccuracy: Number(inventoryAccuracy.toFixed(2)),
      averageRisk: Number(averageRisk.toFixed(2)),
      capacityUtilization: Number(capacityUtilization.toFixed(2)),
      supplierPerformance: Number(supplierPerformance.toFixed(2)),
      mismatchCount
    };
  });
}

function buildCopilotResponse(question, dashboard) {
  const normalized = String(question || "").trim();
  const lower = normalized.toLowerCase();
  const warehouseHealth = buildWarehouseHealth(dashboard);
  const weakest = [...warehouseHealth].sort((a, b) => a.healthScore - b.healthScore)[0];
  const recommendations = dashboard.decisionCenter?.recommendations || [];
  const anomalies = dashboard.theftTiming || [];
  const forecasts = dashboard.ai?.demandForecasts || [];

  if (lower.includes("which warehouse needs attention")) {
    return {
      question: normalized,
      answer: weakest
        ? `${weakest.warehouseName} needs attention with a health score of ${weakest.healthScore}%.`
        : "No warehouse health data is currently available.",
      evidence: weakest ? [
        `Inventory accuracy ${weakest.inventoryAccuracy}%`,
        `Capacity utilization ${weakest.capacityUtilization}%`,
        `Supplier performance ${weakest.supplierPerformance}%`
      ] : [],
      sourceOfTruth: "Warehouse health REST payload",
      status: "supported"
    };
  }

  if (lower.includes("why is warehouse health low")) {
    return {
      question: normalized,
      answer: weakest
        ? `${weakest.warehouseName} is under pressure because inventory accuracy, capacity utilization, and supplier performance are lagging target levels.`
        : "Warehouse health cannot be explained because no warehouse health data is available.",
      evidence: weakest ? [
        `Health score ${weakest.healthScore}%`,
        `Mismatch count ${weakest.mismatchCount}`,
        `Average risk ${weakest.averageRisk}%`
      ] : [],
      sourceOfTruth: "Warehouse health REST payload",
      status: "supported"
    };
  }

  if (lower.includes("priority anomalies")) {
    return {
      question: normalized,
      answer: anomalies.length
        ? `There are ${anomalies.length} anomaly records that require review, led by repeated mismatch windows and location drift.`
        : "No active anomaly records are currently available.",
      evidence: anomalies.slice(0, 3).map(entry => entry.message),
      sourceOfTruth: "Analytics REST payload",
      status: "supported"
    };
  }

  if (lower.includes("decision center")) {
    return {
      question: normalized,
      answer: recommendations.length
        ? `The decision center currently highlights ${recommendations.length} recommendations across restocking, warehouse network actions, and audit follow-up.`
        : "No decision center recommendations are currently available.",
      evidence: recommendations.slice(0, 3).map(entry => entry.title),
      sourceOfTruth: "Decision center REST payload",
      status: "supported"
    };
  }

  if (lower.includes("shortages")) {
    return {
      question: normalized,
      answer: forecasts.length
        ? "Predicted shortages are concentrated in products with projected demand rising above current demand."
        : "No forecast data is currently available for shortage prediction.",
      evidence: forecasts.slice(0, 5).map(entry => `${entry.name}: ${entry.currentDemand} -> ${entry.projectedDemand} (${entry.confidence})`),
      sourceOfTruth: "AI forecast REST payload",
      status: "supported"
    };
  }

  return {
    question: normalized,
    answer: "Supported questions include warehouse attention, warehouse health, priority anomalies, decision center summary, and predicted shortages.",
    evidence: [],
    sourceOfTruth: "Copilot REST gateway",
    status: "unsupported"
  };
}

function sendJson(res, statusCode, body) {
  res.writeHead(statusCode, { "Content-Type": "application/json; charset=utf-8" });
  res.end(`${JSON.stringify(body, null, 2)}\n`);
}

function sendPrometheus(res, body) {
  res.writeHead(200, { "Content-Type": "text/plain; version=0.0.4; charset=utf-8" });
  res.end(body);
}

function parseBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    req.on("data", chunk => chunks.push(chunk));
    req.on("end", () => {
      const text = Buffer.concat(chunks).toString("utf8");
      if (!text) {
        resolve({});
        return;
      }

      try {
        resolve(JSON.parse(text));
      } catch (error) {
        reject(new Error("Invalid JSON body."));
      }
    });
    req.on("error", reject);
  });
}

function serveStatic(req, res) {
  const requestUrl = new URL(req.url, "http://localhost");
  let filePath = decodeURIComponent(requestUrl.pathname);
  if (filePath === "/") filePath = "/index.html";

  const fullPath = path.normalize(path.join(ROOT, filePath));
  if (!fullPath.startsWith(ROOT)) {
    res.writeHead(403);
    res.end("Forbidden");
    return;
  }

  fs.readFile(fullPath, (error, content) => {
    if (error) {
      res.writeHead(404);
      res.end("Not found");
      return;
    }

    const ext = path.extname(fullPath).toLowerCase();
    res.writeHead(200, { "Content-Type": MIME_TYPES[ext] || "application/octet-stream" });
    res.end(content);
  });
}

function checkTcpConnection(host, port, timeoutMs) {
  return new Promise(resolve => {
    const startedAt = process.hrtime.bigint();
    const socket = net.connect({ host, port });
    const finalize = (ready, reason) => {
      socket.removeAllListeners();
      socket.destroy();
      const elapsedSeconds = Number(process.hrtime.bigint() - startedAt) / 1e9;
      resolve({ ready, reason, latencySeconds: elapsedSeconds });
    };

    socket.setTimeout(timeoutMs);
    socket.on("connect", () => finalize(true, "TCP connection established"));
    socket.on("timeout", () => finalize(false, "TCP connection timed out"));
    socket.on("error", error => finalize(false, error.message));
  });
}

async function getDatabaseProbe(config) {
  const backend = String(config.repositoryBackend || "csv").toLowerCase();
  if (!backend.startsWith("postgres")) {
    const snapshot = {
      backend,
      available: 1,
      latencySeconds: 0,
      reason: "Database probe not required for non-PostgreSQL backend"
    };
    metrics.lastDatabaseProbe = snapshot;
    return snapshot;
  }

  const now = Date.now();
  if (databaseProbeCache.expiresAt > now) {
    metrics.lastDatabaseProbe = databaseProbeCache.snapshot;
    return databaseProbeCache.snapshot;
  }

  const probe = await checkTcpConnection(config.postgresHost, config.postgresPort, READINESS_PROBE_TIMEOUT_MS);
  const snapshot = {
    backend,
    available: probe.ready ? 1 : 0,
    latencySeconds: probe.latencySeconds,
    reason: probe.reason
  };

  databaseProbeCache = {
    expiresAt: now + READINESS_PROBE_CACHE_MS,
    snapshot
  };
  metrics.lastDatabaseProbe = snapshot;
  return snapshot;
}

async function computeReadiness(config) {
  if (!fs.existsSync(CONFIG_PATH)) {
    return { ready: false, reason: `Config file missing at ${CONFIG_PATH}` };
  }

  if (!fs.existsSync(RESULT_PATH)) {
    return { ready: false, reason: `Dashboard payload missing at ${RESULT_PATH}` };
  }

  const dashboard = readDashboardPayload(config);
  if (!dashboard.generatedAt) {
    return { ready: false, reason: "Dashboard payload has not been generated yet." };
  }

  const databaseProbe = await getDatabaseProbe(config);
  if (databaseProbe.available !== 1) {
    return { ready: false, reason: databaseProbe.reason };
  }

  return { ready: true, reason: "Dashboard payload and configuration loaded." };
}

function incrementMapCount(map, key) {
  map.set(key, (map.get(key) || 0) + 1);
}

function normalizeRoute(pathname) {
  if (pathname.startsWith("/api/")) return pathname;
  if (pathname === "/health" || pathname === "/ready" || pathname === "/metrics") return pathname;
  return "static";
}

function observeRouteDuration(route, durationSeconds) {
  if (!metrics.routeDuration.has(route)) {
    metrics.routeDuration.set(route, {
      count: 0,
      sum: 0,
      buckets: LATENCY_BUCKETS.map(() => 0),
      inf: 0
    });
  }

  const routeMetric = metrics.routeDuration.get(route);
  routeMetric.count += 1;
  routeMetric.sum += durationSeconds;

  let matched = false;
  LATENCY_BUCKETS.forEach((bucket, index) => {
    if (durationSeconds <= bucket) {
      routeMetric.buckets[index] += 1;
      matched = true;
    }
  });
  if (!matched) {
    routeMetric.inf += 1;
  }
}

function deriveForecastAccuracy(dashboard) {
  const forecasts = Array.isArray(dashboard.ai?.demandForecasts) ? dashboard.ai.demandForecasts : [];
  if (!forecasts.length) return 0;

  const meanAbsolutePercentageError = forecasts.reduce((sum, forecast) => {
    const current = Math.max(1, Number(forecast.currentDemand || 0));
    const projected = Number(forecast.projectedDemand || 0);
    return sum + Math.min(2, Math.abs(projected - current) / current);
  }, 0) / forecasts.length;

  return Math.max(0, Math.min(100, (1 - meanAbsolutePercentageError) * 100));
}

function escapeLabelValue(value) {
  return String(value)
    .replace(/\\/g, "\\\\")
    .replace(/\n/g, "\\n")
    .replace(/"/g, '\\"');
}

function metricLine(name, labels, value) {
  const labelEntries = Object.entries(labels || {});
  if (!labelEntries.length) {
    return `${name} ${value}`;
  }

  const renderedLabels = labelEntries
    .map(([key, labelValue]) => `${key}="${escapeLabelValue(labelValue)}"`)
    .join(",");
  return `${name}{${renderedLabels}} ${value}`;
}

async function renderMetrics(config) {
  const dashboard = readDashboardPayload(config);
  const monitoring = readMonitoringState();
  const readiness = await computeReadiness(config);
  const databaseProbe = await getDatabaseProbe(config);
  const uptimeSeconds = Math.floor((Date.now() - metrics.startedAt) / 1000);
  const warehouseHealth = buildWarehouseHealth(dashboard);
  const authSuccess = metrics.authCounts.get("success") || 0;
  const authFailure = metrics.authCounts.get("failure") || 0;
  const forecastAccuracy = deriveForecastAccuracy(dashboard);

  metrics.lastReady = readiness.ready;
  metrics.lastReadyReason = readiness.reason;

  const lines = [
    "# HELP smartinventory_process_uptime_seconds Process uptime in seconds.",
    "# TYPE smartinventory_process_uptime_seconds counter",
    metricLine("smartinventory_process_uptime_seconds", {}, uptimeSeconds),
    "# HELP smartinventory_http_requests_total Total HTTP requests served.",
    "# TYPE smartinventory_http_requests_total counter",
    metricLine("smartinventory_http_requests_total", {}, metrics.requestsTotal),
    "# HELP smartinventory_http_requests_by_route_total HTTP requests by route.",
    "# TYPE smartinventory_http_requests_by_route_total counter",
    "# HELP smartinventory_http_responses_total HTTP responses by route and status.",
    "# TYPE smartinventory_http_responses_total counter",
    "# HELP smartinventory_http_request_duration_seconds HTTP request latency by route.",
    "# TYPE smartinventory_http_request_duration_seconds histogram",
    "# HELP smartinventory_readiness_state Current readiness state (1 ready, 0 not ready).",
    "# TYPE smartinventory_readiness_state gauge",
    metricLine("smartinventory_readiness_state", {}, readiness.ready ? 1 : 0),
    "# HELP smartinventory_readiness_failures_total Total readiness failures.",
    "# TYPE smartinventory_readiness_failures_total counter",
    metricLine("smartinventory_readiness_failures_total", {}, metrics.readinessFailures),
    "# HELP smartinventory_health_endpoint_state Health endpoint state.",
    "# TYPE smartinventory_health_endpoint_state gauge",
    metricLine("smartinventory_health_endpoint_state", { endpoint: "health" }, 1),
    metricLine("smartinventory_health_endpoint_state", { endpoint: "ready" }, readiness.ready ? 1 : 0),
    "# HELP smartinventory_build_info Build metadata.",
    "# TYPE smartinventory_build_info gauge",
    metricLine("smartinventory_build_info", { version: APP_VERSION, environment: APP_ENV }, 1),
    "# HELP smartinventory_inventory_total Inventory item count.",
    "# TYPE smartinventory_inventory_total gauge",
    metricLine("smartinventory_inventory_total", {}, Number(monitoring.inventoryCount || dashboard.items?.length || 0)),
    "# HELP smartinventory_warehouse_total Warehouse count.",
    "# TYPE smartinventory_warehouse_total gauge",
    metricLine("smartinventory_warehouse_total", {}, Number(monitoring.warehouseCount || warehouseHealth.length || dashboard.warehouses?.length || 0)),
    "# HELP smartinventory_decision_engine_execution_seconds Last decision engine execution time.",
    "# TYPE smartinventory_decision_engine_execution_seconds gauge",
    metricLine("smartinventory_decision_engine_execution_seconds", {}, Number(monitoring.decisionEngineExecutionMs || 0) / 1000),
    "# HELP smartinventory_decision_engine_recommendations_total Last recommendation count generated by the decision engine.",
    "# TYPE smartinventory_decision_engine_recommendations_total gauge",
    metricLine("smartinventory_decision_engine_recommendations_total", {}, Number(monitoring.decisionEngineRecommendationCount || dashboard.decisionCenter?.recommendationCount || 0)),
    "# HELP smartinventory_decision_engine_failures_total Total decision engine failures.",
    "# TYPE smartinventory_decision_engine_failures_total counter",
    metricLine("smartinventory_decision_engine_failures_total", {}, Number(monitoring.decisionEngineFailuresTotal || 0)),
    "# HELP smartinventory_forecast_execution_seconds Last forecast generation execution time.",
    "# TYPE smartinventory_forecast_execution_seconds gauge",
    metricLine("smartinventory_forecast_execution_seconds", {}, Number(monitoring.forecastExecutionMs || 0) / 1000),
    "# HELP smartinventory_forecast_total Total forecast entries in the latest payload.",
    "# TYPE smartinventory_forecast_total gauge",
    metricLine("smartinventory_forecast_total", {}, Number(monitoring.forecastCount || dashboard.ai?.demandForecasts?.length || 0)),
    "# HELP smartinventory_forecast_failures_total Total forecast generation failures.",
    "# TYPE smartinventory_forecast_failures_total counter",
    metricLine("smartinventory_forecast_failures_total", {}, Number(monitoring.forecastFailuresTotal || 0)),
    "# HELP smartinventory_forecast_accuracy_percent Derived forecast accuracy score.",
    "# TYPE smartinventory_forecast_accuracy_percent gauge",
    metricLine("smartinventory_forecast_accuracy_percent", {}, Number(forecastAccuracy.toFixed(2))),
    "# HELP smartinventory_analysis_execution_seconds Last analysis execution time.",
    "# TYPE smartinventory_analysis_execution_seconds gauge",
    metricLine("smartinventory_analysis_execution_seconds", {}, Number(monitoring.analysisExecutionMs || 0) / 1000),
    "# HELP smartinventory_database_available Database availability state.",
    "# TYPE smartinventory_database_available gauge",
    metricLine("smartinventory_database_available", { backend: databaseProbe.backend }, databaseProbe.available),
    "# HELP smartinventory_database_latency_seconds Database connectivity latency.",
    "# TYPE smartinventory_database_latency_seconds gauge",
    metricLine("smartinventory_database_latency_seconds", { backend: databaseProbe.backend }, databaseProbe.latencySeconds),
    "# HELP smartinventory_auth_attempts_total Authentication attempts by result.",
    "# TYPE smartinventory_auth_attempts_total counter",
    metricLine("smartinventory_auth_attempts_total", { result: "success" }, authSuccess),
    metricLine("smartinventory_auth_attempts_total", { result: "failure" }, authFailure),
    "# HELP smartinventory_inventory_mismatches_total Inventory mismatch count.",
    "# TYPE smartinventory_inventory_mismatches_total gauge",
    metricLine("smartinventory_inventory_mismatches_total", {}, Array.isArray(dashboard.mismatches) ? dashboard.mismatches.length : 0),
    "# HELP smartinventory_inventory_misplaced_total Misplaced inventory count.",
    "# TYPE smartinventory_inventory_misplaced_total gauge",
    metricLine("smartinventory_inventory_misplaced_total", {}, Array.isArray(dashboard.misplacedItems) ? dashboard.misplacedItems.length : 0),
    "# HELP smartinventory_inventory_loss_value Estimated financial loss.",
    "# TYPE smartinventory_inventory_loss_value gauge",
    metricLine("smartinventory_inventory_loss_value", {}, Number(dashboard.summary?.estimatedFinancialLoss || 0)),
    "# HELP smartinventory_warehouse_health_score Warehouse health score.",
    "# TYPE smartinventory_warehouse_health_score gauge",
    "# HELP smartinventory_warehouse_inventory_accuracy_percent Warehouse inventory accuracy.",
    "# TYPE smartinventory_warehouse_inventory_accuracy_percent gauge",
    "# HELP smartinventory_warehouse_average_risk Warehouse average risk score.",
    "# TYPE smartinventory_warehouse_average_risk gauge",
    "# HELP smartinventory_warehouse_capacity_utilization_percent Warehouse capacity utilization.",
    "# TYPE smartinventory_warehouse_capacity_utilization_percent gauge"
  ];

  for (const [route, count] of metrics.routeCounts.entries()) {
    lines.push(metricLine("smartinventory_http_requests_by_route_total", { route }, count));
  }

  for (const [key, count] of metrics.routeStatusCounts.entries()) {
    const [route, status] = key.split("::");
    lines.push(metricLine("smartinventory_http_responses_total", { route, status }, count));
  }

  for (const [route, durationMetric] of metrics.routeDuration.entries()) {
    let runningCount = 0;
    LATENCY_BUCKETS.forEach((bucket, index) => {
      runningCount += durationMetric.buckets[index];
      lines.push(metricLine("smartinventory_http_request_duration_seconds_bucket", { route, le: String(bucket) }, runningCount));
    });
    lines.push(metricLine("smartinventory_http_request_duration_seconds_bucket", { route, le: "+Inf" }, durationMetric.count));
    lines.push(metricLine("smartinventory_http_request_duration_seconds_sum", { route }, Number(durationMetric.sum.toFixed(6))));
    lines.push(metricLine("smartinventory_http_request_duration_seconds_count", { route }, durationMetric.count));
  }

  for (const warehouse of warehouseHealth) {
    const labels = {
      warehouse_id: String(warehouse.warehouseId),
      warehouse_name: warehouse.warehouseName,
      status: warehouse.status
    };
    lines.push(metricLine("smartinventory_warehouse_health_score", labels, warehouse.healthScore));
    lines.push(metricLine("smartinventory_warehouse_inventory_accuracy_percent", labels, warehouse.inventoryAccuracy));
    lines.push(metricLine("smartinventory_warehouse_average_risk", labels, warehouse.averageRisk));
    lines.push(metricLine("smartinventory_warehouse_capacity_utilization_percent", labels, warehouse.capacityUtilization));
  }

  lines.push("");
  return lines.join("\n");
}

async function handleApi(req, res, url, config) {
  const dashboard = readDashboardPayload(config);

  if (req.method === "GET" && url.pathname === "/health") {
    return sendJson(res, 200, {
      status: "ok",
      service: "smartinventory-frontend",
      environment: APP_ENV,
      uptimeSeconds: Math.floor((Date.now() - metrics.startedAt) / 1000),
      version: APP_VERSION
    });
  }

  if (req.method === "GET" && url.pathname === "/ready") {
    const readiness = await computeReadiness(config);
    metrics.lastReady = readiness.ready;
    metrics.lastReadyReason = readiness.reason;
    if (!readiness.ready) {
      metrics.readinessFailures += 1;
    }

    return sendJson(res, readiness.ready ? 200 : 503, {
      status: readiness.ready ? "ready" : "not_ready",
      reason: readiness.reason,
      repositoryBackend: config.repositoryBackend
    });
  }

  if (req.method === "GET" && url.pathname === "/metrics") {
    return sendPrometheus(res, await renderMetrics(config));
  }

  if (req.method === "GET" && url.pathname === "/api/dashboard") {
    return sendJson(res, 200, { statusCode: 200, message: "Dashboard payload loaded.", data: dashboard });
  }

  if (req.method === "GET" && url.pathname === "/api/inventory") {
    return sendJson(res, 200, {
      statusCode: 200,
      message: "Inventory loaded.",
      data: { warehouses: dashboard.warehouses || [], items: dashboard.items || [] }
    });
  }

  if (req.method === "GET" && url.pathname === "/api/analytics") {
    return sendJson(res, 200, {
      statusCode: 200,
      message: "Analytics loaded.",
      data: {
        mismatches: dashboard.mismatches || [],
        misplacedItems: dashboard.misplacedItems || [],
        topRiskItems: dashboard.topKItems || [],
        classification: dashboard.classification || [],
        optimization: dashboard.optimization || {},
        clusters: dashboard.clusters || [],
        theftTiming: dashboard.theftTiming || []
      }
    });
  }

  if (req.method === "GET" && url.pathname === "/api/decision-center") {
    return sendJson(res, 200, {
      statusCode: 200,
      message: "Decision Center loaded.",
      data: dashboard.decisionCenter || { recommendationCount: 0, recommendations: [] }
    });
  }

  if (req.method === "GET" && url.pathname === "/api/warehouse-health") {
    return sendJson(res, 200, {
      statusCode: 200,
      message: "Warehouse health loaded.",
      data: {
        warehouses: buildWarehouseHealth(dashboard)
      }
    });
  }

  if (req.method === "POST" && url.pathname === "/api/copilot") {
    const body = await parseBody(req);
    const data = buildCopilotResponse(body.question, dashboard);
    return sendJson(res, data.status === "unsupported" ? 400 : 200, {
      statusCode: data.status === "unsupported" ? 400 : 200,
      message: "Copilot response generated.",
      data
    });
  }

  if (req.method === "GET" && url.pathname === "/api/reports") {
    return sendJson(res, 200, {
      statusCode: 200,
      message: "Reports loaded.",
      data: {
        inventory: { warehouses: dashboard.warehouses || [], items: dashboard.items || [] },
        analytics: {
          mismatches: dashboard.mismatches || [],
          misplacedItems: dashboard.misplacedItems || [],
          topRiskItems: dashboard.topKItems || [],
          classification: dashboard.classification || [],
          optimization: dashboard.optimization || {},
          clusters: dashboard.clusters || [],
          theftTiming: dashboard.theftTiming || []
        },
        decisionCenter: dashboard.decisionCenter || { recommendationCount: 0, recommendations: [] },
        summary: { summary: dashboard.summary || {}, alerts: dashboard.alerts || [] }
      }
    });
  }

  if (req.method === "POST" && url.pathname === "/api/auth/login") {
    const body = await parseBody(req);
    const account = config.accounts.find(user => user.username === body.username);
    const authenticated = Boolean(account) && sha256(String(body.password || "")) === account.passwordHash;
    incrementMapCount(metrics.authCounts, authenticated ? "success" : "failure");

    return sendJson(res, authenticated ? 200 : 401, {
      statusCode: authenticated ? 200 : 401,
      message: authenticated ? "Authentication successful." : "Authentication failed.",
      data: {
        username: authenticated ? account.username : "",
        role: authenticated ? account.role : "",
        authenticated
      }
    });
  }

  if (req.method !== "GET" && (
    url.pathname.startsWith("/api/inventory/") ||
    url.pathname.startsWith("/api/analytics/")
  )) {
    return sendJson(res, 501, {
      statusCode: 501,
      message: "Mutation routes must be handled by the backend service layer.",
      data: null
    });
  }

  return sendJson(res, 404, { statusCode: 404, message: "API route not found.", data: null });
}

function createServer(config) {
  return http.createServer((req, res) => {
    const requestStart = process.hrtime.bigint();
    const url = new URL(req.url, "http://localhost");
    const route = normalizeRoute(url.pathname);

    res.on("finish", () => {
      const durationSeconds = Number(process.hrtime.bigint() - requestStart) / 1e9;
      metrics.requestsTotal += 1;
      incrementMapCount(metrics.routeCounts, route);
      incrementMapCount(metrics.routeStatusCounts, `${route}::${res.statusCode}`);
      observeRouteDuration(route, durationSeconds);
      log("info", "http_request_completed", {
        method: req.method,
        path: url.pathname,
        route,
        statusCode: res.statusCode,
        durationMs: Number((durationSeconds * 1000).toFixed(2))
      });
    });

    if (url.pathname.startsWith("/api/") || url.pathname === "/health" || url.pathname === "/ready" || url.pathname === "/metrics") {
      handleApi(req, res, url, config).catch(error => {
        log("error", "http_request_failed", {
          method: req.method,
          path: url.pathname,
          error: error.message || "Unexpected server error"
        });
        sendJson(res, 500, { statusCode: 500, message: error.message || "Unexpected server error.", data: null });
      });
      return;
    }

    serveStatic(req, res);
  });
}

const config = normalizeConfig(readJson(CONFIG_PATH, {}));
const server = createServer(config);

server.listen(PORT, () => {
  log("info", "frontend_server_started", {
    port: PORT,
    configPath: CONFIG_PATH,
    resultPath: RESULT_PATH,
    monitoringPath: MONITORING_PATH,
    repositoryBackend: config.repositoryBackend,
    monitoringEnabled: envToBool("SMARTINVENTORY_MONITORING_ENABLED", true)
  });
});

process.on("SIGTERM", () => {
  log("info", "frontend_server_stopping", { signal: "SIGTERM" });
  server.close(() => process.exit(0));
});

process.on("SIGINT", () => {
  log("info", "frontend_server_stopping", { signal: "SIGINT" });
  server.close(() => process.exit(0));
});
