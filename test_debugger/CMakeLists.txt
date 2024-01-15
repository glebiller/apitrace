include_directories(BEFORE SYSTEM ${DirectX_D3D11_4_INCLUDE_DIR})

add_executable(test_debugger
        main.cpp
)

target_include_directories(test_debugger
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/image
        ${CMAKE_SOURCE_DIR}/thirdparty
        ${CMAKE_SOURCE_DIR}/dispatch
        ${CMAKE_SOURCE_DIR}/retrace
)

find_library(D3D11_LIB d3d11)
if(NOT D3D11_LIB)
    message(FATAL_ERROR "d3d11 library not found")
endif()

target_link_libraries(test_debugger
        dxgitrace
        interceptor
        image
        imgui
        ${D3D11_LIB}
)
