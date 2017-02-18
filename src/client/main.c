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

#include <arpa/inet.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h> 

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
    client_datas.max_requests = DEFAULT_MAXIMUM_REQUESTS;
    client_datas.num_of_clients = DEFAULT_NUMBER_CLIENTS;
    

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
                    char* ip_read = sscanf(optarg, "%s", ip_int);
                    if (ip_read  == NULL || ip_read[0] == '\0')
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
                    unsigned int max_requests_int;
                    int max_requests_read = sscanf(optarg, "%d", &max_requests_int);
                    if( max_requests_read != 1)
                    {
                        fprintf(stderr, "Invalid number of Max Requests %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        client_datas.max_requests = (unsigned int)max_requests_int;
                    }
                }                    
                break;
                case 'n':
                {
                    unsigned int num_of_clients_int;
                    int num_of_clients_read = sscanf(optarg, "%d", &num_of_clients_int);
                    if(num_of_clients_read != 1)
                    {
                        fprintf(stderr, "Invalid number of clients %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        client_datas.num_of_clients = (unsigned int)num_of_clients_int;
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
    int read = 0;
    int count = 0;
    int data_received = 0;
    int request_time = 0;
    int index = 0;
    int result = 0;
    char request[NETWORK_BUFFER_SIZE];
    client_info *data = (client_info *)infos;
    struct timeval start_time;
    struct timeval end_time;

    if ((buffer = malloc(sizeof(char) * NETWORK_BUFFER_SIZE)) == NULL)
    {
        fprintf(stderr, "Unable to allocate buffer memory.");
        return 0;
    }
    if ((sockets = malloc(sizeof(int) * data->num_of_clients)) == NULL)
    {
        fprintf(stderr, "Unable to allocate socket memory.");
        return 0;
    }

    for (index = 0; index < data->num_of_clients; index++)
    {      
        /* Create a socket and connect to the server */
        result = connect_to_server(data->port, &sockets[index], data->ip);
        
        /* Set the socket to reuse for improper shutdowns */
        if ((result == -1) || (set_reuse(&sockets[index]) == -1))
        {
            break;
        }
    }

    if (result != -1)
    {
        while (1)
        {
            count++;
            
            for (index = 0; index < data->num_of_clients; index++)
            {
                gettimeofday(&start_time, NULL);

                if (send_data(&sockets[index], request, strlen(request)) == -1)
                {
                    continue;
                }

                if (read = read_data(&sockets[index], request, strlen(request)) == -1)
                {
                    continue;
                }


                gettimeofday(&end_time, NULL);

                data_received += read;
                request_time += (end_time.tv_sec * 1000000 + end_time.tv_usec) -
                               (start_time.tv_sec * 1000000 + start_time.tv_usec);
            }

            if (count >= data->max_requests)
            {
                break;
            }
        }
    }

    for (index = 0; index < data->num_of_clients; index++)
    {
        close_socket(&sockets[index]);
    }

    free(buffer);
    
    pthread_exit(NULL);
}

int connect_to_server(const char *port, int *sock, const char *ip)
{
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, port, &hints, result) != 0)
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

int set_reuse(int* socket)
{
    socklen_t optlen = 1;
    return setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, &optlen, sizeof(optlen));
}

int close_socket(int* socket)
{
    return close(*socket);
}

int send_data(int *socket, const char *buffer, int bytes_to_send)
{
    int sent = 0;
    int sentTotal = 0;
    int bytes_left = bytes_to_send;
    
    while (sent < bytes_to_send)
    {
        sent = send(*socket, buffer + sentTotal, bytes_left, 0);
        if (sent == -1)
        {
            return -1;
        }
        sentTotal += sent;
        bytes_left = bytes_to_send - sentTotal;
    }
    
    return sent;
}

int read_data(int *socket, char *buffer, int bytes_to_read)
{
    int read = 0;
    int read_total = 0;
    int bytes_left = bytes_to_read;
    
    while (read < bytes_to_read)
    {
        read = recv(*socket, buffer + read_total, bytes_left, MSG_WAITALL);
        if (read == -1)
        {
            return -1;
        }
        read_total += read;
        bytes_left = bytes_to_read - read_total;
    }
    
    return read;
}