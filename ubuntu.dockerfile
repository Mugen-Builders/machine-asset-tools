################################
# builder
ARG IMAGE_NAME=riscv64/ubuntu
ARG IMAGE_TAG=noble-20251013
ARG MACHINE_GUEST_TOOLS_VERSION=0.17.2
ARG NLOHMANN_JSON_VERSION=3.12.0
ARG NLOHMANN_JSON_SHA=aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63

FROM --platform=linux/riscv64 ${IMAGE_NAME}:${IMAGE_TAG} AS base

# Install busybox
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    busybox-static=1:1.36.1-6ubuntu3.1

# Install guest tools
ARG MACHINE_GUEST_TOOLS_VERSION
ADD https://github.com/cartesi/machine-guest-tools/releases/download/v${MACHINE_GUEST_TOOLS_VERSION}/machine-guest-tools_riscv64.deb /tmp/
RUN dpkg -i /tmp/machine-guest-tools_riscv64.deb
RUN rm /tmp/machine-guest-tools_riscv64.deb

FROM base AS builder

# Install build essential and libcmt
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    wget=1.21.4-1ubuntu4.1 ca-certificates=20240203 \
    g++-14=14.2.0-4ubuntu2~24.04 build-essential=12.10ubuntu1 \
    clang-tidy=1:18.0-59~exp2 clang-format=1:18.0-59~exp2

RUN <<EOF
set -e
apt-get remove -y g++-13
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100
update-alternatives --config g++
EOF

ARG NLOHMANN_JSON_VERSION
ARG NLOHMANN_JSON_SHA
RUN <<EOF
set -e
mkdir -p /usr/include/nlohmann
wget https://github.com/nlohmann/json/releases/download/v${NLOHMANN_JSON_VERSION}/json.hpp -O /usr/include/nlohmann/json.hpp
echo "${NLOHMANN_JSON_SHA} /usr/include/nlohmann/json.hpp" | sha256sum -c --quiet
EOF
