FROM ubuntu:24.04 AS vcpkg-base

ARG VCPKG_COMMIT

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    build-essential \
    cmake \
    ninja-build \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git \
 && cd vcpkg \
 && git checkout ${VCPKG_COMMIT} \
 && ./bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

FROM vcpkg-base AS vcpkg-deps
WORKDIR /tmp
COPY vcpkg.json .
RUN vcpkg install --clean-after-build

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    zip \
    unzip \
    pkg-config \
    libgsf-bin \
 && rm -rf /var/lib/apt/lists/*

COPY --from=vcpkg-deps /opt/vcpkg /opt/vcpkg
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

WORKDIR /workspace
