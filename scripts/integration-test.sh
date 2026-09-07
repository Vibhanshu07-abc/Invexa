#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND_DIR="$ROOT_DIR/backend"
FRONTEND_DIR="$ROOT_DIR/frontend"
OUTPUT_PATH="$BACKEND_DIR/output/result.json"
PORT="${PORT:-18080}"
TEST_PASSWORD="${TEST_PASSWORD:-integration-secret}"
TEST_HASH="$(node -e "const crypto=require('crypto'); process.stdout.write(crypto.createHash('sha256').update(process.argv[1]).digest('hex'));" "$TEST_PASSWORD")"

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
}

trap cleanup EXIT

pushd "$BACKEND_DIR" >/dev/null
SMARTINVENTORY_SERVICE_MODE=true \
SMARTINVENTORY_RUN_ONCE=true \
SMARTINVENTORY_CONFIG_PATH=./config/config.development.json \
SMARTINVENTORY_OUTPUT_JSON_PATH="$OUTPUT_PATH" \
./SmartInventory
popd >/dev/null

pushd "$FRONTEND_DIR" >/dev/null
PORT="$PORT" \
NODE_ENV=test \
SMARTINVENTORY_CONFIG_PATH="$BACKEND_DIR/config/config.development.json" \
SMARTINVENTORY_RESULT_PATH="$OUTPUT_PATH" \
SMARTINVENTORY_ADMIN_PASSWORD_HASH="$TEST_HASH" \
node server.js &
SERVER_PID=$!
popd >/dev/null

for _ in {1..30}; do
  if curl -fsS "http://127.0.0.1:${PORT}/health" >/dev/null; then
    break
  fi
  sleep 1
done

curl -fsS "http://127.0.0.1:${PORT}/health" | grep -q '"status": "ok"'
curl -fsS "http://127.0.0.1:${PORT}/ready" | grep -q '"status": "ready"'
curl -fsS "http://127.0.0.1:${PORT}/metrics" | grep -q 'smartinventory_http_requests_total'
curl -fsS "http://127.0.0.1:${PORT}/api/dashboard" | grep -q '"statusCode": 200'
curl -fsS -X POST "http://127.0.0.1:${PORT}/api/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"username\":\"admin\",\"password\":\"${TEST_PASSWORD}\"}" | grep -q '"authenticated": true'

echo "Integration tests passed."
