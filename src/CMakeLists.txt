add_subdirectory(util)

set(SOURCES main.c acceptor.c logger.c thread_server.c select_server.c epoll_server.c)
add_executable(assn2 ${SOURCES})
target_include_directories(assn2 PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(assn2 util -lm -lpthread -lrt)