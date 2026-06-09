add_executable(PiCZ
    main.cpp
    GenData.h
    function.h
    PKE.h
    PVHSS.h
    tool.h
)

target_include_directories(PiCZ PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PiCZ
    PRIVATE
        common
)
