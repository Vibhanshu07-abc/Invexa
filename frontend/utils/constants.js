export const ROLE_LABELS = {
  Admin: "Executive Control",
  "Warehouse Manager": "Warehouse Operations",
  Analyst: "Decision Intelligence",
  Viewer: "Read-Only Access"
};

export const PAGE_DEFINITIONS = [
  { id: "executive-dashboard", label: "Executive Dashboard", icon: "ED", roles: ["Admin", "Warehouse Manager", "Analyst", "Viewer"] },
  { id: "warehouse-operations", label: "Warehouse Operations", icon: "WO", roles: ["Admin", "Warehouse Manager"] },
  { id: "inventory-intelligence", label: "Inventory Intelligence", icon: "II", roles: ["Admin", "Warehouse Manager", "Analyst"] },
  { id: "warehouse-network", label: "Warehouse Network", icon: "WN", roles: ["Admin", "Warehouse Manager", "Analyst"] },
  { id: "decision-center", label: "Decision Center", icon: "DC", roles: ["Admin", "Warehouse Manager", "Analyst"] },
  { id: "demand-forecast", label: "Demand Forecast", icon: "DF", roles: ["Admin", "Warehouse Manager", "Analyst"] },
  { id: "inventory-anomaly-detection", label: "Inventory Anomaly Detection", icon: "IA", roles: ["Admin", "Warehouse Manager", "Analyst"] },
  { id: "supplier-intelligence", label: "Supplier Intelligence", icon: "SI", roles: ["Admin", "Warehouse Manager", "Analyst"] },
  { id: "audit-timeline", label: "Audit Timeline", icon: "AT", roles: ["Admin", "Warehouse Manager", "Analyst", "Viewer"] },
  { id: "reports", label: "Reports", icon: "RP", roles: ["Admin", "Warehouse Manager", "Analyst", "Viewer"] },
  { id: "ai-copilot", label: "AI Copilot", icon: "AI", roles: ["Admin", "Warehouse Manager", "Analyst", "Viewer"] },
  { id: "administration", label: "Administration", icon: "AD", roles: ["Admin"] }
];

export const COPILOT_SUGGESTIONS = [
  "Which warehouse needs attention?",
  "Why is warehouse health low?",
  "What are the priority anomalies?",
  "Summarize the decision center.",
  "What shortages are predicted?"
];
