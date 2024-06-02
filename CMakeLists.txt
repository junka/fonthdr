cmake_minimum_required(VERSION 3.10)
project(my_project)

# Find the FreeType2 library
find_package(Freetype REQUIRED)
if(NOT FREETYPE_FOUND)
  message(FATAL_ERROR "FreeType2 library not found")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -g -O0")

add_executable(fonthdr fttool.c)

# Link the FreeType2 library to the executable
target_link_libraries(fonthdr ${FREETYPE_LIBRARIES})

# Include the FreeType2 include directories
include_directories(${FREETYPE_INCLUDE_DIRS})



add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ftsrc.h"
    COMMAND $<TARGET_FILE:fonthdr> -f ${CMAKE_CURRENT_SOURCE_DIR}/fonts/arial.ttf -s "abcdefghijklmnopqrstuvwxyz"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS fonthdr
    COMMENT "Generating sample header file: ${CMAKE_CURRENT_BINARY_DIR}/fonthdr -f ${CMAKE_CURRENT_SOURCE_DIR}/fonts/arial.ttf -s \"abcdefghijklmnopqrstuvwxyz\""
    VERBATIM
)

add_custom_target(eheader ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/ftsrc.h")

add_subdirectory(example)
