################################
# builder
ARG IMAGE_NAME=riscv64/alpine
ARG IMAGE_TAG=3.22.1
ARG MACHINE_GUEST_TOOLS_VERSION=0.17.1-r1
ARG MACHINE_ASSET_TOOLS_VERSION=0.1.0-alpha.2

### Base
FROM --platform=linux/riscv64 ${IMAGE_NAME}:${IMAGE_TAG} AS base

# Install guest tools
ARG MACHINE_GUEST_TOOLS_VERSION
ADD --chmod=644 https://edubart.github.io/linux-packages/apk/keys/cartesi-apk-key.rsa.pub /etc/apk/keys/cartesi-apk-key.rsa.pub
RUN echo "https://edubart.github.io/linux-packages/apk/stable" >> /etc/apk/repositories
RUN apk update && \
    apk add cartesi-machine-guest-tools=${MACHINE_GUEST_TOOLS_VERSION}

ARG MACHINE_ASSET_TOOLS_VERSION
ADD https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz /tmp/
RUN tar -xzf /tmp/machine-asset-tools_musl_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz -C / && \
    rm /tmp/machine-asset-tools_musl_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz

### Builder
FROM base AS builder

# Install build essential and libcmt
ARG MACHINE_GUEST_TOOLS_VERSION
RUN apk add \
    alpine-sdk=1.1-r0 \
    clang=20.1.8-r0 \
    clang-dev=20.1.8-r0 \
    cartesi-machine-guest-libcmt-dev=${MACHINE_GUEST_TOOLS_VERSION}

ARG MACHINE_ASSET_TOOLS_VERSION
ADD https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_dev_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz /tmp/
RUN tar -xzf /tmp/machine-asset-tools_musl_riscv64_dev_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz -C / && \
    rm /tmp/machine-asset-tools_musl_riscv64_dev_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz

WORKDIR /opt/build/app

COPY app.cpp .
COPY Makefile .

RUN make

### App
FROM --platform=linux/riscv64 scratch AS app

COPY --from=builder /opt/build/app/build/* /
