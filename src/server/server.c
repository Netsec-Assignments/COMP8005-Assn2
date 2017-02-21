/*********************************************************************************************
Name:			server.c

    Required:	acceptor.h
                done.h
                server.h

    Developer:  Shane Spoor

    Created On: 2017-02-17

    Description:
    Generic function that handles the socket setup

    Revisions:
    (none)

*********************************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <acceptor.h>

#include "done.h"
#include "acceptor.h"
#include "server.h"

atomic_int done = 0;
static __sig_atomic_t handled = 0;
static void nonfatal_sighandler(int sig)
{
    atomic_store(&done, 1);
    handled = 1;
}

/*********************************************************************************************
FUNCTION

    Name:		serve

    Prototype:	int serve(server_t *server, unsigned short port)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    server - Struct for the server data
    port - port number for the server.

    Return Values:
	
    Description:
    Generic function used by the servers to connect to the client.

    Revisions:
	(none)

*********************************************************************************************/
int serve(server_t *server, unsigned short port)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = nonfatal_sighandler;
    sigemptyset(&sa.sa_mask);

    // Set up signal handlers so that we can interrupt system calls on signal
    sigaction(SIGQUIT, &sa, 0);
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);

    acceptor_t acceptor;

    // Thanks Beej: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#bind
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // TODO: Just pass in the port string from argv instead of this nonsense
    char buf[5] = {0};
    snprintf(buf, 5, "%hu", port);
    if (getaddrinfo(NULL, buf, &hints, &acceptor.info) < 0)
    {
        perror("getaddrinfo");
        return -1;
    }

    acceptor.port = port;
    acceptor.sock = socket(acceptor.info->ai_family, acceptor.info->ai_socktype, acceptor.info->ai_protocol);
    if (acceptor.sock < 0)
    {
        perror("socket");
        freeaddrinfo(acceptor.info);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(acceptor.sock, SOL_SOCKET, SO_REUSEADDR, &reuse, (socklen_t)sizeof(reuse)) < 0)
    {
        // This isn't a fatal error, so just print the error message and carry on
        perror("setsockopt");
    }

    if (bind(acceptor.sock, acceptor.info->ai_addr, acceptor.info->ai_addrlen) < 0)
    {
        perror("bind");
        cleanup_acceptor(&acceptor);
        return -1;
    }

    if (listen(acceptor.sock, 100) == -1)
    {
        perror("listen");
        cleanup_acceptor(&acceptor);
        return -1;
    }

    int handles_accept;
    if (server->start(server, &acceptor, &handles_accept) == -1)
    {
        perror("server->start");
        return -1;
    }

    if (!handles_accept)
    {
        // TODO: Need to find a way to set done in the handles_accept case (i.e. every case)
        while(1)
        {
            client_t client;
            if (accept_client(&acceptor, &client) == -1)
            {
                atomic_store(&done, 1);
                break;
            }
            else
            {
                server->add_client(server, client);
            }
        }
    }

    server->cleanup(server);
    cleanup_acceptor(&acceptor);

    if (handled)
    {
        printf("Caught signal; exiting.\n");
        fflush(stdout);
    }

    return handled ? 0 : -1;
}