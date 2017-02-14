//
// Created by shane on 2/13/17.
//

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include "acceptor.h"

int start_acceptor(server_t* server, short port)
{
    if (server->init(server) == -1)
    {
        return -1;
    }

    int accept_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (accept_sock == -1)
    {
        fprintf(stderr, "Error while creating accept socket.\n");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(accept_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, (socklen_t)sizeof(reuse)) == -1)
    {
        // This isn't a fatal error, so just print the error message and carry on
        perror("start_acceptor: couldn't set socket option SO_REUSEADDR");
    }

    struct sockaddr addr;
    addr.sa_family = AF_INET;
    memset(addr.sa_data, 0, sizeof(addr.sa_data));

    if (bind(accept_sock, &addr, sizeof(addr)) == -1)
    {
        perror("bind");
        return -1;
    }

    

    return 0;
}