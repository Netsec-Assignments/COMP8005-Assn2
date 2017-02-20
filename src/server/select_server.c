//
// Created by shane on 2/13/17.
//

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#undef __FD_SETSIZE
#define __FD_SETSIZE 32768
#include <sys/select.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "log.h"
#include "timing.h"
#include "done.h"
#include "acceptor.h"
#include "protocol.h"
#include "server.h"

static int select_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept);
static int select_server_add_client(server_t* server, client_t client);
static void select_server_cleanup(server_t* server);

typedef struct
{
    ssize_t transferred;
    time_t transfer_time;
    uint32_t partial_msg_size; // :(
    uint32_t msg_size;
    char* msg;
} select_server_request;

typedef struct
{
    fd_set set;
    int max_fd;
    client_t clients[FD_SETSIZE];
    select_server_request requests[FD_SETSIZE];
} select_server_client_set;

/**
 * Handles a client request on the given socket.
 *
 * @param set  The client set for this server, which contains the list of clients and requests.
 * @param sock The socket for the given client.
 * @return 0 on success, or -1 on failure.
 */
static int handle_request(select_server_client_set* set, int sock)
{
    // TODO: Try to remove some of the return paths

    int index = sock; // To make things a bit less confusing
    select_server_request* request = set->requests + index;

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

        char csv[256];
        char *addr = inet_ntoa(set->clients[index].peer.sin_addr);
        snprintf(csv, 256, "%ld,%ld,%s\n", request->transfer_time, request->transferred, addr);
        log_msg(csv);

        char pretty[256];
        snprintf(pretty, 256, "Transfer time; %ldms; total bytes transferred: %ld; peer: %s\n",
                 request->transfer_time, request->transferred, addr);
        printf("%s\n", pretty);
    }

    shutdown(sock, 0);
    close(sock);
    free(request->msg);

    // Reset everything for the next client
    FD_CLR(sock, &set->set);
    request->msg = NULL;
    request->msg_size = 0;
    request->partial_msg_size = 0;
    request->transferred = 0;
    request->transfer_time = 0;
    set->clients[index].sock = -1;
    return result;
}

static int select_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept)
{
    *handles_accept = 1;

    select_server_client_set* client_set = malloc(sizeof(select_server_client_set));
    if (client_set == NULL)
    {
        perror("malloc");
        return -1;
    }

    // Set accept socket to non-blocking mode
    if (fcntl(acceptor->sock, F_SETFD, O_NONBLOCK) == -1)
    {
        perror("fnctl");
        free(client_set);
        return -1;
    }

    server->private = client_set;

    for (int i = 0; i < FD_SETSIZE; ++i)
    {
        client_set->clients[i].sock = -1;
    }

    client_set->clients[acceptor->sock].sock = acceptor->sock;
    client_set->max_fd = acceptor->sock;
    memset(client_set->requests, 0, FD_SETSIZE * sizeof(select_server_request));

    FD_ZERO(&client_set->set);
    FD_SET(acceptor->sock, &client_set->set);

    int num_selected;
    while(!atomic_load(&done))
    {
        fd_set read_fds = client_set->set;

        num_selected = select(client_set->max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (num_selected == -1)
        {
            // if (errno != EBADF)
            // {
                if (errno != EINTR)
                {
                    perror("select");
                }
                break;
            // }
        }

        // Check for new clients
        if(FD_ISSET(acceptor->sock, &read_fds))
        {
            client_t client;
            int result = accept_client(acceptor, &client);
            if (result == -1)
            {
                if (errno != EWOULDBLOCK)
                {
                    break;
                }
            }
            else
            {
                if (server->add_client(server, client) == -1)
                {
                    break;
                }
            }

        }

        for (int i = acceptor->sock + 1; i < FD_SETSIZE; ++i)
        {
            if (atomic_load(&done))
            {
                break;
            }

            if (client_set->clients[i].sock != -1 && FD_ISSET(client_set->clients[i].sock, &read_fds))
            {
                if (handle_request(client_set, client_set->clients[i].sock) == -1)
                {
                    return -1;
                }
            }
        }
    }

    return (errno && errno != EINTR) * -1;
}

static int select_server_add_client(server_t* server, client_t client)
{
    if (fcntl(client.sock, F_SETFD, O_NONBLOCK) == -1)
    {
        perror("setsockopt");
        return -1;
    }

    select_server_client_set* client_set = (select_server_client_set*)server->private;
    client_set->clients[client.sock] = client;
    if (client.sock > client_set->max_fd)
    {
        client_set->max_fd = client.sock;
    }
    FD_SET(client.sock, &client_set->set);

    return 0;
}

static void select_server_cleanup(server_t* server)
{
    select_server_client_set* client_set = (select_server_client_set*)server->private;

    for (size_t i = 0; i < FD_SETSIZE; ++i)
    {
        if (client_set->clients[i].sock != -1)
        {
            // Technically we could just use i here, but w/e
            shutdown(client_set->clients[i].sock, 0);
            close(client_set->clients[i].sock);
            free(client_set->requests[i].msg);
        }
    }
    free(server->private);
}

static server_t select_server_impl =
{
    select_server_start,
    select_server_add_client,
    select_server_cleanup,
    NULL
};

server_t* select_server = &select_server_impl;