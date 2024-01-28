set(CMAKE_CXX_STANDARD 20)

include_directories(BEFORE SYSTEM ${DirectX_D3D11_4_INCLUDE_DIR})

add_convenience_library(debugger
        model_fingerprints_window.cpp
        model_instances_window.cpp
        model_instance_details_window.cpp
        debugger.cpp
)

target_include_directories(debugger
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/interceptor
        ${CMAKE_SOURCE_DIR}/thirdparty
)

target_link_libraries(debugger
        imgui
        interceptor
)
