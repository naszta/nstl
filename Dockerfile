# Stage 1: Build image
FROM ubuntu:latest AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update && \
    apt-get install -y build-essential git cmake ccache ninja-build libtbb-dev libgtest-dev googletest libssl-dev libcurl4-openssl-dev curl ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

RUN --mount=type=cache,target=/root/.ccache \
    --mount=type=cache,target=/src/build/_deps \
    cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_INSTALL_PREFIX=/install && \
    cmake --build build && \
    cmake --install build
