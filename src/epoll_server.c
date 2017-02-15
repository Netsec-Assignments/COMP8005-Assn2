//
// Created by shane on 2/13/17.
//

#include <stddef.h>
#include "assn2/server.h"

static server_t epoll_server_impl =
{
    NULL,
    NULL,
    NULL,
    NULL
};

server_t* epoll_server = &epoll_server_impl;