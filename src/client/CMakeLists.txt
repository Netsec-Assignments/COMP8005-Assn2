project(client)

set(SOURCES main.c)
add_executable(client ${SOURCES} ../common/protocol.c)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/include/assn2/client
                                          ${CMAKE_SOURCE_DIR}/include/assn2/util
                                          ${CMAKE_SOURCE_DIR}/include/assn2/common)

target_link_libraries(client util -lm -lpthread -lrt)