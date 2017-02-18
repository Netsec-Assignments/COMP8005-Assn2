//
// Created by shane on 2/18/17.
//
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include "protocol.h"

ssize_t send_data(int sock, void const *buffer, size_t bytes_to_send)
{
    ssize_t bytes_sent = 0;
    ssize_t sent_total = 0;
    size_t bytes_left = bytes_to_send;

    unsigned char const* raw = (unsigned char const*)buffer;

    while (sent_total < bytes_to_send)
    {
        bytes_sent = send(sock, raw + sent_total, bytes_left, 0);
        if (bytes_sent == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                return sent_total;
            }
            else
            {
                return -1;
            }
        }
        sent_total += bytes_sent;
        bytes_left = bytes_to_send - sent_total;
    }

    return sent_total;
}

ssize_t read_data(int sock, void *buffer, size_t bytes_to_read)
{
    ssize_t bytes_read = 0;
    ssize_t read_total = 0;
    size_t bytes_left = bytes_to_read;

    unsigned char* raw = (unsigned char*)buffer;

    while (read_total < bytes_to_read)
    {
        bytes_read = recv(sock, raw + read_total, bytes_left, MSG_WAITALL);
        if (bytes_read == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                return read_total;
            }
            return -1;
        }
        read_total += bytes_read;
        bytes_left = bytes_to_read - read_total;
    }

    return read_total;
}