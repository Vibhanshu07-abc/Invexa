const API_BASE = window.SMART_INVENTORY_API_BASE || "/api";

function buildUrl(path, params = {}) {
  const url = new URL(`${API_BASE}${path}`, window.location.origin);
  Object.entries(params).forEach(([key, value]) => {
    if (value !== undefined && value !== null && value !== "") {
      url.searchParams.set(key, value);
    }
  });
  return url;
}

export async function apiRequest(path, options = {}) {
  const { params, ...requestOptions } = options;
  const response = await fetch(buildUrl(path, params), {
    headers: {
      "Content-Type": "application/json",
      ...(requestOptions.headers || {})
    },
    cache: "no-store",
    ...requestOptions
  });

  const payload = await response.json().catch(() => null);
  if (!response.ok) {
    throw new Error(payload?.message || `REST request failed with status ${response.status}.`);
  }

  return payload;
}
