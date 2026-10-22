# Day 3: multi-stage build. Fill in the TODOs.

# ---- build stage ----
# TODO: FROM <base with a package manager> AS build
# TODO: install a compiler, CMake, and git (FetchContent clones at configure time)
# TODO: copy the sources, configure in Release, build, and run the tests

# ---- runtime stage ----
# TODO: FROM <slim base>. Same distro as the build stage, or expect "GLIBCXX not found"
# TODO: COPY --from=build only the benchmark binary
# TODO: exec-form ENTRYPOINT, so `docker run <image> --benchmark_filter=...` passes flags through
