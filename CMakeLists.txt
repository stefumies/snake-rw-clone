cmake_minimum_required(VERSION 3.29)
unset(CMAKE_TOOLCHAIN_FILE CACHE)


project(SnakeClone
        VERSION 1.0
        LANGUAGES C
)

# Use C23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Raylib root
set(RAYLIB_ROOT "C:/Tools/raylib/w64devkit/x86_64-w64-mingw32")

# Collect all source files
file(GLOB_RECURSE SOURCES
        src/*.c
        src/*.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Include raylib headers
target_include_directories(${PROJECT_NAME} PRIVATE
        ${RAYLIB_ROOT}/include
)

# Link raylib and Windows system libraries
target_link_directories(${PROJECT_NAME} PRIVATE
        ${RAYLIB_ROOT}/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        winmm
        gdi32
        opengl32
)

# Helpful warnings
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()




