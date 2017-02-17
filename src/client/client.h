

#ifndef COMP8005_ASSN2_CLIENT_H
#define COMP8005_ASSN2_CLIENT_H

typedef struct
{
    char* ip;
    int port;
    unsigned int numOfClients;
    unsigned int maxRequests;
} client_info;

int start_client(client_info client_datas);
void print_usage();
void *clients(void *info);

int sendData(int *socket, const char *buffer, int bytesToSend);
int closeSocket(int* socket);
int setReuse(int* socket);
int connectToServer(const char *port, int *sock, const char *ip);

#endif