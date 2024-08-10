cmake_minimum_required(VERSION 3.13)

set(TARGET "SeaInvaders")
project(${TARGET} VERSION 0.0.1 LANGUAGES C)   

set(CMAKE_C_STANDARD 11)           
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
include_directories(include ${SDL2_INCLUDE_DIRS})

file(GLOB SRC_FILES src/*.c)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

add_executable(${TARGET} ${SRC_FILES})

target_compile_options(${TARGET}
  PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    $<$<CONFIG:Debug>:
      -O0
      -g
      -fsanitize=address,undefined,leak
    >
    $<$<CONFIG:Release>:
      -O3
    >
)

target_link_libraries(${TARGET}
  ${SDL2_LIBRARIES}
  $<$<CONFIG:Debug>:
    -fsanitize=address
    -fsanitize=undefined
    -fsanitize=leak
  >
)
