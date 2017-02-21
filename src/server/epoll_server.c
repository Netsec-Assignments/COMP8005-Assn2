/*********************************************************************************************
Name:			epoll_server.c

    Required:	epoll_server.h	
                acceptor.h
                done.h
                server.h
                protocol.h

    Developer:	Mat Siwoski/Shane Spoor

    Created On: 2017-02-17

    Description:
    This is the epoll server. This file will deal with handling the epoll server connections
    coming from the client.

    Revisions:
    (none)

*********************************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h> 

#include "acceptor.h"
#include "done.h"
#include "server.h"
#include "protocol.h"

static server_t epoll_server_impl =
{
    NULL,
    NULL,
    NULL,
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
    client_t clients[FD_SETSIZE];
    epoll_server_request requests[FD_SETSIZE];
} epoll_server_client_set;

/*********************************************************************************************
FUNCTION

    Name:		epoll_server_start

    Prototype:	static int epoll_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    server - server struct with server data
    acceptor - acceptor struct with acceptor data
    handles_accept - The number of handles to accept.

    Return Values:
	
    Description:
    This is the start of the epoll server. This will set up the connections and pass the data to another
    function to handle the data.

    Revisions:
	(none)

*********************************************************************************************/
static int epoll_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept)
{
    *handles_accept = 1;
    int epoll_max_events = 10000;
    int epoll = 0;
    int epoll_ready = 0;
    struct epoll_event event;
    struct epoll_event events[epoll_max_events];
    int index = 0;
    
    epoll_server_client_set* client_set = malloc(sizeof(epoll_server_client_set));
    if (client_set == NULL)
    {
        perror("malloc");
        return -1;
    }

    // Set accept socket to non-blocking mode
    if (fcntl(acceptor->sock, F_SETFL, O_NONBLOCK) == -1)
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
    memset(client_set->requests, 0, FD_SETSIZE * sizeof(epoll_server_request));

    FD_ZERO(&client_set->set);
    FD_SET(acceptor->sock, &client_set->set);

    int num_selected;

    if ((epoll = epoll_create1(0)) == -1)
    {
        perror("epoll_create1");
        return -1;
    }
    event.events = EPOLLIN;
    event.data.fd = acceptor->sock;

    if (epoll_ctl(epoll, EPOLL_CTL_ADD, acceptor->sock, &event) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    while (1)
    {
        epoll_ready = epoll_wait(epoll, events, epoll_max_events, -1);
        if (epoll_ready == -1)
        {
            perror("epoll_wait");
            break;
        }
        
        for (index = 0; index < epoll_ready; index++)
        {
            if (events[index].data.fd == acceptor->sock)
            {
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
                }
            }
            else
            {
                //do something about the data
            }
        }
    }
    
    close(acceptor->sock);
    close(epoll);

    return (errno && errno != EINTR) * -1;
}