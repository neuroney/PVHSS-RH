add_executable(CZ23 main.cpp  tool.h GenData.h PVHSS.h  PKE.h  function.h PVHSS.h)

target_link_libraries(CZ23
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)