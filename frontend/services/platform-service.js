import { apiRequest } from "./api-client.js";

export const PlatformService = {
  getDashboard(budget) {
    return apiRequest("/dashboard", {
      params: {
        generatedAt: new Date().toISOString(),
        mode: "Warehouse Intelligence",
        budget
      }
    });
  },
  getInventory() {
    return apiRequest("/inventory");
  },
  getAnalytics() {
    return apiRequest("/analytics");
  },
  getDecisionCenter() {
    return apiRequest("/decision-center");
  },
  getReports() {
    return apiRequest("/reports");
  },
  getWarehouseHealth() {
    return apiRequest("/warehouse-health");
  },
  login(username, password) {
    return apiRequest("/auth/login", {
      method: "POST",
      body: JSON.stringify({ username, password })
    });
  },
  askCopilot(question) {
    return apiRequest("/copilot", {
      method: "POST",
      body: JSON.stringify({ question })
    });
  }
};
