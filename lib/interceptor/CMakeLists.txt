add_definitions(-DHAVE_DXGI)

include_directories(BEFORE SYSTEM ${DirectX_D3D11_4_INCLUDE_DIR})

add_convenience_library(interceptor
        dumper.cpp
        state_manager.cpp
        debugger.cpp
        helpers.cpp
)

target_include_directories(interceptor
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/image
        ${CMAKE_SOURCE_DIR}/thirdparty
        ${CMAKE_SOURCE_DIR}/dispatch
        ${CMAKE_SOURCE_DIR}/retrace
)

target_link_libraries(interceptor
        image
        imgui
)
