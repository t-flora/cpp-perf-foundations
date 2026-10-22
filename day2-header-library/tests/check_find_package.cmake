# Run by CTest (see tests/CMakeLists.txt). Installs vec3 into a scratch prefix,
# then configures, builds, and runs examples/consumer against that install.
set(work "${BUILD_DIR}/find_package_test")
file(REMOVE_RECURSE "${work}")

function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "Command failed (${rc}): ${ARGV}")
    endif()
endfunction()

run("${CMAKE_COMMAND}" --install "${BUILD_DIR}" --config "${CONFIG}" --prefix "${work}/install")
run("${CMAKE_COMMAND}" -S "${SOURCE_DIR}/examples/consumer" -B "${work}/build"
    "-DCMAKE_PREFIX_PATH=${work}/install" "-DCMAKE_BUILD_TYPE=${CONFIG}")
run("${CMAKE_COMMAND}" --build "${work}/build" --config "${CONFIG}")
run("${CTEST}" -C "${CONFIG}" --output-on-failure WORKING_DIRECTORY "${work}/build")
