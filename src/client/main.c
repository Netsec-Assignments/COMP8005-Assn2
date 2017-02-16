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
#include "assn2/client.h"

#define DEFAULT_PORT 8005

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
void print_usage()
{
    printf("usage: %s [-h] [-p port]\n");
    printf("\t-h, --help:          print this help message and exit.\n");
    printf("\t-p, --port [port]:   the port on which to listen for connections;\n");
    printf("\t                     default is %u.\n", DEFAULT_PORT);
    printf("\t-c, --client: the client used to send connections.\n");
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
    unsigned short port = DEFAULT_PORT;
    int client = 0;
    char const* short_opts = "p:c:h";
    struct option long_opts[] =
    {
        {"port",   1, NULL, 'p'},
        {"client", 1, NULL, 'c'},
        {"help",   0, NULL, 'h'},
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
                        port = (unsigned short)port_int;
                    }
                }
                break;
                case 'c':
                {
                    client = 1;
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

        if (start_acceptor(client, port) == -1)
        {
            exit(EXIT_FAILURE);
        }
    }
}
