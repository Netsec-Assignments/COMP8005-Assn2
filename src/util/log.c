/*********************************************************************************************
Name:			log.c

    Required:	log.h

    Developer:  Shane Spoor

    Created On: 2017-02-17

    Description:
    Handles the reading/writing to a log file for the server.

    Revisions:
    (none)

*********************************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <aio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "log.h"

static int log_fd;

/*********************************************************************************************
FUNCTION

    Name:		handle_signal

    Prototype:	static void handle_signal(int sig)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    sig - Signal from the server.

    Return Values:
	
    Description:
    Handle the signal from the server

    Revisions:
	(none)

*********************************************************************************************/
static void handle_signal(int sig)
{
    fsync(log_fd);
    log_close();
}

/*********************************************************************************************
FUNCTION

    Name:		log_open

    Prototype:	int log_open(char const* name)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    name - name of the log file

    Return Values:
	
    Description:
    Open a log file to write to.

    Revisions:
	(none)

*********************************************************************************************/
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

/*********************************************************************************************
FUNCTION

    Name:		log_msg

    Prototype:	int log_msg(char const *message)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    message - Message to write to the file.

    Return Values:
	
    Description:
    Create the log msg

    Revisions:
	(none)

*********************************************************************************************/
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

/*********************************************************************************************
FUNCTION

    Name:		log_close

    Prototype:	int log_close()

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:

    Return Values:
	
    Description:
    Closes the log file

    Revisions:
	(none)

*********************************************************************************************/
int log_close()
{
    return close(log_fd);
}