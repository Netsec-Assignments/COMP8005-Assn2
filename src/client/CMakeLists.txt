set(SOURCES vector.c)
add_library(client ${SOURCES})
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/include/assn2/client)