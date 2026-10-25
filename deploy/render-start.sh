#!/bin/sh
set -eu

# The C++ service continuously refreshes the JSON read by the Node.js API.
smartinventory &
backend_pid=$!

shutdown() {
  kill "$backend_pid" 2>/dev/null || true
}

trap shutdown INT TERM EXIT
node /app/frontend/server.js
