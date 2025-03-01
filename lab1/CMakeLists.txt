cmake_minimum_required(VERSION 3.30)
project(lab1)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-ffast-math)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenMP REQUIRED)



set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        vec3.h
        vec3.cpp
        vec4.h
        vec4.cpp
        mat3.h
        mat3.cpp
        mat4.h
        mat4.cpp
)

add_executable(lab1 main.cpp ${IMGUI_SOURCES})

target_include_directories(lab1 PRIVATE
        ${OPENGL_INCLUDE_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_link_libraries(lab1 PRIVATE
        ${OPENGL_LIBRARIES}
        glfw
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(lab1 PRIVATE OpenMP::OpenMP_CXX)
endif()