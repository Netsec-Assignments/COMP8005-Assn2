//
// Created by shane on 2/13/17.
//
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "assn2/server.h"
#include "assn2/util/vector.h"

const int WORKER_POOL_SIZE = 200;

static atomic_int done = 0;

typedef struct
{
    atomic_int busy;
    int socket;
    struct sockaddr_in peer;
} worker_data;

typedef struct
{
    vector_t worker_params;
    pthread_t add_client_handler;
} private;

static void worker_func(void* worker_data)
{
    worker_data* data = (worker_data*)worker_data;

    // Busy wait for a socket ;)
    while(!atomic_load_explicit(data->busy, memory_order_acquire));
    if (!atomic_load_explicit(done, memory_order_acquire))
    {
        
    }
}

int thread_server_init(server_t* thread_server)
{
    private* priv = malloc(sizeof(private));
    if (!priv)
    {
        perror("malloc");
        return -1;
    }

    if (vector_init(&priv->workers, sizeof(worker_data*), WORKER_POOL_SIZE) == -1)
    {
        fprintf(stderr, "vector_init failed");
        return -1;
    }

    // Allocate structures to find 
    size_t i;
    for(i = 0; i < WORKER_POOL_SIZE; ++i)
    {
        worker_data data = malloc(sizeof(worker_data));
        worker->data.busy = 0;
        if (!data)
        {
            break;
        }
        vector_push_back(&priv->workers, data);
    }
    if (i != WORKER_POOL_SIZE)
    {
        size_t const allocd = i;
        worker_data** worker_param_list = (worker_data**)priv->worker_params.items;
        perror("malloc");

        for (i = 0; i < allocd; ++i)
        {
            free(worker_param_list[i]);
        } 
        vector_free(&priv->worker_params);
        return -1;
    }

    for (i = 0; i < WORKER_POOL_SIZE; ++i)
    {
        worker_data** worker_param_list = (worker_data**)priv->worker_params.items;
        pthread_t thread;
        int result = pthread_create(&thread, NULL, worker_func, worker_param_list[i]);
        
    }

    if (i != WORKER_POOL_SIZE)
    {
        size_t const threads = i;
        //for (i = 0; i )
        fprintf(stderr, "Fuck\n");
        return -1;
    }
}

static server_t thread_server_impl =
{
    thread_server_init,
    NULL,
    NULL,
    NULL
};

server_t* thread_server = &thread_server_impl;
