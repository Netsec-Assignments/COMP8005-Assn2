//
// Created by shane on 2/13/17.
//
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "vector.h"
#include "ring_buffer.h"
#include "server.h"
#include "protocol.h"

static const unsigned int WORKER_POOL_SIZE = 200;

#define CLIENT_BACKLOG_SIZE 100
static client_t client_backlog_buf[CLIENT_BACKLOG_SIZE];

typedef struct
{
    client_stats_t stats;
    uint32_t msg_size;
    char* msg;
} thread_server_request;

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

static int thread_server_start(server_t* start, acceptor_t* acceptor, int* handles_accept);
static int thread_server_add_client(server_t* server, client_t client);
static void thread_server_cleanup(server_t* server);

static void* worker_func(void* void_params)
{
    worker_params* params = (worker_params*)void_params;

    // Busy wait for a socket or done ;)
    // If we cared about the threaded implementation, use of a conditional variable/eventfd would probably be a good
    // way to make it better
    while (1)
    {
        int done_local;
        while(!atomic_load(&params->busy) && !(done_local = atomic_load(&done)));
        if (done_local)
        {
            break;
        }

        // Handle the new client
        thread_server_request request;

        if (read_data(params->client.sock, &request.msg_size, sizeof(request.msg_size)) == -1 ||
            (request.msg = malloc(request.msg_size)) == NULL)
        {
            atomic_store(&done, 1);
            break;
        }
        //fprintf(stderr, "%u\n", request.msg_size);
        //fflush(stderr);

        read_data(params->client.sock, request.msg, request.msg_size);
        send_data(params->client.sock, request.msg, request.msg_size);
    }

    free(params);
    return NULL;
}

static void accept_loop(server_t* server, acceptor_t* acceptor)
{
    // Get clients from the acceptor and send them to an available thread
    while (1)
    {
        client_t next_client;
        int result = accept_client(acceptor, &next_client);
        if (result == -1 || atomic_load(&done))
        {
            break;
        }

        if (server->add_client(server, next_client) == -1)
        {
            break;
        }
    }
}

int thread_server_start(server_t *thread_server, acceptor_t *acceptor, int *handles_accept)
{
    *handles_accept = 1;

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
        vector_push_back(&priv->worker_params_list, &params);
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

    thread_server->private = priv;
    accept_loop(thread_server, acceptor);
    return 0;
}

static int thread_server_add_client(server_t* server, client_t client)
{
    thread_server_private* private = (thread_server_private*)server->private;
    worker_params** list = (worker_params**)private->worker_params_list.items;

    // Find the next non-busy thread, if any
    unsigned i;
    for(i = 0; i < private->worker_params_list.size; ++i)
    {
        worker_params* params = list[i];
        int busy = atomic_load(&params->busy);
        if (!busy)
        {
            list[i]->client = client;
            atomic_store(&params->busy, 1);
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
            atomic_store(&done, 1);
            free(new_params);
            return -1;
        }

        new_params->busy = 1;
        new_params->client = client;

        pthread_t new_thread;
        if(pthread_create(&new_thread, NULL, worker_func, new_params) == -1 ||
           pthread_detach(new_thread) == -1)
        {
            atomic_store(&done, 1);
            free(new_params);
            return -1;
        }
    }

    return 0;
}

static void thread_server_cleanup(server_t* thread_server)
{
    thread_server_private* private = (thread_server_private*)thread_server->private;
    vector_free(&private->worker_params_list); // The threads will free the individual elements... I hope
    atomic_store(&done, 1);
    free(private);
}

static server_t thread_server_impl =
{
    thread_server_start,
    thread_server_add_client,
    thread_server_cleanup,
    NULL
};

server_t* thread_server = &thread_server_impl;
