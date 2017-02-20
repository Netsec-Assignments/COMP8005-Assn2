//
// Created by shane on 2/19/17.
//

#include <fcntl.h>
#include <stdio.h>
#include <aio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "log.h"

static int log_fd;

static void handle_signal(int sig)
{
    fsync(log_fd);
    log_close();
}

int log_open(char const* name)
{
    log_fd = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_NONBLOCK);
    if (log_fd < 0)
    {
        perror("open");
        return -1;
    }
    return 0;
}

int log_msg(char const *message)
{
    struct aiocb cb;
    memset(&cb, 0, sizeof(cb));
    cb.aio_buf = (void*)message;
    cb.aio_fildes = log_fd;
    cb.aio_nbytes = strlen(message);

    if (aio_write(&cb) == -1)
    {
        perror("aio_write");
        log_close();
        return -1;
    }

    int error;
    while((error = aio_error(&cb)) == EINPROGRESS);

    if (error)
    {
        perror("aio_write");
        log_close();
        return -1;
    }

    return 0;
}

int log_close()
{
    return close(log_fd);
}