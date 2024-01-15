add_convenience_library (imgui EXCLUDE_FROM_ALL
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/misc/cpp/imgui_stdlib.cpp
        imgui/backends/imgui_impl_dx11.cpp
        imgui/backends/imgui_impl_win32.cpp
)

target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

target_include_directories (imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

target_optimize (imgui)
