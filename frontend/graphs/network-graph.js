import { escapeHtml } from "../utils/dom.js";

export function renderNetworkGraph(network, focusedNode) {
  if (!network.nodes.length || !network.edges.length) {
    return `
      <section class="page-state">
        <h3>Network graph unavailable</h3>
        <p>Warehouse relationship data will appear when multiple warehouse nodes have shared operating dependencies.</p>
      </section>
    `;
  }

  const radius = 160;
  const centerX = 220;
  const centerY = 210;
  const positions = network.nodes.map((node, index) => {
    const angle = (Math.PI * 2 * index) / network.nodes.length;
    return {
      ...node,
      x: centerX + Math.cos(angle) * radius,
      y: centerY + Math.sin(angle) * radius
    };
  });

  return `
    <article class="panel graph-panel">
      <div class="panel-heading">
        <div>
          <span class="panel-kicker">Interactive Graph</span>
          <h3>Warehouse Relationship Graph</h3>
        </div>
      </div>
      <div class="graph-layout">
        <svg viewBox="0 0 460 420" class="graph-svg" role="img" aria-label="Warehouse relationship graph">
          ${network.edges.map(edge => {
            const source = positions.find(node => node.id === edge.source);
            const target = positions.find(node => node.id === edge.target);
            if (!source || !target) return "";
            return `
              <g>
                <line x1="${source.x}" y1="${source.y}" x2="${target.x}" y2="${target.y}" class="graph-edge"></line>
                <text x="${(source.x + target.x) / 2}" y="${(source.y + target.y) / 2}" text-anchor="middle">${escapeHtml(String(edge.weight))}</text>
              </g>
            `;
          }).join("")}
          ${positions.map(node => `
            <g class="graph-node ${focusedNode === node.id ? "is-focused" : ""}" data-node="${escapeHtml(node.id)}">
              <circle cx="${node.x}" cy="${node.y}" r="38"></circle>
              <text x="${node.x}" y="${node.y - 4}" text-anchor="middle">${escapeHtml(node.label)}</text>
              <text x="${node.x}" y="${node.y + 16}" text-anchor="middle">${escapeHtml(`${Math.round(node.healthScore)}%`)}</text>
            </g>
          `).join("")}
        </svg>
        <div class="graph-sidebar">
          ${positions.map(node => `
            <button class="graph-chip ${focusedNode === node.id ? "is-active" : ""}" data-node="${escapeHtml(node.id)}">
              <strong>${escapeHtml(node.label)}</strong>
              <span>${escapeHtml(node.status)}</span>
            </button>
          `).join("")}
        </div>
      </div>
    </article>
  `;
}
