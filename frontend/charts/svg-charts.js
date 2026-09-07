import { escapeHtml } from "../utils/dom.js";

function maxValue(data) {
  return Math.max(...data.map(point => Number(point.value || 0)), 1);
}

export function renderBarChart(title, data) {
  const height = 220;
  const width = 560;
  const gap = 14;
  const barWidth = Math.max(28, Math.floor((width - gap * (data.length - 1)) / Math.max(data.length, 1)));
  const max = maxValue(data);

  return `
    <article class="panel chart-panel">
      <div class="panel-heading">
        <div>
          <span class="panel-kicker">Chart</span>
          <h3>${escapeHtml(title)}</h3>
        </div>
      </div>
      <svg viewBox="0 0 ${width} ${height + 48}" class="chart-svg" role="img" aria-label="${escapeHtml(title)}">
        ${data.map((point, index) => {
          const value = Number(point.value || 0);
          const barHeight = Math.round((value / max) * height);
          const x = index * (barWidth + gap);
          const y = height - barHeight;
          return `
            <g>
              <rect x="${x}" y="${y}" width="${barWidth}" height="${barHeight}" rx="10"></rect>
              <text x="${x + barWidth / 2}" y="${height + 18}" text-anchor="middle">${escapeHtml(point.label)}</text>
              <text x="${x + barWidth / 2}" y="${Math.max(14, y - 8)}" text-anchor="middle">${escapeHtml(point.display || String(value))}</text>
            </g>
          `;
        }).join("")}
      </svg>
    </article>
  `;
}

export function renderLineChart(title, data, seriesKeys) {
  const width = 560;
  const height = 220;
  const keys = seriesKeys.length ? seriesKeys : Object.keys(data[0] || {}).filter(key => key !== "label");
  const max = Math.max(...data.flatMap(point => keys.map(key => Number(point[key] || 0))), 1);
  const step = data.length > 1 ? width / (data.length - 1) : width;
  const palette = ["#7bdff6", "#f7b267", "#7bd389", "#f25f5c"];

  return `
    <article class="panel chart-panel">
      <div class="panel-heading">
        <div>
          <span class="panel-kicker">Trend</span>
          <h3>${escapeHtml(title)}</h3>
        </div>
      </div>
      <svg viewBox="0 0 ${width} ${height + 40}" class="chart-svg" role="img" aria-label="${escapeHtml(title)}">
        ${keys.map((key, seriesIndex) => {
          const color = palette[seriesIndex % palette.length];
          const path = data.map((point, index) => {
            const x = index * step;
            const y = height - (Number(point[key] || 0) / max) * height;
            return `${index === 0 ? "M" : "L"} ${x} ${y}`;
          }).join(" ");
          return `<path d="${path}" fill="none" stroke="${color}" stroke-width="4" stroke-linecap="round"></path>`;
        }).join("")}
        ${data.map((point, index) => `<text x="${index * step}" y="${height + 18}" text-anchor="middle">${escapeHtml(point.label)}</text>`).join("")}
      </svg>
      <div class="chart-legend">
        ${keys.map((key, index) => `<span><i style="background:${palette[index % palette.length]}"></i>${escapeHtml(key)}</span>`).join("")}
      </div>
    </article>
  `;
}

export function renderDonutChart(title, data) {
  const total = Math.max(data.reduce((sum, item) => sum + Number(item.value || 0), 0), 1);
  const circumference = 2 * Math.PI * 54;
  let offset = 0;
  const palette = ["#7bdff6", "#7bd389", "#f7b267", "#f25f5c", "#9b8cff"];

  return `
    <article class="panel chart-panel donut-panel">
      <div class="panel-heading">
        <div>
          <span class="panel-kicker">Distribution</span>
          <h3>${escapeHtml(title)}</h3>
        </div>
      </div>
      <div class="donut-shell">
        <svg viewBox="0 0 160 160" class="donut-svg" role="img" aria-label="${escapeHtml(title)}">
          <circle cx="80" cy="80" r="54" class="donut-track"></circle>
          ${data.map((item, index) => {
            const length = (Number(item.value || 0) / total) * circumference;
            const dashArray = `${length} ${circumference - length}`;
            const dashOffset = -offset;
            offset += length;
            return `<circle cx="80" cy="80" r="54" class="donut-segment" stroke="${palette[index % palette.length]}" stroke-dasharray="${dashArray}" stroke-dashoffset="${dashOffset}"></circle>`;
          }).join("")}
          <text x="80" y="76" text-anchor="middle" class="donut-total">${Math.round(total)}</text>
          <text x="80" y="96" text-anchor="middle" class="donut-label">Total</text>
        </svg>
        <div class="chart-legend">
          ${data.map((item, index) => `
            <span><i style="background:${palette[index % palette.length]}"></i>${escapeHtml(item.label)} (${escapeHtml(item.display || String(item.value))})</span>
          `).join("")}
        </div>
      </div>
    </article>
  `;
}
