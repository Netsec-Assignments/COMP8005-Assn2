//
// Created by shane on 2/13/17.
//


#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include "acceptor.h"

int accept_client(acceptor_t* acceptor, client_t* out)
{
    struct sockaddr_in peer;
    socklen_t accepted_len = sizeof(peer);
    int peer_sock = accept(acceptor->sock, (struct sockaddr*)&peer, &accepted_len);
    if (peer_sock == -1)
    {
        perror("accept");
        return -1;
    }

    out->peer = peer;
    out->sock = peer_sock;
    return 0;
}

void cleanup_acceptor(acceptor_t* acceptor)
{
    shutdown(acceptor->sock, 0);
    freeaddrinfo(acceptor->info);
}
