//
// Created by shane on 2/13/17.
//

#ifndef COMP8005_ASSN2_SERVER_H
#define COMP8005_ASSN2_SERVER_H

#include <netinet/in.h>

typedef struct server_t server_t;

extern server_t* thread_server;
extern server_t* select_server;
extern server_t* epoll_server;

struct server_t
{
    /**
     * Sets up the server (pre-allocates threads, creates select/epoll fds, etc.).
     *
     * @param server The server to initialise.
     *
     * @return 0 on success, -1 on failure with errno set appropriately.
     */
    int (*init)(server_t* server);

    /**
     * Adds a client to the server. This function should return quickly so that the accept thread can resume accepting
     * connections as soon as possible.
     *
     * @param server The server to which the client will be added.
     * @param peer   Informatio about the peer to which the socket is connected.
     * @param sock   The new socket to serve.
     *
     * @return 0 on success, -1 on failure with errno set appropriately.
     */
    int (*add_client)(server_t* server, struct sockaddr_in peer, int sock);

    /**
     * Cleans up the server's resources. This might not actually be called given that the point is to stress the server
     * to exhaustion. Could be used if the main thread receives a signal.
     *
     * @param server The server to clean up.
     */
    void (*cleanup)(server_t* server);

    // Data private to the server implementation (reference to thread pool, queue for receiving new clients, etc.)
    void* private;
};

#endif //COMP8005_ASSN2_SERVER_H
