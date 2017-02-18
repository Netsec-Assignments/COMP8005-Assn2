//
// Created by shane on 2/13/17.
//

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>

#include "acceptor.h"

static __sig_atomic_t done = 0;
static void nonfatal_sighandler(int sig)
{
    done = 1;
}

int start_acceptor(server_t* server, unsigned short port)
{
    // Set up signal handlers for Ctrl + C and Ctrl + D
    signal(SIGINT, nonfatal_sighandler);
    signal(SIGTERM, nonfatal_sighandler);

    if (server->init(server) == -1)
    {
        return -1;
    }

    // Thanks Beej: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#bind
    struct addrinfo hints, *res;
    int accept_sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // TODO: Just pass in the port string from argv instead of this nonsense
    char buf[5] = {0};
    snprintf(buf, 5, "%hu", port);
    getaddrinfo(NULL, buf, &hints, &res);

    accept_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (accept_sock == -1)
    {
        perror("socket");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(accept_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, (socklen_t)sizeof(reuse)) == -1)
    {
        // This isn't a fatal error, so just print the error message and carry on
        perror("setsockopt");
    }

    if (bind(accept_sock, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("bind");
        return -1;
    }

    if (listen(accept_sock, 5) == -1)
    {
        perror("listen");
        return -1;
    }

    while(!done)
    {
        struct sockaddr_in accepted;
        socklen_t accepted_len = sizeof(accepted);
        int peer_sock = accept(accept_sock, (struct sockaddr*)&accepted, &accepted_len);
        if (peer_sock == -1)
        {
            perror("accept");
            server->cleanup(server);
            return -1;
        }
        client_t client = {accepted, peer_sock};
        server->add_client(server, client);
    }

    server->cleanup(server);
    printf("Caught signal, exiting.\n");

    return 0;
}