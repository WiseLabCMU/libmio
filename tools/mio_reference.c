/******************************************************************************
 *  Mortar IO (MIO) Library Command Line Tools
 *  Publish to Event Node Tool
 * 	C Strophe Implementation
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

int err = 0;

mio_conn_t *conn;
mio_response_t *response;

void print_usage(char *prog_name) {
    fprintf(stdout,
            "Usage: %s <-parent parent_event_node> <-child child_event_node> <-child_type child_node_type> <-parent_type parent_node_type> [-add_ref_child] <-u username> <-p password> [-verbose]\n",
            prog_name);
    fprintf(stdout, "Usage: %s -help\n", prog_name);
    fprintf(stdout,
            "\t-parent parent_event_node = name of parent event node to add to child\n");
    fprintf(stdout,
            "\t-a = add reference\n");
    fprintf(stdout,
            "\t-r = remove reference\n");
    fprintf(stdout,
            "\t-q = query references\n");
    fprintf(stdout,
            "\t-child child_event_node = name of child event node to add to parent\n");
    fprintf(stdout,
            "\t-add_ref_child = adds a reference to the parent at the child node if possible\n");
    fprintf(stdout,
            "\t-u username = JID (give the full JID, i.e. user@domain)\n");
    fprintf(stdout, "\t-p password = JID user password\n");
    fprintf(stdout, "\t-help = print this usage and exit\n");
    fprintf(stdout, "\t-verbose = print info\n");
}

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
    int xmpp_server_port = 5223;
    char *parent = NULL;
    char *child = NULL;
    char command = '\0';

    int verbose = 0, add_ref_child = 0;

    int current_arg_num = 1;
    char *current_arg_name = NULL;
    char *current_arg_val = NULL;

    struct sigaction sig_int_handler;
    mio_response_t *response;

    // Add SIGINT handler
    sig_int_handler.sa_handler = int_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL );

    while (current_arg_num < argc) {
        current_arg_name = argv[current_arg_num++];
        if (strcmp(current_arg_name, "-help") == 0) {
            print_usage(argv[0]);
            return -1;
        }
        if (strcmp(current_arg_name, "-add_ref_child") == 0) {
            add_ref_child = 1;
            continue;
        }
        if (strcmp(current_arg_name, "-verbose") == 0) {
            verbose = 1;
            continue;
        }
        if (strcmp(current_arg_name, "-r") == 0) {
            command = 'r';
            continue;
        }
        if (strcmp(current_arg_name, "-q") == 0) {
            command = 'q';
            continue;
        }
        if (strcmp(current_arg_name, "-a") == 0) {
            command = 'a';
            continue;
        }
        if (current_arg_num == argc) {
            print_usage(argv[0]);
            return -1;
        }
        current_arg_val = argv[current_arg_num++];

        if (strcmp(current_arg_name, "-parent") == 0) {
            parent = current_arg_val;
        } else if (strcmp(current_arg_name, "-child") == 0) {
            child = current_arg_val;
        } else if (strcmp(current_arg_name, "-u") == 0) {
            username = current_arg_val;
            xmpp_server = mio_get_server(username);
            if (xmpp_server == NULL ) {
                fprintf(stderr, "Invalid JID, use format user@domain\n");
                return MIO_ERROR_INVALID_JID;
            }
            strcpy(pubsub_server, "pubsub.");
            strcat(pubsub_server, xmpp_server);
        } else if (strcmp(current_arg_name, "-p") == 0) {
            password = current_arg_val;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", current_arg_name);
            print_usage(argv[0]);
            return -1;
        }
    }

    if (username == NULL ) {
        fprintf(stderr, "Username missing\n");
        print_usage(argv[0]);
        return -1;
    } else if (parent == NULL ) {
        fprintf(stderr, "Parent node missing\n");
        print_usage(argv[0]);
        return -1;
    } else if (child == NULL ) {
        fprintf(stderr, "Child node missing\n");
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
    }

    if (verbose) {
        fprintf(stdout, "XMPP Server: f%s\n", xmpp_server);
        fprintf(stdout, "XMPP Server Port: %d\n", xmpp_server_port);
        fprintf(stdout, "XMPP PubSub Server: %s\n", pubsub_server);
        fprintf(stdout, "Username: %s\n", username);
        fprintf(stdout, "Parent: %s\n", parent);
        fprintf(stdout, "Child: %s\n", child);
        fprintf(stdout, "Verbose: YES\n");
        fprintf(stdout, "\n");

        conn = mio_conn_new(MIO_LEVEL_DEBUG);
        mio_connect(username, password, NULL, NULL, conn);

    } else {
        conn = mio_conn_new(MIO_LEVEL_ERROR);
        mio_connect(username, password, NULL, NULL, conn);
    }

    response = mio_response_new();
    if(command == 'a') { 
        if (add_ref_child) { 
            err = mio_reference_child_add(conn, parent, child, MIO_ADD_REFERENCE_AT_CHILD, response);
        } else { 
            err = mio_reference_child_add(conn, parent, child, MIO_NO_REFERENCE_AT_CHILD, response);
        }
    } else if (comand ='r') { 
    
    } else if (command == 'q') 
        err = mio_reference_child_add(conn, parent, child, MIO_NO_REFERENCE_AT_CHILD, response);

    mio_response_print(response);
    mio_response_free(response);
    mio_disconnect(conn);
    mio_conn_free(conn);
    return MIO_OK;
}

