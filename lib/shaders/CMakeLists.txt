add_library (shaders STATIC
        vertex_shader.cpp
        vertex_shader_simulator.cpp
)

target_include_directories(shaders PRIVATE
)

target_link_libraries(shaders PRIVATE
)
