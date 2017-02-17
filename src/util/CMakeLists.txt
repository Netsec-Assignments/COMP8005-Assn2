set(SOURCES vector.c ring_buffer.c)
add_library(util ${SOURCES})
target_include_directories(util PRIVATE ${CMAKE_SOURCE_DIR}/include/assn2/util)