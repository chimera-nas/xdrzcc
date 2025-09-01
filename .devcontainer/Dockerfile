# SPDX-FileCopyrightText: 2025 Ben Jarvis
#
# SPDX-License-Identifier: Unlicense

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update && \
    apt-get -y --no-install-recommends upgrade && \
    apt-get -y --no-install-recommends install unminimize apt-utils && \
    echo y | unminimize && \
    apt-get -y --no-install-recommends install cmake ninja-build git gdb uncrustify reuse \
    man-db manpages-dev ca-certificates ssh flex bison build-essential npm clangd reuse uthash-dev && \
    npm install -g @anthropic-ai/claude-code

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
