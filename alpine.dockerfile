################################
# builder
ARG IMAGE_NAME=riscv64/alpine
ARG IMAGE_TAG=3.22.2
ARG MACHINE_GUEST_TOOLS_VERSION=0.17.1-r1
ARG NLOHMANN_JSON_VERSION=3.12.0
ARG NLOHMANN_JSON_SHA=aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63

FROM --platform=linux/riscv64 ${IMAGE_NAME}:${IMAGE_TAG} AS base

# Install guest tools
ARG MACHINE_GUEST_TOOLS_VERSION
ADD --chmod=644 https://edubart.github.io/linux-packages/apk/keys/cartesi-apk-key.rsa.pub /etc/apk/keys/cartesi-apk-key.rsa.pub
RUN echo "https://edubart.github.io/linux-packages/apk/stable" >> /etc/apk/repositories
RUN apk update && \
    apk add cartesi-machine-guest-tools=${MACHINE_GUEST_TOOLS_VERSION}

FROM base AS builder

# Install build essential and libcmt
ARG MACHINE_GUEST_TOOLS_VERSION
RUN apk add \
    alpine-sdk=1.1-r0 \
    clang=20.1.8-r0 \
    clang-dev=20.1.8-r0 \
    cartesi-machine-guest-libcmt-dev=${MACHINE_GUEST_TOOLS_VERSION}

ARG NLOHMANN_JSON_VERSION
ARG NLOHMANN_JSON_SHA
RUN <<EOF
set -e
mkdir -p /usr/include/nlohmann
wget https://github.com/nlohmann/json/releases/download/v${NLOHMANN_JSON_VERSION}/json.hpp -O /usr/include/nlohmann/json.hpp
echo "${NLOHMANN_JSON_SHA} /usr/include/nlohmann/json.hpp" | sha256sum -c -s
EOF
