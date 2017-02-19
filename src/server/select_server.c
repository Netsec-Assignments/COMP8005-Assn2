//
// Created by shane on 2/13/17.
//

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "acceptor.h"
#include "protocol.h"
#include "server.h"

static int select_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept);
static int select_server_add_client(server_t* server, client_t client);
static void select_server_cleanup(server_t* server);

typedef struct
{
    ssize_t transferred;
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

// Not too efficient. Oh well ¯\_(ツ)_/¯
static void register_fds(select_server_client_set* client_set)
{
    FD_ZERO(&client_set->set);
    for (int i = 0; i < FD_SETSIZE; ++i)
    {
        if (client_set->clients[i].sock != -1)
        {
            FD_SET(client_set->clients[i].sock, &client_set->set);
        }
    }
}

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

    // We haven't yet received anything from this client
    if (request->msg == NULL)
    {
        unsigned char* raw = ((unsigned char*)&request->partial_msg_size) + request->transferred;
        size_t bytes_left = sizeof(request->partial_msg_size) - request->transferred;
        ssize_t bytes_read = read_data(sock, raw, sizeof(request->partial_msg_size) - request->transferred);
        if (bytes_read == -1)
        {
            shutdown(sock, 0);
            close(sock);
            return -1;
        }

        request->transferred += bytes_read;
        if (bytes_read < bytes_left)
        {
            return 0;
        }
        else
        {
            request->msg_size = request->partial_msg_size;
            request->msg = malloc(request->msg_size);
            if (!request->msg)
            {
                perror("malloc");
                shutdown(sock, 0);
                close(sock);
                return -1;
            }
        }
    }

    // Read the next piece of data (message size or message content) from the client
    int which_message = request->transferred / (request->msg_size + sizeof(request->msg_size));
    size_t offset = request->transferred % (request->msg_size + sizeof(request->msg_size));

    // We're reading partial_msg_size
    if (offset < sizeof(request->msg_size))
    {
        unsigned char* raw = ((unsigned char*)&request->partial_msg_size) + offset;
        size_t bytes_left = sizeof(request->partial_msg_size) - offset;
        ssize_t bytes_read = read_data(sock, raw, bytes_left);
        if (bytes_read == -1)
        {
            shutdown(sock, 0);
            close(sock);
            return -1;
        }

        request->transferred += bytes_read;
        if (bytes_read >= bytes_left)
        {
            if (request->partial_msg_size == 0)
            {
                // We've read all data from this client, so clean up
                shutdown(sock, 0);
                close(sock);
                free(request->msg);
                set->clients[index].sock = -1;
                return 0;
            }
            else
            {
                offset += bytes_read;
            }
        }
        else
        {
            return 0;
        }
    }

    offset -= sizeof(request->msg_size);
    size_t bytes_left = request->msg_size - offset;
    ssize_t bytes_read = read_data(sock, request->msg + offset, bytes_left);
    if (bytes_read == -1)
    {
        shutdown(sock, 0);
        close(sock);
        return -1;
    }

    request->transferred += bytes_read;
    if (bytes_read == bytes_left)
    {
        if (request->partial_msg_size == 0)
        {
            // We've read all data from this client, so clean up
            shutdown(sock, 0);
            close(sock);
            free(request->msg);
            set->clients[index].sock = -1;
            return 0;
        }
        else
        {
            offset += bytes_read;
        }
    }
    else
    {
        return 0;
    }

    return 0;
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
    int yes = 1;
    if (setsockopt(acceptor->sock, SOL_SOCKET, SOCK_NONBLOCK, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        free(client_set);
        return -1;
    }

    server->private = client_set;

    client_set->clients[acceptor->sock].sock = acceptor->sock;
    client_set->max_fd = acceptor->sock;
    memset(client_set->requests, 0, FD_SETSIZE * sizeof(select_server_request));

    int num_selected;
    while(1)
    {
        register_fds(client_set);

        num_selected = select(client_set->max_fd, &client_set->set, NULL, NULL, NULL);
        if (num_selected == -1)
        {
            if (errno != EINTR)
            {
                perror("select");
            }
            break;
        }

        // Check for new clients
        if(FD_ISSET(acceptor->sock, &client_set->set))
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
            if (client_set->clients[i].sock != -1 && FD_ISSET(client_set->clients[i].sock, &client_set->set))
            {
                if (handle_request(client_set, client_set->clients[i].sock) == -1)
                {
                    break;
                }
            }
        }
    }

    return errno == EINTR;
}

static int select_server_add_client(server_t* server, client_t client)
{
    int yes = 1;
    if (setsockopt(client.sock, SOL_SOCKET, SOCK_NONBLOCK, &yes, sizeof(yes)) == -1)
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