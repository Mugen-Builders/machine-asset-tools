# syntax=docker.io/docker/dockerfile:1
ARG APT_UPDATE_SNAPSHOT=20260113T030400Z
ARG MACHINE_GUEST_TOOLS_VERSION=0.17.2
ARG MACHINE_GUEST_TOOLS_SHA256SUM=c077573dbcf0cdc146adf14b480bfe454ca63aa4d3e8408c5487f550a5b77a41
ARG CFFI_VERSION=2.0.0
ARG MACHINE_ASSET_TOOLS_VERSION=0.1.0-alpha.4
ARG MACHINE_ASSET_TOOLS_TAR=https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz
ARG MACHINE_ASSET_TOOLS_DEV_TAR=https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_musl_riscv64_dev_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz
ARG SETUPTOOLS_VERSION=80.9.0
ARG CYTHON_VERSION=3.2.2

FROM --platform=linux/riscv64 cartesi/python:3.13.2-slim-noble AS base
# FROM --platform=linux/riscv64 cartesi/python:3.12.9-slim-noble AS base


ARG APT_UPDATE_SNAPSHOT
ARG DEBIAN_FRONTEND=noninteractive
RUN <<EOF
set -eu
apt-get update
apt-get install -y --no-install-recommends ca-certificates
apt-get update --snapshot=${APT_UPDATE_SNAPSHOT}
apt-get remove -y --purge ca-certificates
apt-get autoremove -y --purge
EOF

# Install guest tools
ARG MACHINE_GUEST_TOOLS_VERSION
ARG MACHINE_GUEST_TOOLS_SHA256SUM
ADD --checksum=sha256:${MACHINE_GUEST_TOOLS_SHA256SUM} \
    https://github.com/cartesi/machine-guest-tools/releases/download/v${MACHINE_GUEST_TOOLS_VERSION}/machine-guest-tools_riscv64.deb \
    /tmp/machine-guest-tools_riscv64.deb

ARG DEBIAN_FRONTEND=noninteractive
RUN <<EOF
set -e
apt-get install -y --no-install-recommends \
  busybox-static \
  /tmp/machine-guest-tools_riscv64.deb

rm /tmp/machine-guest-tools_riscv64.deb
EOF

ARG MACHINE_ASSET_TOOLS_TAR
ADD ${MACHINE_ASSET_TOOLS_TAR} /

FROM base AS builder

RUN <<EOF
set -e
apt-get install -y --no-install-recommends \
    gcc=4:13.2.0-7ubuntu1 libc6-dev=2.39-0ubuntu8.6
EOF

ARG SETUPTOOLS_VERSION
ARG CYTHON_VERSION
RUN pip install setuptools==${SETUPTOOLS_VERSION} cython==${CYTHON_VERSION}

ARG MACHINE_ASSET_TOOLS_DEV_TAR
ADD ${MACHINE_ASSET_TOOLS_DEV_TAR} /

# use fixed libcmt rollup.h
COPY include/rollup.h /usr/include/libcmt/.

USER ubuntu

WORKDIR /opt/build/app/cmpy

COPY --chown=ubuntu:ubuntu cmpy /opt/build/app/cmpy

RUN python setup.py build_ext -i

FROM base AS app

WORKDIR /opt/cartesi/app

# COPY --from=builder /opt/build/app/cmpy/*.so /usr/local/lib/python3.12/site-packages/.
COPY --from=builder /opt/build/app/cmpy/*.so /usr/local/lib/python3.13/site-packages/.

RUN <<EOF
set -e
find /usr/local/lib -type d -name __pycache__ -exec rm -r {} +
find . -type d -name __pycache__ -exec rm -r {} +
rm -rf /var/lib/apt/lists/* /var/log/* /var/cache/* /tmp/*
EOF
