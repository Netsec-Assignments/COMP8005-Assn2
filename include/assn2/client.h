

#ifndef COMP8005_ASSN2_CLIENT_H
#define COMP8005_ASSN2_CLIENT_H

typedef struct client_t client_t;

struct client_t
{
};

int start_acceptor(client_t* client, unsigned short port);

#endif