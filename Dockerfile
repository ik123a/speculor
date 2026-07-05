# Stage 1: Build environment
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Bootstrap vcpkg locally in /opt
WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git \
    && ./vcpkg/bootstrap-vcpkg.sh

WORKDIR /app

# Pre-install dependencies to cache this layer
COPY vcpkg.json ./
RUN /opt/vcpkg/vcpkg install --triplet=x64-linux

# Copy source files
COPY CMakeLists.txt ./
COPY include/ include/
COPY src/ src/
COPY config.yaml ./

# Configure and compile using CMake and Ninja toolchain
RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="/opt/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -G Ninja

RUN cmake --build build --config Release

# Stage 2: Runtime environment
FROM debian:bookworm-slim

# Install runtime dependencies (like libstdc++)
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled binaries and configuration from builder stage
COPY --from=builder /app/build/speculor_rest /app/speculor_rest
COPY --from=builder /app/build/speculor /app/speculor
COPY --from=builder /app/config.yaml /app/config.yaml

# Expose port for REST API server
EXPOSE 8080

# Start REST server
CMD ["/app/speculor_rest", "8080"]
