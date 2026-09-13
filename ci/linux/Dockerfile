FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y \
        build-essential \
        ca-certificates \
        ccache \
        git \
        libasound2-dev \
        libfreetype6-dev \
        libglib2.0-dev \
        libjack-jackd2-dev \
        libudisks2-dev \
        libx11-dev \
        libxcomposite-dev \
        libxcursor-dev \
        libxext-dev \
        libxi-dev \
        libxinerama-dev \
        libxrandr-dev \
        libxrender-dev \
        ninja-build \
        pkg-config \
        wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.31.5/cmake-3.31.5-Linux-x86_64.sh \
        -q -O /tmp/cmake-install.sh \
    && mkdir /opt/cmake-3.31.5 \
    && sh /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.31.5 \
    && rm /tmp/cmake-install.sh \
    && ln -s /opt/cmake-3.31.5/bin/* /usr/local/bin
