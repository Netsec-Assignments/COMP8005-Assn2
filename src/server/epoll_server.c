//
// Created by shane on 2/13/17.
//

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h> 

#include "log.h"
#include "timing.h"
#include "done.h"
#include "acceptor.h"
#include "protocol.h"
#include "server.h"
#include "vector.h"


#define ACCEPT_PER_ITER 100
#define NUM_EPOLL_EVENTS 256

static int epoll_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept);
static int epoll_server_add_client(server_t* server, client_t client);
static void epoll_server_cleanup(server_t* epoll_server);

static server_t epoll_server_impl =
{
    epoll_server_start,
    epoll_server_add_client,
    epoll_server_cleanup,
    NULL
};

server_t* epoll_server = &epoll_server_impl;

typedef struct
{
    ssize_t transferred;
    time_t transfer_time;
    uint32_t partial_msg_size; // :(
    uint32_t msg_size;
    char* msg;
} epoll_server_request;

typedef struct
{
    fd_set set;
    int max_fd;
    client_t client;
    epoll_server_request request;
} epoll_server_client;

typedef struct
{
    int epfd;
    vector_t epoll_clients;
} epoll_server_private;

/**
 * Handles a client request on the given socket.
 *
 * @param set  The client set for this server, which contains the list of clients and requests.
 * @param sock The socket for the given client.
 * @return 0 on success, or -1 on failure.
 */
static int handle_request(epoll_server_private* private, int sock)
{
    epoll_server_client* client_list = (epoll_server_client*)private->epoll_clients.items;
    int index = sock; // To make things a bit less confusing
    epoll_server_client* epoll_client = client_list + index;
    epoll_server_request* request = &epoll_client->request;

    struct timeval start;
    gettimeofday(&start, NULL);

    int result = 0;
    int would_block = 0;
    do
    {
        int which_message = request->transferred / (request->msg_size + sizeof(request->msg_size));
        size_t offset = request->transferred % (request->msg_size + sizeof(request->msg_size));

        if (offset < sizeof(request->msg_size))
        {
            // We're reading a message size
            unsigned char* raw = ((unsigned char*)&request->partial_msg_size) + offset;
            size_t bytes_left = sizeof(request->partial_msg_size) - offset;
            ssize_t bytes_read = read_data(sock, raw, bytes_left);
            if (bytes_read == -1)
            {
                result = -1;
                goto cleanup;
            }

            request->transferred += bytes_read;
            if (bytes_read < bytes_left)
            {
                would_block = 1;
            }
            else
            {
                request->msg_size = request->partial_msg_size;
                if (request->msg_size == 0)
                {
                    // Client is finished sending data
                    goto cleanup;
                }
                if (request->msg == NULL)
                {
                    request->msg = malloc(request->msg_size);
                    if (!request->msg)
                    {
                        perror("malloc");
                        result = -1;
                        goto cleanup;
                    }
                }
            }
        }
        else
        {
            // We're reading message content
            offset -= sizeof(request->msg_size);
            size_t bytes_left = request->msg_size - offset;
            ssize_t bytes_read = read_data(sock, request->msg, bytes_left);

            if (bytes_read == -1)
            {
                result = -1;
                goto cleanup;
            }

            request->transferred += bytes_read;
            if (bytes_read < bytes_left)
            {
                would_block = 1;
            }
            else
            {
                // We've received a full message; echo back to the client
                ssize_t bytes_sent;
                size_t send_bytes_left = (ssize_t)request->msg_size;
                do
                {
                    bytes_sent = send_data(sock, request->msg + (request->msg_size - send_bytes_left), send_bytes_left);
                    send_bytes_left -= bytes_sent;
                } while(bytes_sent != -1 && send_bytes_left > 0);
                
                if (bytes_sent == -1)
                {
                    result = -1;
                    goto cleanup;
                }
            }
        }
    } while(!would_block && !atomic_load(&done));

    {
        struct timeval end;
        gettimeofday(&end, NULL);
        request->transfer_time += TIME_DIFF(start, end);
    }

    return 0;

cleanup:
    if (result == 0)
    {
        // Success, so write results to file
        struct timeval end;
        gettimeofday(&end, NULL);
        request->transfer_time += TIME_DIFF(start, end);

        // unsigned short src_port = ntohs(set->clients[index].peer.sin_port);
        // char *addr = inet_ntoa(set->clients[index].peer.sin_addr);
        // char csv[256];
        // snprintf(csv, 256, "%ld,%ld,%s:%hu\n", request->transfer_time, request->transferred, addr, src_port);
        // fprintf(stderr, "%s", csv);
        // //log_msg(csv);

        // char pretty[256];
        // snprintf(pretty, 256, "Transfer time; %ldms; total bytes transferred: %ld; peer: %s:%hu\n",
        //          request->transfer_time, request->transferred, addr, src_port);
        // printf("%s", pretty);
    }
    else
    {
        perror("oops!");
    }

    struct epoll_event ev;
    epoll_ctl(private->epfd, EPOLL_CTL_DEL, sock, &ev);

    close(sock);
    free(request->msg);

    // Reset everything for the next client
    //FD_CLR(sock, &set->set);
    request->msg = NULL;
    request->msg_size = 0;
    request->partial_msg_size = 0;
    request->transferred = 0;
    request->transfer_time = 0;
    epoll_client->client.sock = -1;
    return result;
}

static int epoll_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept)
{
    *handles_accept = 1;

    int epoll_ready = 0;
    struct epoll_event event;
    struct epoll_event events[NUM_EPOLL_EVENTS];
    
    epoll_server_private* priv = malloc(sizeof(epoll_server_private));
    if (priv == NULL)
    {
        perror("malloc priv");
        return -1;        
    }

    int result = vector_init(&priv->epoll_clients, sizeof(epoll_server_client), NUM_EPOLL_EVENTS);
    if (result == -1)
    {
        perror("malloc clients");
        free(priv);
        return -1;
    }

    // Set accept socket to non-blocking mode
    if (fcntl(acceptor->sock, F_SETFL, O_NONBLOCK | fcntl(acceptor->sock, F_GETFL, 0)) == -1)
    {
        perror("fnctl");
        return -1;
    }

    server->private = priv;

    if ((priv->epfd = epoll_create(NUM_EPOLL_EVENTS)) == -1)
    {
        perror("epoll_create");
        return -1;
    }
    event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
    event.data.fd = acceptor->sock;

    if (epoll_ctl(priv->epfd, EPOLL_CTL_ADD, acceptor->sock, &event) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    while (1)
    {
        epoll_ready = epoll_wait(priv->epfd, events, NUM_EPOLL_EVENTS, -1);
        if (epoll_ready == -1)
        {
            perror("epoll_wait");
            break;
        }
        // printf("number of events ready: %d\n", epoll_ready);
        int index;
        int err = 0;
        for (index = 0; index < epoll_ready && !atomic_load(&done); index++)
        {
            if (events[index].data.fd == acceptor->sock)
            {
                while(1)//for (size_t i = 0; i < ACCEPT_PER_ITER; ++i)
                {
                    client_t client;
                    int result = accept_client(acceptor, &client);
                    if (result == -1)
                    {
                        if (errno != EWOULDBLOCK && errno != EAGAIN)
                        {
                            err = 1;
                        }
                        break;
                    }
                    // if (fcntl(acceptor->sock, F_SETFL, O_NONBLOCK | fcntl(acceptor->sock, F_GETFL, 0)) == -1)
                    // {
                    //     perror("fnctl");
                    //     return -1;
                    // }
                    if (server->add_client(server, client) == -1)
                    {
                        err = 1;
                        break;
                    }
                    else
                    {
                        // printf("Added client\n");
                    }
                }
            }
            else
            {
                int result = handle_request(priv, events[index].data.fd);
                if (result == -1)
                {
                    err = 1;
                    break;
                }
            }
        }
        if (err)
        {
            break;
        }
    }

    return (errno && errno != EINTR) * -1;
}

static int epoll_server_add_client(server_t* server, client_t client)
{
    struct epoll_event event;
    epoll_server_private* priv = (epoll_server_private*)server->private;

    if (fcntl(client.sock, F_SETFL, O_NONBLOCK | fcntl(client.sock, F_GETFL, 0)) == -1)
    {
        perror("fnctl");
        return -1;
    }

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client.sock;
    if (epoll_ctl(priv->epfd, EPOLL_CTL_ADD, client.sock, &event) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    epoll_server_client* clients = (epoll_server_client*)priv->epoll_clients.items;
    clients[client.sock].client = client;
}

static void epoll_server_cleanup(server_t* epoll_server)
{
    epoll_server_private* private = (epoll_server_private*)epoll_server->private;
    vector_free(&private->epoll_clients);
    free(private);
    close(private->epfd);
}