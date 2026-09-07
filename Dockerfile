FROM debian:bookworm-slim AS backend-builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential make pkg-config libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/backend
COPY backend/ /app/backend/
RUN make all

FROM debian:bookworm-slim AS backend-runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libspdlog1.10 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/backend
COPY --from=backend-builder /app/backend/SmartInventory /usr/local/bin/smartinventory
COPY backend/config /app/backend/config
COPY backend/data /app/backend/data
COPY data /app/data
RUN mkdir -p /app/shared/output /app/backend/logs

ENV SMARTINVENTORY_SERVICE_MODE=true
ENV SMARTINVENTORY_STARTUP_MODE=demo
ENV SMARTINVENTORY_SERVICE_REFRESH_SECONDS=30
ENV SMARTINVENTORY_CONFIG_PATH=/app/backend/config/config.production.json
ENV SMARTINVENTORY_INVENTORY_CSV_PATH=/app/data/inventory.csv
ENV SMARTINVENTORY_OUTPUT_JSON_PATH=/app/shared/output/result.json
ENV SMARTINVENTORY_MONITORING_PATH=/app/shared/output/monitoring.json
ENV SMARTINVENTORY_LOG_FILE_PATH=/app/backend/logs/backend.log

CMD ["smartinventory"]

FROM node:20-alpine AS frontend-runtime

WORKDIR /app/frontend
COPY frontend/ /app/frontend/
COPY backend/config /app/backend-config
RUN mkdir -p /app/shared/output

ENV NODE_ENV=production
ENV PORT=8080
ENV SMARTINVENTORY_CONFIG_PATH=/app/backend-config/config.production.json
ENV SMARTINVENTORY_RESULT_PATH=/app/shared/output/result.json
ENV SMARTINVENTORY_MONITORING_PATH=/app/shared/output/monitoring.json

CMD ["node", "server.js"]
