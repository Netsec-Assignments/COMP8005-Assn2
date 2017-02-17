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
#include <pthread.h>
#include <stdint.h>
#include "client.h"

#define DEFAULT_PORT 8005
#define DEFAULT_IP "192.168.0.9"
#define DEFAULT_NUMBER_CLIENTS 1
#define DEFAULT_MAXIMUM_REQUESTS 1
#define NETWORK_BUFFER_SIZE 1024

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

    client_info client_datas;
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

    client_datas.port = DEFAULT_PORT;
    client_datas.ip = DEFAULT_IP;
    client_datas.maxRequests = DEFAULT_MAXIMUM_REQUESTS;
    client_datas.numOfClients = DEFAULT_NUMBER_CLIENTS;
    

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
                        client_datas.port = (unsigned int)port_int;
                    }
                }
                break;
                case 'i':
                {
                    char* ip_int;
                    char* ip_read = sscanf(optarg, "%s", &ip_int);
                    if (&ip_read != "\0")
                    {
                        fprintf(stderr, "Invalid IP Address %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        client_datas.ip = ip_int;
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
                        client_datas.maxRequests = (unsigned int)maxRequests_int;
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
                        client_datas.numOfClients = (unsigned int)numOfClients_int;
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
        
        if (start_client(client_datas) == -1)
        {
            exit(EXIT_FAILURE);
        }
    }
}


int start_client(client_info client_datas)
{
    int sockets[2];
    int threads = 10;
    int count = 0;
    client_info data[threads];
    pthread_t thread = 0;
    pthread_attr_t attribute;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1)
    {
        fprintf(stderr, "Unable to create socket pair`");
        return -1;
    }

    pthread_attr_init(&attribute);
    
    if (pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_DETACHED) != 0)
    {
        fprintf(stderr, "Thread Attributes unable to be set to detached.");
        return -1;
    }

    if (pthread_attr_setscope(&attribute, PTHREAD_SCOPE_SYSTEM) != 0)
    {
        fprintf(stderr,"System Scopen unable to be set to thread.");
        return -1;
    }
    
    for (count = 0; count < threads; count++)
    {
        memcpy(&data[count], &client_datas, sizeof(client_datas));
    }

    for (count = 0; count < threads; count++)
    {
        if (pthread_create(&thread, &attribute, clients, (void *) &data[count]) != 0)
        {
            fprintf(stderr, "Unable to create thread.");
            return -1;
        }
    }
 
    pthread_attr_destroy(&attribute);
    
    wait(&count);

    return 1;
}

void* clients(void* infos)
{
    int* sockets = 0;
    char* buffer = 0;
    int count = 0;
    int index = 0;
    int result = 0;
    char request[NETWORK_BUFFER_SIZE];
    client_info *data = (client_info *)infos;

    if ((buffer = malloc(sizeof(char) * NETWORK_BUFFER_SIZE)) == NULL)
    {
        fprintf(stderr, "Unable to allocate buffer memory.");
        return -1;
    }
    if ((sockets = malloc(sizeof(int) * data->numOfClients)) == NULL)
    {
        fprintf(stderr, "Unable to allocate socket memory.");
        return -1;
    }

    for (index = 0; index < data->numOfClients; index++)
    {      
        /* Create a socket and connect to the server */
        result = connectToServer(data->port, &sockets[index], data->ip);
        
        /* Set the socket to reuse for improper shutdowns */
        if ((result == -1) || (setReuse(&sockets[index]) == -1))
        {
            break;
        }
    }

    if (result != -1)
    {
        while (1)
        {
            count++;
            
            for (index = 0; index < data->numOfClients; index++)
            {
                if (sendData(&sockets[index], request, strlen(request)) == -1)
                {
                    continue;
                }

            }

            if (count >= data->maxRequests)
            {
                break;
            }
        }
    }

    for (index = 0; index < data->numOfClients; index++)
    {
        closeSocket(&sockets[index]);
    }

    free(buffer);
    
    pthread_exit(NULL);
}

int connectToServer(const char *port, int *sock, const char *ip)
{
    struct addrinfo serverInfo;
    struct addrinfo *result;
    struct addrinfo *rp;
    
    memset(&serverInfo, 0, sizeof(serverInfo));
    serverInfo.ai_family = AF_INET;
    serverInfo.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, port, &serverInfo, &result) != 0)
    {
        return -1;
    }
        
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        *sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        
        if (*sock == -1)
            continue;
        
        if (connect(*sock, rp->ai_addr, rp->ai_addrlen) != -1)
            break;    

        close(*sock);
    }
    
    if (rp == NULL)
    {
        return -1;
    }
    
    freeaddrinfo(result);
    
    return *sock;
}

int setReuse(int* socket)
{
    socklen_t optlen = 1;
    return setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, &optlen, sizeof(optlen));
}

int closeSocket(int* socket)
{
    return close(*socket);
}

int sendData(int *socket, const char *buffer, int bytesToSend)
{
    int sent = 0;
    int sentTotal = 0;
    int bytesLeft = bytesToSend;
    
    while (sent < bytesToSend)
    {
        sent = send(*socket, buffer + sentTotal, bytesLeft, 0);
        if (sent == -1)
        {
            return -1;
        }
        sentTotal += sent;
        bytesLeft = bytesToSend - sentTotal;
    }
    
    return sent;
}