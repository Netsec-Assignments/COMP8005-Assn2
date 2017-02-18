/*********************************************************************************************
Name:			main.c

    Required:	main.h	

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Description:
    This is the client application. This will create the clients that will connect to the
	servers and will act as a basic echo server. The client will send and receive data and
	will test the time between to record stats.	

    Revisions:
    (none)

*********************************************************************************************/

#include <arpa/inet.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "client.h"
#include "protocol.h"

#define DEFAULT_PORT "8005"
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
    printf("usage: %s [-h] [-i ip] [-p port] [-m max] [-n clients]\n", name);
    printf("\t-h, --help:               print this help message and exit.\n");
    printf("\t-i, --ip [ip]             the ip on which the server is on.\n");
    printf("\t-p, --port [port]:        the port on which to listen for connections;\n");
    printf("\t-m, --max [max]           the max numbers of requests.\n");
    printf("\t-n, --clients [clients]   the number of clients to create.\n");
    printf("\t                          default port is %s.\n", DEFAULT_PORT);
    printf("\t                          default IP is %s.\n", DEFAULT_IP);
    printf("\t                          default number of clients is %d.\n", DEFAULT_NUMBER_CLIENTS);
    printf("\t                          default number of max requests is %d.\n", DEFAULT_MAXIMUM_REQUESTS);
}

/*********************************************************************************************
FUNCTION

    Name:	main

    Prototype:	int main(int argc, char** argv) 

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    argc - Number of arguments
	argv - Arguments

    Return Values:
	
    Description:
    Runs/creates the clients that will connect to the server. This function also accepts the
	user's inputs to customize the ip/port/max number of requests and the number of clients 
	to create.

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
    

    if (argc > 1)
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
                        size_t port_str_size = strlen(optarg);
                        char* port_copy = malloc(port_str_size + 1);
                        memcpy(port_copy, optarg, port_str_size);
                        port_copy[port_str_size] = 0;

                        client_datas.port = port_copy;
                    }
                }
                break;
                case 'i':
                {
                    size_t ip_str_size = strlen(optarg);
                    char* ip_copy = malloc(ip_str_size + 1);
                    memcpy(ip_copy, optarg, ip_str_size);
                    ip_copy[ip_str_size] = 0;

                    client_datas.ip = ip_copy;
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
                    fprintf (stderr, "Incorrect argument or unknown option. See %s -h for help.\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;
                default:
                    fprintf(stderr, "You broke getopt_long! Congrats.\n");
                    exit(EXIT_FAILURE);
                break;
            }
        }
    }

    if (start_client(client_datas) == -1)
    {
        exit(EXIT_FAILURE);
    }

    if (client_datas.port != DEFAULT_PORT)
    {
        free(client_datas.port);
    }
    if (client_datas.ip != DEFAULT_IP)
    {
        free(client_datas.ip);
    }
}

/*********************************************************************************************
FUNCTION

    Name:		start_client

    Prototype:	int start_client(client_info client_datas)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    client_datas - Server Data to connect to.

    Return Values:
	
    Description:
    Start the clients and connect to the server. Create the applicable number of threads dependent
	on the number of clients that the user wants to connect.

    Revisions:
	(none)

*********************************************************************************************/
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
        fprintf(stderr, "Unable to create socket pair.\n");
        return -1;
    }

    pthread_attr_init(&attribute);
    
    if (pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_DETACHED) != 0)
    {
        fprintf(stderr, "Thread Attributes unable to be set to detached.\n");
        return -1;
    }

    if (pthread_attr_setscope(&attribute, PTHREAD_SCOPE_SYSTEM) != 0)
    {
        fprintf(stderr,"System Scopen unable to be set to thread.\n");
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
            fprintf(stderr, "Unable to create thread.\n");
            return -1;
        }
    }
 
    pthread_attr_destroy(&attribute);
    
    wait(&count);

    return 1;
}


/*********************************************************************************************
FUNCTION

    Name:		connect_to_server

    Prototype:	void* clients(void* infos)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    infos - Server Data to connect to.

    Return Values:
	
    Description:
    Create the clients that the will connect to the server and send/receive data.

    Revisions:
	(none)

*********************************************************************************************/
void* clients(void* infos)
{
    int* sockets = 0;
    char* buffer = 0;
    ssize_t read = 0;
    int count = 0;
    int data_received = 0;
    int request_time = 0;
    int index = 0;
    int result = 0;
    char const* msg_send = "Hello, Mat!";
    unsigned char msg_recv[NETWORK_BUFFER_SIZE];
    client_info *data = (client_info *)infos;
    struct timeval start_time;
    struct timeval end_time;

    if ((buffer = malloc(sizeof(char) * NETWORK_BUFFER_SIZE)) == NULL)
    {
        fprintf(stderr, "Unable to allocate buffer memory.\n");
        return 0;
    }
    if ((sockets = malloc(sizeof(int) * data->num_of_clients)) == NULL)
    {
        fprintf(stderr, "Unable to allocate socket memory.\n");
        return 0;
    }

    for (index = 0; index < data->num_of_clients; index++)
    {      
        /* Create a socket and connect to the server */
        sockets[index] = connect_to_server(data->port, data->ip);

        /* Set the socket to reuse for improper shutdowns */
        if ((sockets[index] != -1) && (set_reuse(&sockets[index]) != -1))
        {
            break;
        }
    }

    // TODO: Change this to allow concurrent requests
    if (index != data->num_of_clients)
    {
        while (1)
        {
            count++;
            
            for (index = 0; index < data->num_of_clients; index++)
            {
                gettimeofday(&start_time, NULL);

                uint32_t msg_send_size = (uint32_t)strlen(msg_send);
                if (send_data(sockets[index], (char const*)&msg_send_size, sizeof(uint32_t)) == -1 ||
                    send_data(sockets[index], msg_send, strlen(msg_send)) == -1)
                {
                    continue;
                }

                if ((read = read_data(sockets[index], msg_recv, strlen(msg_send))) == -1)
                {
                    continue;
                }

                gettimeofday(&end_time, NULL);

                data_received += read;
                request_time += (end_time.tv_sec * 1000000 + end_time.tv_usec) -
                               (start_time.tv_sec * 1000000 + start_time.tv_usec);
                printf("%d\n", request_time);
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

/*********************************************************************************************
FUNCTION

    Name:		connect_to_server

    Prototype:	int connect_to_server(const char *port, const char *ip)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    port - Port to connect to server
	ip - IP to connect to server

    Return Values:
	
    Description:
    Connect to the server 

    Revisions:
	(none)

*********************************************************************************************/
int connect_to_server(const char *port, const char *ip)
{
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp = NULL;

    int sock = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, port, &hints, &result) != 0)
    {
        return -1;
    }
        
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        
        if (sock == -1)
            continue;
        
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
            break;    

        close(sock);
    }
    
    if (rp == NULL)
    {
        return -1;
    }
    
    freeaddrinfo(result);
    
    return sock;
}

/*********************************************************************************************
FUNCTION

    Name:		set_reuse

    Prototype:	int set_reuse(int* socket)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    socket - Socket

    Return Values:
	
    Description:
    Sets the socket for reuse.

    Revisions:
	(none)

*********************************************************************************************/
int set_reuse(int* socket)
{
    socklen_t optlen = 1;
    return setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, &optlen, sizeof(optlen));
}

/*********************************************************************************************
FUNCTION

    Name:		close_socket

    Prototype:	int close_socket(int* socket)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    socket - Socket

    Return Values:
	
    Description:
    Closes the socket.

    Revisions:
	(none)

*********************************************************************************************/
int close_socket(int* socket)
{
    return close(*socket);
}
