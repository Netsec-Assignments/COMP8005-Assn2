//
// Created by shane on 2/13/17.
//
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assn2/util/vector.h>
#include <netdb.h>

#include "assn2/util/vector.h"
#include "assn2/util/ring_buffer.h"
#include "assn2/server/server.h"
#include "assn2/server/client.h"

typedef struct
{
    struct sockaddr_in peer;
    int sock;
} client_t;
.h"
#include "assn2/server/server.h"

static const unsigned int WORKER_POOL_SIZE = 200;

static const unsigned int CLIENT_BACKLOG_SIZE = 100;
static client_t client_backlog_buf[CLIENT_BACKLOG_SIZE];

static atomic_int done = 0;

typedef struct
{
    atomic_int busy;
    client_t client;
} worker_params;

typedef struct
{
    vector_t worker_params_list;
    ring_buffer_t client_backlog;
} thread_server_private;

static void* worker_func(void* void_params)
{
    worker_params* params = (worker_params*)void_params;

    // Busy wait for a socket or done ;)
    // If we cared about the threaded implementation, use of a conditional variable/eventfd would probably be a good
    // way to make it better
    while (1)
    {
        int done_local;
        while(!atomic_load(&params->busy) && !(done = atomic_load(&done)));
        if (done_local)
        {
            break;
        }

        // We got a new client, so handle that
        //client_stats_t
    }

    free(params);
    return NULL;
}

static void* get_clients_thread_func(void* void_server)
{
    server_t* server = (server_t*)void_server;
    thread_server_private* private = (thread_server_private*)server->private;
    worker_params** list = (worker_params**)private->worker_params_list.items;

    // Get clients from the acceptor and send them to an available thread
    while (!atomic_load_explicit(&done, memory_order_acquire))
    {
        client_t next_client;
        ring_buffer_get(&private->client_backlog, &next_client);

        // Find the next non-busy thread, if any
        unsigned i;
        for(i = 0; i < private->worker_params_list.size; ++i)
        {
            worker_params* params = list[i];
            int busy = atomic_load_explicit(&params->busy, memory_order_acquire);
            if (!busy)
            {
                list[i]->client = next_client;
                atomic_store_explicit(&params->busy, 1);
                break;
            }
        }

        if (i == private->worker_params_list.size)
        {
            // All threads are busy; add a new one to the list and give it the new connection
            worker_params* new_params = NULL;
            new_params = malloc(sizeof(worker_params));
            if (!new_params || (vector_push_back(&private->worker_params_list, &new_params) == -1))
            {
                free(new_params);
                goto error_cleanup;
            }

            new_params->busy = 1;
            new_params->client = next_client;

            pthread_t new_thread;
            if(pthread_create(&new_thread, NULL, worker_func, new_params) == -1 ||
               pthread_detach(new_thread) == -1)
            {
                free(new_params);
                goto error_cleanup;
            }
        }
    }

    vector_free(&private->worker_params_list); // The threads will free the individual elements
    return NULL;

error_cleanup:
    // TODO: Log error
    atomic_store(&done, 1);
    vector_free(&private->worker_params_list);
    getaddrinfo()
    return NULL;
}

int thread_server_init(server_t* thread_server)
{
    thread_server_private* priv = malloc(sizeof(thread_server_private));
    if (!priv)
    {
        perror("malloc");
        return -1;
    }

    ring_buffer_init(&priv->client_backlog, &client_backlog_buf[0], CLIENT_BACKLOG_SIZE, sizeof(client_t));

    if (vector_init(&priv->worker_params_list, sizeof(worker_params*), WORKER_POOL_SIZE) == -1)
    {
        fprintf(stderr, "vector_init failed");
        return -1;
    }

    size_t i;
    for(i = 0; i < WORKER_POOL_SIZE; ++i)
    {
        worker_params* params = malloc(sizeof(worker_params));
        if (!params)
        {
            break;
        }
        params->busy = 0;
        vector_push_back(&priv->worker_params_list, params);
    }
    if (i != WORKER_POOL_SIZE)
    {
        size_t const allocd = i;
        worker_params** worker_param_list = (worker_params**)priv->worker_params_list.items;
        perror("malloc");

        for (i = 0; i < allocd; ++i)
        {
            free(worker_param_list[i]);
        } 
        vector_free(&priv->worker_params_list);
        return -1;
    }

    for (i = 0; i < WORKER_POOL_SIZE; ++i)
    {
        worker_params** worker_param_list = (worker_params**)priv->worker_params_list.items;
        pthread_t thread;
        int result = pthread_create(&thread, NULL, worker_func, worker_param_list[i]);
        if (result == -1)
        {
            break;
        }
        pthread_detach(thread);
        
    }

    if (i != WORKER_POOL_SIZE)
    {
        perror("pthread_create");
        atomic_store(&done, 1);

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
