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

# A single-container runtime for hosts such as Render.  The Compose stack above
# remains the full local/production setup; this target keeps the CSV demo
# self-contained by running the analysis engine beside the Node web server.
FROM node:20-bookworm-slim AS render-runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libspdlog1.10 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=backend-builder /app/backend/SmartInventory /usr/local/bin/smartinventory
COPY backend/config /app/backend/config
COPY data /app/data
COPY frontend /app/frontend
COPY deploy/render-start.sh /app/deploy/render-start.sh
RUN mkdir -p /app/shared/output /app/backend/logs \
    && chmod +x /app/deploy/render-start.sh

ENV NODE_ENV=production
ENV PORT=10000
ENV SMARTINVENTORY_SERVICE_MODE=true
ENV SMARTINVENTORY_STARTUP_MODE=demo
ENV SMARTINVENTORY_SERVICE_REFRESH_SECONDS=30
ENV SMARTINVENTORY_CONFIG_PATH=/app/backend/config/config.json
ENV SMARTINVENTORY_INVENTORY_CSV_PATH=/app/data/inventory.csv
ENV SMARTINVENTORY_OUTPUT_JSON_PATH=/app/shared/output/result.json
ENV SMARTINVENTORY_MONITORING_PATH=/app/shared/output/monitoring.json
ENV SMARTINVENTORY_RESULT_PATH=/app/shared/output/result.json
ENV SMARTINVENTORY_REPOSITORY_BACKEND=csv
ENV SMARTINVENTORY_LOG_FILE_PATH=/app/backend/logs/backend.log

EXPOSE 10000
CMD ["/app/deploy/render-start.sh"]
