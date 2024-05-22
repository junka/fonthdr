cmake_minimum_required(VERSION 3.10)
project(my_project)

# Find the FreeType2 library
find_package(Freetype REQUIRED)
if(NOT FREETYPE_FOUND)
  message(FATAL_ERROR "FreeType2 library not found")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -g -O0")

add_executable(fonthdr bmp.c fttool.c)

# Link the FreeType2 library to the executable
target_link_libraries(fonthdr ${FREETYPE_LIBRARIES})

# Include the FreeType2 include directories
include_directories(${FREETYPE_INCLUDE_DIRS})
