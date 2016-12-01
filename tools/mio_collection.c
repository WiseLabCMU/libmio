/******************************************************************************
 *  Mortar IO (MIO) Library Command Line Tools
 *  Collection Node Children Query Tool
 *  C Strophe Implementation
 *  Copyright (C) 2014, Carnegie Mellon University
 *  All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2.0 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contributing Authors (specific to this file):
 *  Patrick Lazik
 *******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strophe.h>
#include <common.h>
#include <mio.h>
#include <mio_handlers.h>
#include <stdlib.h>
#include <signal.h>

mio_conn_t *conn;
mio_response_t *response = NULL;

void print_usage(char *prog_name) {
    fprintf(stdout,
            "\"%s\" Command line utility to retrieve the children of a collection node\n",
            prog_name);
    fprintf(stdout,
            "Usage: %s <-collection collection_node> <-j username> <-p password> [-verbose] -a -r \n",
            prog_name);
    fprintf(stdout, "Usage: %s -help\n", prog_name);
    fprintf(stdout,
            "\t-collection collection_node = id of collection node to query\n");
    fprintf(stdout,
            "\t-child = id of node to add or remove to collection\n");
    fprintf(stdout,
            "\t-parent = id of node of parent collection\n");
    fprintf(stdout,
            "\t-j username = JID (give the full JID, i.e. user@domain)\n");
    fprintf(stdout, "\t-p password = JID user password\n");
    fprintf(stdout, "\t-help = print this usage and exit\n");
    fprintf(stdout, "\t-verbose = print info\n");
    fprintf(stdout, "\t-title = name of collection node when creating\n");
    fprintf(stdout, "\t-acm = access control model of collection node\n");

    fprintf(stdout, "\t-a = add child node with id child id \n");
    fprintf(stdout, "\t-c = create collection node with collection id\n");
    fprintf(stdout, "\t-r = remove child node with id child id\n");
    fprintf(stdout, "\t-q = query collection node with collection id\n");
}

// Handler to catch SIGINT (Ctrl+C)
void int_handler(int s) {
    mio_disconnect(conn);
    mio_conn_free(conn);
    if (response != NULL )
        mio_response_free(response);
    exit(1);
}

int main(int argc, char **argv) {
    char *username = NULL;
    char *password = NULL;
    char *xmpp_server = NULL;
    char pubsub_server[80];

    int verbose = 0;

    char command = 0;
    int current_arg_num = 1;
    char *child;
    char *parent;
    char *title;
    char *current_arg_name = NULL;
    char *current_arg_val = NULL;
    char *collection_node = NULL;
    int err;
    int stanza = 0;

    struct sigaction sig_int_handler;

    // Add SIGINT handler
    sig_int_handler.sa_handler = int_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL );

    if (argc == 1 || !strcmp(argv[1], "-help")) {
        print_usage(argv[0]);
        return -1;
    }

    while (current_arg_num < argc) {
        current_arg_name = argv[current_arg_num];
        if (strcmp(current_arg_name, "-help") == 0) {
            print_usage(argv[0]);
            return -1;
        }
        if (strcmp(current_arg_name, "-verbose") == 0) {
            verbose = 1;
        }
        if (current_arg_num == argc) {
            print_usage(argv[0]);
            return -1;
        }
        current_arg_val = argv[current_arg_num+1];
        if (strcmp(current_arg_name, "-collection") == 0) {
            collection_node = current_arg_val;
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-j") == 0) {
            username = current_arg_val;
            xmpp_server = mio_get_server(username);
            if (xmpp_server == NULL ) {
                fprintf(stderr, "Invalid JID, use format user@domain\n");
                return MIO_ERROR_INVALID_JID;
            }
            strcpy(pubsub_server, "pubsub.");
            strcat(pubsub_server, xmpp_server);
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-p") == 0) {
            password = current_arg_val;
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-a") == 0) {
            command = 'a';
        } else if (strcmp(current_arg_name, "-r") == 0) {
            command = 'r';
        } else if (strcmp(current_arg_name, "-q") == 0) {
            command = 'r';
        } else if (strcmp(current_arg_name, "-c") == 0) {
            command = 'c';
        } else if (strcmp(current_arg_name, "-title") == 0) {
            title = current_arg_val;
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-child") == 0) {
            child = current_arg_val;
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-parent") == 0) {
            parent = current_arg_val;
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-stanza") == 0) {
            stanza = 1;
        } else if (strcmp(current_arg_name, "-verbose") == 0) {
            verbose = 1;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", current_arg_name);
            print_usage(argv[0]);
            return -1;
        }
	current_arg_num++;
    }

    if (username == NULL ) {
        fprintf(stderr, "Username missing\n");
        print_usage(argv[0]);
        return -1;
    } else if (collection_node == NULL ) {
        fprintf(stderr, "Collection Node missing\n");
        print_usage(argv[0]);
        return -1;
    } else if (password == NULL ) {
        fprintf(stdout, "%s's ", username);
        fflush(stdout);
        password = getpass("password: ");
        if (password == NULL ) {
            fprintf(stderr, "Invalid password\n");
            print_usage(argv[0]);
            return -1;
        }
    } else if (command == 0) {
        fprintf(stderr, "No command provided\n");
        return -1;
    } else if (command == 'c' && title == NULL) {
        fprintf(stderr, "No title for node provided\n");
        return -1;
    }

    if (verbose) {
        conn = mio_conn_new(MIO_LEVEL_DEBUG);
        err = mio_connect(username, password, NULL, NULL, conn);
    } else {
        conn = mio_conn_new(MIO_LEVEL_ERROR);
        err = mio_connect(username, password, NULL, NULL, conn);
    }

    if (err != MIO_OK) { 
	    mio_conn_free(conn);
	    fprintf(stdout, "Could not connect to xmpp server.");
	    return err;
    }
    response = mio_response_new();
    if (command == 'q') {
        err = mio_collection_children_query(conn, collection_node, response);
        mio_response_print(response);
        mio_response_free(response);
        response = mio_response_new();
        err = mio_collection_parents_query(conn, collection_node, response);
    } else if (command == 'c') {
        err = mio_collection_node_create(conn, collection_node, title,
                                         response);
    } else if (command == 'a') {
            err = mio_collection_child_add(conn, child, parent,
                                       response);
    } else if (command == 'r'){  
        err = mio_collection_child_remove(conn, child, parent,
                                          response);
    } else {
        fprintf(stderr, "Unrecognized command\n");
	err = !MIO_OK;
    }
    if (err != MIO_OK) {
        fprintf(stderr, "Something went wrong\n");
    }

    mio_response_print(response);
    mio_response_free(response);

    mio_disconnect(conn);
    mio_conn_free(conn);
    return err;
}
