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
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "client.h"
#include "protocol.h"

#define DEFAULT_PORT "8005"
#define DEFAULT_IP "192.168.0.9"
#define DEFAULT_NUMBER_CLIENTS 5000
#define DEFAULT_MAXIMUM_REQUESTS 1
#define NETWORK_BUFFER_SIZE 1024

static atomic_int thread_count = 0;

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
    printf("usage: %s [-h] [-i ip] [-p port] [-m max] [-n clients] [-t threads]\n", name);
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
    char const* short_opts = "i:p:m:n:t:h";
	int file_descriptors[2];
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
	
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, file_descriptors) == -1)
    {
        fprintf(stderr, "Unable to create socket pair.\n");
        exit(EXIT_FAILURE);
    }

    if(!fork())
    {
        //SEND TO OTHER PROCESS TO HANDLE DATA TO PRINT TO FILE
        //USE file_descriptors[1]
        record_result(file_descriptors[1], client_datas.num_of_clients);
        return 0;
    }

    client_datas.file_descriptor = file_descriptors[0];

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
    int threads = client_datas.num_of_clients;
    int count = 0;
    client_info* data = malloc(sizeof(client_info) * client_datas.num_of_clients);
    pthread_t thread = 0;
    pthread_attr_t attribute;

    atomic_store_explicit(&thread_count, client_datas.num_of_clients, memory_order_relaxed);

    if (data == NULL)
    {
        perror("malloc");
        return -1;
    }
        
    for (count = 0; count < threads; count++)
    {
        memcpy(&data[count], &client_datas, sizeof(client_datas));
    }

    for (count = 0; count < threads; count++)
    {
        if (pthread_create(&thread, NULL, clients, (void *) &data[count]) != 0)
        {
            fprintf(stderr, "Unable to create thread.\n");
            return -1;
        }
        pthread_detach(thread);
    }
 
    printf("All threads created, waiting for them to finish now...\n");
    int old_remaining = client_datas.num_of_clients;
    int remaining;
    while(remaining = atomic_load_explicit(&thread_count, memory_order_relaxed))
    {
        if (remaining != old_remaining)
        {
            printf("Number of threads left: %d\n", remaining);
            old_remaining = remaining;
        }
    }
    printf("All threads finished. Exiting\n");

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
    int sock = 0;
    char* buffer = 0;
    int data_received = 0;
    int request_time = 0;
    char const* msg_send = "0123456789012345678901234567890123456789012345678901234567980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879801234568798012345687980123456879";
    unsigned char msg_recv[4096];
    unsigned char result_info[4096]; //not sure about this 
    client_info *data = (client_info *)infos;
    
    struct timeval start_time;
    struct timeval end_time;

    // if ((buffer = malloc(sizeof(char) * NETWORK_BUFFER_SIZE)) == NULL)
    // {
    //     fprintf(stderr, "Unable to allocate buffer memory.\n");
    //     return 0;
    // }

    sock = connect_to_server(data->port, data->ip);
    if (sock == -1)
    {
        atomic_fetch_sub(&thread_count, 1);
        return NULL;
    }
    else if (set_reuse(&sock) == -1)
    {
        atomic_fetch_sub(&thread_count, 1);
        shutdown(sock, 0);
        close_socket(&sock);
        return NULL;
    }

    for (int i = 0; i < data->max_requests; i++)
    {
        gettimeofday(&start_time, NULL);

        uint32_t msg_send_size = (uint32_t)strlen(msg_send);
        if (send_data(sock, (char const*)&msg_send_size, sizeof(uint32_t)) == -1 ||
            send_data(sock, msg_send, strlen(msg_send)) == -1)
        {
            continue;
        }

        ssize_t bytes_read;
        if ((bytes_read = read_data(sock, msg_recv, strlen(msg_send))) == -1)
        {
            continue;
        }

        gettimeofday(&end_time, NULL);

        data_received += bytes_read;
        request_time += (end_time.tv_sec * 1000000 + end_time.tv_usec) -
                        (start_time.tv_sec * 1000000 + start_time.tv_usec);
        
    }

	sprintf(result_info, "Total Request Time: %d and Total Data Received: %d\n", request_time,  data_received);
    // fflush(stdout);

    if(send_data(data->file_descriptor, result_info, strlen(result_info) == -1))
    {
        fprintf(stderr, "Unable to send results to socket.\n"); 
        return 0;
    }

    close_socket(&sock);
    // free(buffer);   
    atomic_fetch_sub(&thread_count, 1); 
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
        perror("getaddrinfo");
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
        perror("connect");
        freeaddrinfo(result);
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

/*********************************************************************************************
FUNCTION

    Name:		record_result

    Prototype:	void record_result(int socket)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    socket - Socket

    Return Values:
	
    Description:
    Records the results from the socket pair.

    Revisions:
	(none)

*********************************************************************************************/
int record_result(int socket, int number_of_clients)
{
    char *buffer;
    int fd = 0;
    int count = 0;

    if((buffer = malloc(sizeof(char) * 4096)) == NULL)
    {
        fprintf(stderr, "Unable to allocate buffer.\n");
        return -1;
    }

    if((fd = open("./result/result.txt", O_WRONLY | O_CREAT, 0755)) == -1)
    {
        fprintf(stderr, "Unable to open file.\n");
        return -1;
    }

    while(1)
    {
        if(write(fd, buffer, count) == -1)
        {
            fprintf(stderr, "Unable to write to file.\n");
            return -1;
        }
    }
    
    close(fd);
    close(socket);
}