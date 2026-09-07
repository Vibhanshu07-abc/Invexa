import { renderMetricCards } from "../components/cards.js";
import { renderLineChart, renderBarChart } from "../charts/svg-charts.js";
import { renderTable } from "../components/tables.js";
import { formatPercent } from "../utils/format.js";

function buildForecastSeries(forecasts) {
  const totalCurrent = forecasts.reduce((sum, forecast) => sum + Number(forecast.currentDemand || 0), 0);
  const totalProjected = forecasts.reduce((sum, forecast) => sum + Number(forecast.projectedDemand || 0), 0);
  return [
    { label: "Daily", current: totalCurrent, projected: totalProjected },
    { label: "Weekly", current: totalCurrent * 7, projected: totalProjected * 7 },
    { label: "Monthly", current: totalCurrent * 30, projected: totalProjected * 30 },
    { label: "Seasonal", current: totalCurrent * 90, projected: totalProjected * 90 }
  ];
}

export const DemandForecastPage = {
  id: "demand-forecast",
  title: "Demand Forecast",
  render(context) {
    const series = buildForecastSeries(context.data.forecasts);
    const confidenceChart = renderBarChart("Forecast Confidence", context.data.forecasts.map(forecast => ({
      label: forecast.name,
      value: forecast.confidence?.toLowerCase().includes("high") ? 92 : forecast.confidence?.toLowerCase().includes("medium") ? 78 : 60,
      display: forecast.confidence
    })));

    const trendChart = renderLineChart("Daily, Weekly, Monthly, Seasonal Forecast", series, ["current", "projected"]);

    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Demand forecast</span>
            <h1>Projected demand, seasonal scaling, and forecast confidence</h1>
          </div>
        </div>
        ${renderMetricCards([
          { label: "Daily Forecast", value: series[0]?.projected || 0, footnote: "Projected daily demand", tone: "brand" },
          { label: "Weekly Forecast", value: series[1]?.projected || 0, footnote: "Projected weekly demand", tone: "accent" },
          { label: "Monthly Forecast", value: series[2]?.projected || 0, footnote: "Projected monthly demand", tone: "success" },
          { label: "Forecast Confidence", value: formatPercent(context.data.forecastAccuracy), footnote: "Confidence derived from AI forecast payload", tone: "warning" }
        ])}
        <div class="two-column">
          ${trendChart}
          ${confidenceChart}
        </div>
        ${renderTable({
          columns: [
            { key: "name", label: "Product" },
            { key: "currentDemand", label: "Current Demand" },
            { key: "projectedDemand", label: "Projected Demand" },
            { key: "confidence", label: "Forecast Confidence" },
            { key: "explanation", label: "Seasonal Forecast Context" }
          ],
          rows: context.data.forecasts,
          emptyMessage: "No forecast payload is available from the AI layer."
        })}
      </section>
    `;
  }
};
