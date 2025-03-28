cmake_minimum_required(VERSION 3.13...3.24)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(
    pyromancer
    LANGUAGES C CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")  # Keep all runtime files in one directory.

file(
    GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Enforce UTF-8 encoding on MSVC.
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# Enable warnings recommended for new projects.
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

if (EMSCRIPTEN)
    # Attach data folder to Emscripten builds.
    target_link_options(${PROJECT_NAME} PRIVATE
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/data@data"
        -lidbfs.js  # Enable IDBFS (browser file system.)
    )
    configure_file(
        ${PROJECT_SOURCE_DIR}/emscripten/index.html
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_MATH_DEFINES)  # For M_PI
target_compile_definitions(${PROJECT_NAME} PRIVATE NO_SOUND)
target_compile_definitions(${PROJECT_NAME} PRIVATE NO_LUA)

add_subdirectory(umbra)

find_package(SDL3 CONFIG REQUIRED)
find_package(libtcod CONFIG REQUIRED)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        libtcod::libtcod
        umbra::umbra
)
