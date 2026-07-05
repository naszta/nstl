# Stage 1: Build image
FROM ubuntu:noble AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,sharing=locked,target=/var/cache/apt \
    apt-get update && \
    apt-get install -y build-essential git cmake gcc-14 g++-14 ccache ninja-build && \
    rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 14 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 14

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
