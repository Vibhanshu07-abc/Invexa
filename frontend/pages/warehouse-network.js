import { renderInsightCards } from "../components/cards.js";
import { renderNetworkGraph } from "../graphs/network-graph.js";
import { buildWarehouseNetwork } from "../utils/data-helpers.js";

export const WarehouseNetworkPage = {
  id: "warehouse-network",
  title: "Warehouse Network",
  render(context) {
    const network = buildWarehouseNetwork(context.data);
    const focus = context.ui.networkFocus || network.nodes[0]?.id;
    const focused = network.nodes.find(node => node.id === focus);

    return `
      <section class="page-shell">
        <div class="page-header">
          <div>
            <span class="page-eyebrow">Warehouse network</span>
            <h1>Dependencies, transfer opportunities, and movement visibility</h1>
          </div>
        </div>
        ${renderNetworkGraph(network, focus)}
        ${renderInsightCards([
          {
            kicker: "Focused warehouse",
            title: focused?.label || "No warehouse selected",
            body: focused ? `${focused.label} is operating at ${focused.healthScore}% health with status ${focused.status}.` : "Select a warehouse node to inspect dependencies.",
            meta: "Interactive relationship graph"
          },
          {
            kicker: "Recommended transfers",
            title: `${context.data.actions.filter(action => action.type === "move").length} movement actions`,
            body: "Location corrections and transfer actions are surfaced from the action report and should be reviewed as network balancing opportunities."
          },
          {
            kicker: "Warehouse dependencies",
            title: `${network.edges.length} active dependencies`,
            body: "Dependencies reflect shared categories and current operating overlap between warehouses."
          }
        ])}
      </section>
    `;
  }
};
