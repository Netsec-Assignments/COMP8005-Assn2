#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "acceptor.h"

#define DEFAULT_PORT 8005

void print_usage(char const* name)
{
    printf("usage: %s [-h] [-p port]\n", name);
    printf("\t-h, --help:          print this help message and exit.\n");
    printf("\t-p, --port [port]:   the port on which to listen for connections;\n");
    printf("\t                     default is %u.\n", DEFAULT_PORT);
    printf("\t-s, --server [name]: the server used to handle connections.\n");
    printf("\t                     Valid values are thread, select, or epoll.");
    printf("\t                     Default is epoll.");
}

int main(int argc, char** argv)
{
    short port = DEFAULT_PORT;
    server_t* server = epoll_server;

    char const* short_opts = "p:s:h";
    struct option long_opts[] =
    {
        {"port",   1, NULL, 'p'},
        {"server", 1, NULL, 's'},
        {"help",   0, NULL, 'h'},
        {0, 0, 0, 0},
    };

    if (argc)
    {
        int c;
        while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
        {
            switch(c)
            {
                case 'p':
                {
                    int num_read = sscanf(optarg, "%hu", &port);
                    if (num_read != 1)
                    {
                        fprintf(stderr, "Invalid port number %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                }
                break;
                case 's':
                {
                    if (strcmp(optarg, "epoll") == 0)
                    {
                        server = epoll_server;
                    }
                    else if (strcmp(optarg, "select") == 0)
                    {
                        server == select_server;
                    }
                    else if (strcmp(optarg, "thread") == 0)
                    {
                        server = thread_server;
                    }
                    else
                    {
                        fprintf(stderr, "Invalid server %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                }
                break;
                case 'h':
                    print_usage(argv[0]);
                    exit(EXIT_SUCCESS);
                break;
                case '?':
                {
                    fprintf (stderr, "Incorrect argument or unknown option. See %s -h for help.", argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;
                default:
                    fprintf(stderr, "You broke getopt_long! Congrats.\n");
                    exit(EXIT_FAILURE);
                break;
            }
        }

        if (start_acceptor(server, port) == -1)
        {
            perror("start_acceptor");
            exit(EXIT_FAILURE);
        }
    }
}
