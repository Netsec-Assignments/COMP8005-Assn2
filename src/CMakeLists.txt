set(SOURCES main.c acceptor.c acceptor.h server.h logger.c logger.h thread_server.c select_server.c epoll_server.c)

add_executable(assn2 ${SOURCES})
