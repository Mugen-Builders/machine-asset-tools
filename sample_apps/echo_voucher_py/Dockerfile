# syntax=docker.io/docker/dockerfile:1.4
ARG IMAGE_NAME=riscv64/python
ARG IMAGE_TAG=3.12.10-alpine3.21
ARG MACHINE_GUEST_TOOLS_VERSION=0.17.1-r1
ARG CFFI_VERSION=2.0.0
ARG MACHINE_ASSET_TOOLS_VERSION=0.1.0-alpha.4
ARG MACHINE_ASSET_TOOLS_TAR=https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz
ARG MACHINE_ASSET_TOOLS_DEV_TAR=https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_dev_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz

FROM --platform=linux/riscv64 ${IMAGE_NAME}:${IMAGE_TAG} AS base

# Install guest tools
ARG MACHINE_GUEST_TOOLS_VERSION
ADD --chmod=644 https://edubart.github.io/linux-packages/apk/keys/cartesi-apk-key.rsa.pub /etc/apk/keys/cartesi-apk-key.rsa.pub
RUN echo "https://edubart.github.io/linux-packages/apk/stable" >> /etc/apk/repositories
RUN apk update && \
    apk add cartesi-machine-guest-tools=${MACHINE_GUEST_TOOLS_VERSION} \
    cartesi-machine-guest-libcmt=${MACHINE_GUEST_TOOLS_VERSION} \
    libffi-dev=3.4.7-r0 build-base=0.5-r3

ARG MACHINE_ASSET_TOOLS_TAR
ADD ${MACHINE_ASSET_TOOLS_TAR} /

ARG CFFI_VERSION
RUN pip install cffi==${CFFI_VERSION}

WORKDIR /opt/build/app

FROM base AS builder

RUN pip install setuptools==80.9.0

ARG MACHINE_GUEST_TOOLS_VERSION
RUN apk update && \
    apk add cartesi-machine-guest-libcmt-dev=${MACHINE_GUEST_TOOLS_VERSION}

# use fixed libcmt ffi.h
COPY include/libcmt_ffi.h /usr/include/libcmt/ffi.h

ARG MACHINE_ASSET_TOOLS_DEV_TAR
ADD ${MACHINE_ASSET_TOOLS_DEV_TAR} /

COPY tools/compile_libs.py compile_libs.py
RUN python compile_libs.py

FROM base AS app

WORKDIR /opt/cartesi/app

COPY --from=builder /opt/build/app/pycm.cpython-312-riscv64-linux-musl.so /usr/local/lib/python3.12/site-packages/pycm.cpython-312-riscv64-linux-musl.so

RUN <<EOF
set -e
find /usr/local/lib -type d -name __pycache__ -exec rm -r {} +
find . -type d -name __pycache__ -exec rm -r {} +
rm -rf /var/cache/apk /var/log/* /var/cache/* /tmp/*
EOF
