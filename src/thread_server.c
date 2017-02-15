//
// Created by shane on 2/13/17.
//

#include "assn2/server.h"
#include "assn2/util/vector.h"

typedef struct
{
    
} thread_server_private;

int thread_server_init(server_t* thread_server)
{

}

static server_t thread_server_impl =
{
    NULL,
    NULL,
    NULL,
    NULL
};

server_t* thread_server = &thread_server_impl;
