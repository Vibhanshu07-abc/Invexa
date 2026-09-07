export function renderLoadingState(message = "Loading warehouse intelligence...") {
  return `
    <section class="page-state page-state-loading">
      <div class="spinner"></div>
      <p>${message}</p>
    </section>
  `;
}

export function renderEmptyState(title, body) {
  return `
    <section class="page-state">
      <h3>${title}</h3>
      <p>${body}</p>
    </section>
  `;
}

export function renderErrorState(title, body) {
  return `
    <section class="page-state page-state-error">
      <h3>${title}</h3>
      <p>${body}</p>
    </section>
  `;
}
