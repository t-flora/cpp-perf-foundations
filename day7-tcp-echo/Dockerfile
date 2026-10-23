# Linux build of the echo server and client, for comparing loopback TCP with the host's.
# The experiments run both ends inside one container, so traffic never leaves its kernel.
ARG UBUNTU_VERSION=24.04
FROM ubuntu:${UBUNTU_VERSION}
RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential cmake python3 \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B /build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build /build -j"$(nproc)" \
 && cd /build && ctest --output-on-failure
ENTRYPOINT ["python3", "scripts/run_experiments.py", "--build", "/build"]
