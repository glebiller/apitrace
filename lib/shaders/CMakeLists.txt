add_library (shaders STATIC
        vertex_shader.cpp
        vertex_shader_simulator.cpp
)

target_include_directories(shaders PRIVATE
)

target_link_libraries(shaders PRIVATE
)

add_executable(test_shaders
    main.cpp
)

target_include_directories(test_shaders PRIVATE
        ${CMAKE_SOURCE_DIR}/helpers
        ${CMAKE_SOURCE_DIR}/lib/math
)

target_link_libraries(test_shaders PRIVATE
        os
        d3dhelpers
        shaders
)
