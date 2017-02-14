//
// Created by shane on 2/13/17.
//

#ifndef COMP8005_ASSN2_ACCEPTOR_H
#define COMP8005_ASSN2_ACCEPTOR_H

#include "server.h"

/**
 * Starts accepting connections and relaying them to the provided server.
 *
 * @param server The server used to handle each connection.
 * @param port   The port on which to listen for connections.
 * @return 0 on success, or -1 on failure with errno set appropriately.
 */
int start_acceptor(server_t* server, short port);

#endif //COMP8005_ASSN2_ACCEPT_H
