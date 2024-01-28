set(CMAKE_CXX_STANDARD 20)

add_definitions(-DHAVE_DXGI)

include_directories(BEFORE SYSTEM ${DirectX_D3D11_4_INCLUDE_DIR})

add_convenience_library(interceptor
        model_fingerprint.cpp
        model_instance.cpp
        state_manager.cpp
        helpers.cpp
)

target_include_directories(interceptor
        PRIVATE
        ${CMAKE_SOURCE_DIR}/thirdparty
)

target_link_libraries(interceptor
        imgui
)
