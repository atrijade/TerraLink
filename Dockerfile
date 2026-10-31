# ==============================================================================
# Stage 1: Build the React + Cesium Web UI
# ==============================================================================
FROM node:20-bullseye-slim AS frontend-builder
WORKDIR /app/web-ui

# Install frontend dependencies
COPY web-ui/package*.json ./
RUN npm install

# Build production assets (outputs to /app/web-ui/dist)
COPY web-ui/ ./
RUN npm run build

# ==============================================================================
# Stage 2: Build the C++ NTN-Sim Backend
# ==============================================================================
FROM gcc:13-bookworm AS backend-builder
RUN apt-get update && apt-get install -y cmake ninja-build git --no-install-recommends && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Copy C++ source code
COPY ntn-sim/ ./ntn-sim/

# Configure and compile release binary using Ninja
WORKDIR /app/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ../ntn-sim
RUN ninja ntn_sim_bin

# ==============================================================================
# Stage 3: Lightweight Production Runtime
# ==============================================================================
FROM debian:bookworm-slim
WORKDIR /app

# Install standard C++ runtime libraries
RUN apt-get update && apt-get install -y libstdc++6 ca-certificates --no-install-recommends && rm -rf /var/lib/apt/lists/*

# Copy compiled C++ binary, config files, and datasets
COPY --from=backend-builder /app/build/ntn_sim_bin /app/ntn_sim_bin
COPY --from=backend-builder /app/ntn-sim/config /app/config
COPY --from=backend-builder /app/ntn-sim/datasets /app/datasets

# Copy built frontend assets into www/ (served automatically by C++ server)
COPY --from=frontend-builder /app/web-ui/dist /app/www

# Render injects $PORT dynamically; default to 10000
ENV PORT=10000
EXPOSE 10000

# Execute server
CMD ["/app/ntn_sim_bin"]
