cmake_minimum_required(VERSION 3.11)
set(PROJECT_NAME rg_project)
project(${PROJECT_NAME})
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


function(watch)
    set_property(
            DIRECTORY
            APPEND
            PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARGV}
    )
endfunction()

set(CMAKE_POLICY_DEFAULT_CMP0012 NEW)
set(CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_CXX_FLAGS "-g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter --coverage")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

file(GLOB SOURCES "src/*.cpp" "src/*.c" src/main.cpp)
file(GLOB HEADERS "include/*.h" "include/*.hpp")

find_package(OpenGL REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(ASSIMP REQUIRED)

add_subdirectory(libs/glad)
add_subdirectory(libs/imgui)

add_definitions(${OPENGL_DEFINITIONS})

add_library(STB_IMAGE libs/stb_image.cpp)
set_source_files_properties(libs/stb_image.cpp include/stb_image.h
        PROPERTIES
        COMPILE_FLAGS
        "-Wno-shift-negative-value -Wno-implicit-fallthrough --coverage")

set(LIBS glfw glad OpenGL::GL X11 Xrandr Xinerama Xi Xxf86vm Xcursor dl pthread freetype ${ASSIMP_LIBRARIES} STB_IMAGE imgui)


configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)


include_directories(include/)
add_executable(${PROJECT_NAME}
        ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${LIBS})
target_compile_options(${PROJECT_NAME} PRIVATE -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter --coverage)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${OPENGL_DEFINITIONS})
target_link_libraries(${PROJECT_NAME} ${LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
file(GLOB SHADERS "shaders/*.vs"
        "shaders/*.fs")
foreach(SHADER ${SHADERS})
    watch(${SHADER})
endforeach()

if(CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    SETUP_TARGET_FOR_COVERAGE_LCOV(NAME coverage EXECUTABLE rg_project)
endif()

