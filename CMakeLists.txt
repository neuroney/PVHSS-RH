cmake_minimum_required(VERSION 3.10)
project(PVHSS-RH VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(PVHSS_BUILD_BENCHMARKS "Build benchmark and P5 accuracy check targets" ON)

find_package(NTL REQUIRED)
find_package(GMP REQUIRED)
find_package(RLC REQUIRED)

add_library(pvhss_external_deps INTERFACE)
target_include_directories(pvhss_external_deps INTERFACE
    ${NTL_INCLUDE_PATHS}
    ${GMP_INCLUDE_PATHS}
    ${RLC_INCLUDE_PATHS}
)
target_link_libraries(pvhss_external_deps INTERFACE
    ${NTL_LIBRARIES}
    ${GMP_LIBRARIES}
    ${RLC_LIBRARIES}
)
if(UNIX)
    target_link_libraries(pvhss_external_deps INTERFACE m)
endif()

include(CTest)

add_subdirectory(common)
add_subdirectory(programs)
add_subdirectory(backends/group_hss)
add_subdirectory(backends/group_vhss)
add_subdirectory(backends/rlwe_hss)
add_subdirectory(backends/rlwe_vhss)
add_subdirectory(backends/cz)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
if(PVHSS_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
