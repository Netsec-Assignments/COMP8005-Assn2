project(server)

set(SOURCES main.c acceptor.c thread_server.c select_server.c epoll_server.c server.c)
add_executable(server ${SOURCES} ../common/protocol.c)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include/assn2/server
                                          ${CMAKE_SOURCE_DIR}/include/assn2/util
                                          ${CMAKE_SOURCE_DIR}/include/assn2/common)
target_link_libraries(server util -lm -lpthread -lrt)