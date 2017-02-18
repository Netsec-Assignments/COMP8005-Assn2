project(server)

set(SOURCES main.c acceptor.c logger.c thread_server.c select_server.c epoll_server.c server.c)
add_executable(server ${SOURCES})
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include/assn2/server
                                          ${CMAKE_SOURCE_DIR}/include/assn2/util)

target_link_libraries(server util -lm -lpthread -lrt)