cmake_minimum_required(VERSION 3.10)
project(PVHSS VERSION 0.1.0)

set(PVHSS_ROOT_DIR $ENV{HOME})
set(PVHSS_INCLUDE_DIR "${PVHSS_ROOT_DIR}/include")
set(PVHSS_LIBRARY_DIR "${PVHSS_ROOT_DIR}/lib")

include_directories(
    ${PVHSS_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

link_directories(${PVHSS_LIBRARY_DIR})

file(GLOB SOURCES 
    "test/main.cpp"
    "src/*.cpp"
)

add_executable(PVHSS ${SOURCES})
target_link_libraries(PVHSS ntl m gmp pthread relic)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)