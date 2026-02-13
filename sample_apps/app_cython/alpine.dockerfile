# syntax=docker.io/docker/dockerfile:1.4
ARG MACHINE_GUEST_TOOLS_VERSION=0.17.1-r1
ARG CFFI_VERSION=2.0.0
ARG MACHINE_ASSET_TOOLS_VERSION=0.1.0-alpha.4
ARG MACHINE_ASSET_TOOLS_TAR=https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz
ARG MACHINE_ASSET_TOOLS_DEV_TAR=https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_dev_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz
ARG SETUPTOOLS_VERSION=80.9.0
ARG CYTHON_VERSION=3.2.2

FROM --platform=linux/riscv64 riscv64/python:3.13.11-alpine3.22 AS base

# Install guest tools
ARG MACHINE_GUEST_TOOLS_VERSION
ADD --chmod=644 https://edubart.github.io/linux-packages/apk/keys/cartesi-apk-key.rsa.pub /etc/apk/keys/cartesi-apk-key.rsa.pub
RUN echo "https://edubart.github.io/linux-packages/apk/stable" >> /etc/apk/repositories
RUN apk update && \
    apk add cartesi-machine-guest-tools=${MACHINE_GUEST_TOOLS_VERSION} \
    cartesi-machine-guest-libcmt=${MACHINE_GUEST_TOOLS_VERSION} \
    build-base=0.5-r3

ARG MACHINE_ASSET_TOOLS_TAR
ADD ${MACHINE_ASSET_TOOLS_TAR} /

FROM base AS builder

ARG SETUPTOOLS_VERSION
ARG CYTHON_VERSION
RUN pip install setuptools==${SETUPTOOLS_VERSION} cython==${CYTHON_VERSION}

ARG MACHINE_GUEST_TOOLS_VERSION
RUN apk update && \
    apk add cartesi-machine-guest-libcmt-dev=${MACHINE_GUEST_TOOLS_VERSION}

ARG MACHINE_ASSET_TOOLS_DEV_TAR
ADD ${MACHINE_ASSET_TOOLS_DEV_TAR} /

# use fixed libcmt rollup.h
COPY include/rollup.h /usr/include/libcmt/.

USER dapp

WORKDIR /opt/build/app/cmpy

COPY --chown=dapp:dapp cmpy /opt/build/app/cmpy

RUN python setup.py build_ext -i

FROM base AS app

WORKDIR /opt/cartesi/app

COPY --from=builder /opt/build/app/cmpy/*.so /usr/local/lib/python3.13/site-packages/.

RUN <<EOF
set -e
find /usr/local/lib -type d -name __pycache__ -exec rm -r {} +
find . -type d -name __pycache__ -exec rm -r {} +
rm -rf /var/cache/apk /var/log/* /var/cache/* /tmp/*
EOF
