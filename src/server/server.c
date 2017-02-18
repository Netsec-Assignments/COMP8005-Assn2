//
// Created by shane on 2/17/17.
//

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <acceptor.h>
#include "acceptor.h"
#include "server.h"

static __sig_atomic_t done = 0;
static void nonfatal_sighandler(int sig)
{
    done = 1;
}

int serve(server_t *server, unsigned short port)
{
    // Set up signal handlers for Ctrl + C and Ctrl + D
    signal(SIGINT, nonfatal_sighandler);
    signal(SIGTERM, nonfatal_sighandler);

    acceptor_t acceptor;

    // Thanks Beej: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#bind
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // TODO: Just pass in the port string from argv instead of this nonsense
    char buf[5] = {0};
    snprintf(buf, 5, "%hu", port);
    int result = getaddrinfo(NULL, buf, &hints, &acceptor.info);
    if (result == -1)
    {
        perror("getaddrinfo");
        return -1;
    }

    acceptor.sock = socket(acceptor.info->ai_family, acceptor.info->ai_socktype, acceptor.info->ai_protocol);
    if (acceptor.sock == -1)
    {
        perror("socket");
        freeaddrinfo(acceptor.info);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(acceptor.sock, SOL_SOCKET, SO_REUSEADDR, &reuse, (socklen_t)sizeof(reuse)) == -1)
    {
        // This isn't a fatal error, so just print the error message and carry on
        perror("setsockopt");
    }

    if (bind(acceptor.sock, acceptor.info->ai_addr, acceptor.info->ai_addrlen) == -1)
    {
        perror("bind");
        cleanup_acceptor(&acceptor);
        return -1;
    }

    if (listen(acceptor.sock, 5) == -1)
    {
        perror("listen");
        cleanup_acceptor(&acceptor);
        return -1;
    }

    int handles_accept;
    if (server->start(server, &acceptor, &handles_accept) == -1)
    {
        return -1;
    }

    if (!handles_accept)
    {
        // TODO: Need to find a way to set done in the handles_accept case (i.e. every case)
        while(!done)
        {
            client_t client;
            if (accept_client(&acceptor, &client) == -1)
            {
                server->cleanup(server);
                cleanup_acceptor(&acceptor);
                return -1;
            }

            server->add_client(server, client);
        }
    }

    server->cleanup(server);
    cleanup_acceptor(&acceptor);

    printf("Caught signal, exiting.\n");

    return 0;
}