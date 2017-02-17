/*********************************************************************************************
Name:		main.c

Required:	main.h	

Developer:	Mat Siwoski

Created On: 2017-02-17

Description:
	This is the client application.
Revisions:
	(none)

*********************************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "client.h"

#define DEFAULT_PORT 8005
#define DEFAULT_IP "192.168.0.9"
#define DEFAULT_NUMBER_CLIENTS 1
#define DEFAULT_MAXIMUM_REQUESTS 1


/*********************************************************************************************
FUNCTION

    Name:		print_usage

    Prototype:	void print_usage(char const* name) 

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    name - name

    Return Values:
	
    Description:
    Prints usage help when running the application.

    Revisions:
	(none)

*********************************************************************************************/
void print_usage(char const* name)
{
    printf("usage: %s [-h] [-i ip] [-p port] [-c clients]\n", name);
    printf("\t-h, --help:               print this help message and exit.\n");
    printf("\t-i, --ip [ip]             the ip on which the server is on.\n");
    printf("\t-p, --port [port]:        the port on which to listen for connections;\n");
    printf("\t-m, --max [max]           the max numbers of requests.\n");
    printf("\t-n, --clients [clients]   the number of clients to create.\n");
    printf("\t                          default port is %d.\n", DEFAULT_PORT);
    printf("\t                          default IP is %s.\n", DEFAULT_IP);
    printf("\t                          default number of clients is %d.\n", DEFAULT_NUMBER_CLIENTS);
    printf("\t                          default number of max requests is %d.\n", DEFAULT_MAXIMUM_REQUESTS);
}

/*********************************************************************************************
FUNCTION

    Name:		main

    Prototype:	int main(int argc, char** argv) 

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    argc - Number of arguments
	argv - Arguments

    Return Values:
	
    Description:
    Runs the different servers based off the users choice.

    Revisions:
	(none)

*********************************************************************************************/
int main(int argc, char** argv)
{
    unsigned int port = DEFAULT_PORT;
    char* ip = DEFAULT_IP;
    unsigned int maxRequests = DEFAULT_MAXIMUM_REQUESTS;
    unsigned int numOfClients = DEFAULT_NUMBER_CLIENTS;
    char const* short_opts = "i:p:m:n:h";
    struct option long_opts[] =
    {
        {"ip",      1, NULL, 'i'},
        {"port",    1, NULL, 'p'},
        {"max",     1, NULL, 'm'},
        {"clients", 1, NULL, 'n'},
        {"help",    0, NULL, 'h'},
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
                    unsigned int port_int;
                    int num_read = sscanf(optarg, "%u", &port_int);
                    if (num_read != 1 || port_int > UINT16_MAX)
                    {
                        fprintf(stderr, "Invalid port number %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        port = (unsigned int)port_int;
                    }
                }
                break;
                case 'i':
                {
                    char* ip_int;
                    char* ip_read = sscanf(optarg, "%s", &ip_int);
                    if (&ip_read != "\0"
                    {
                        fprintf(stderr, "Invalid IP Address %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        ip = ip_int;
                    }
                }
                break;
                case 'm':
                {
                    unsigned int maxRequests_int;
                    int maxRequests_read = sscanf(optarg, "%d", &maxRequests_int);
                    if( maxRequests_read != 1)
                    {
                        fprintf(stderr, "Invalid number of Max Requests %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        maxRequests = (unsigned int)maxRequests_int;
                    }
                }                    
                break;
                case 'n':
                {
                    unsigned int numOfClients_int;
                    int numOfClients_read = sscanf(optarg, "%d", &numOfClients_int);
                    if(numOfClients_read != 1)
                    {
                        fprintf(stderr, "Invalid number of clients %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        numOfClients = (unsigned int)numOfClients_int;
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
        
        if (start_client(ip, port, maxRequests, numOfClients) == -1)
        {
            exit(EXIT_FAILURE);
        }
    }
}


int start_client(char* ip, unsigned int port, unsigned int maxRequests, unsigned int numOfClients)
{
    return 1;
}
