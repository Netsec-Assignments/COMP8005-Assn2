//
// Created by shane on 2/13/17.
//

#include <stddef.h>
#include "assn2/server/server.h"

static server_t select_server_impl =
{
    NULL,
    NULL,
    NULL,
    NULL
};

server_t* select_server = &select_server_impl;