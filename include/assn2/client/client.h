

#ifndef COMP8005_ASSN2_CLIENT_H
#define COMP8005_ASSN2_CLIENT_H

typedef struct
{
    char* ip;
    int port;
    unsigned int num_of_clients;
    unsigned int max_requests;
} client_info;

int start_client(client_info client_datas);
void print_usage();
void *clients(void *info);
int read_data(int *socket, char *buffer, int bytes_to_read);
int send_data(int *socket, const char *buffer, int bytes_to_send);
int close_socket(int* socket);
int set_reuse(int* socket);
int connect_to_server(const char *port, int *sock, const char *ip);

#endif