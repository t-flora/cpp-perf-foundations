# Both stages must use the same base, so it's set once: --build-arg UBUNTU_VERSION=22.04
ARG UBUNTU_VERSION=24.04

# ---- build stage: full toolchain, never shipped ----
FROM ubuntu:${UBUNTU_VERSION} AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential cmake git ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Configure fetches Google Benchmark (hence git); a failing test fails the image build
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build -j"$(nproc)" \
 && cd build && ctest --output-on-failure

# ---- runtime stage: only the benchmark binary ----
# Same distro as the build stage, so the libstdc++ the binary was linked against is the one
# it finds at runtime (avoids "GLIBCXX_3.4.xx not found")
FROM ubuntu:${UBUNTU_VERSION} AS runtime
COPY --from=build /src/build/bench_sorting /usr/local/bin/bench_sorting
WORKDIR /results
# Exec form, so `docker run <image> --benchmark_filter=radix` passes flags straight through
ENTRYPOINT ["/usr/local/bin/bench_sorting"]
