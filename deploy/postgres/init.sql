CREATE TABLE IF NOT EXISTS inventory_items (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    category TEXT NOT NULL,
    warehouse_id INTEGER NOT NULL,
    warehouse_name TEXT NOT NULL,
    expected_quantity INTEGER NOT NULL,
    actual_quantity INTEGER NOT NULL,
    price NUMERIC(12, 2) NOT NULL,
    demand_score INTEGER NOT NULL,
    expected_location TEXT NOT NULL,
    current_location TEXT NOT NULL,
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS audit_events (
    id BIGSERIAL PRIMARY KEY,
    item_id INTEGER NOT NULL,
    actual_quantity INTEGER NOT NULL,
    mismatch_quantity INTEGER NOT NULL,
    current_location TEXT NOT NULL,
    audited_at TIMESTAMPTZ NOT NULL
);
