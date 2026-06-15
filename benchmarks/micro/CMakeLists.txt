add_executable(microbench
    microbench.cpp
    ${CMAKE_SOURCE_DIR}/schemes/hss/group_hss/src/HSSElg.cpp
    ${CMAKE_SOURCE_DIR}/schemes/hss/rlwe_hss/src/HSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/common/src/RLWEVHSS.cpp
)

target_include_directories(microbench PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/hss/group_hss/include
    ${CMAKE_SOURCE_DIR}/schemes/hss/rlwe_hss/include
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/common/include
)

target_link_libraries(microbench
    PRIVATE
        common
)
