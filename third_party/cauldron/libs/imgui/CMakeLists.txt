cmake_minimum_required(VERSION 3.6)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_GENERATOR_PLATFORM x64)
project (ImGUI)

set(imgui_src "imgui.h" "imgui.cpp" "imgui_draw.cpp" "imgui_tables.cpp" "imgui_widgets.cpp")
add_library (${PROJECT_NAME} STATIC ${imgui_src})

target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


