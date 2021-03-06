/******************************************************************************
 *  Mortar IO (MIO) Library Command Line Tools
 *  Get last data from event node
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
 *  Ricardo Lopes Pereira
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
            "\"%s\" Command line utility to read most recent data item from an event node\n",
            prog_name);
    fprintf(stdout,
            "Usage: %s <-event event_node> [-max max_items] [-id item_id] <-u username> <-p password> [-verbose]\n",
            prog_name);
    fprintf(stdout, "Usage: %s -help\n", prog_name);
    fprintf(stdout, "\t-event event_node = name of node to get data from\n");
    fprintf(stdout,
            "\t-u username = JID (give the full JID, i.e. user@domain)\n");
    fprintf(stdout, "\t-p password = JID user password\n");
    fprintf(stdout,
            "\t-max max_items = maximum number of items to retrieve if no item_id is specified\n");
    fprintf(stdout, "\t-id item_id = item id of the item to be retrieved\n");
    fprintf(stdout, "\t-help = print this usage and exit\n");
    fprintf(stdout, "\t-verbose = print info\n");
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
    char **item_holder = NULL;
    char pubsub_server[80];
    int xmpp_server_port = 5223;
    xmpp_stanza_t *item;

    int verbose = 0;

    int current_arg_num = 1;
    char *current_arg_name = NULL;
    char *current_arg_val = NULL;
    char *cur_item = NULL;
    char *event_node = NULL;
    char *item_id = NULL;
    char *buf;
    int buflen;

    int max_items = 10;
    int err;
    int stanza = 0;
    int item_count = 0;
    int n_items = 500;


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
        current_arg_name = argv[current_arg_num++];

        if (strcmp(current_arg_name, "-help") == 0) {
            print_usage(argv[0]);
            return -1;
        }

        if (strcmp(current_arg_name, "-max_items") == 0) {
            max_items = atoi(argv[current_arg_num++]);
            return -1;
        }

        if (strcmp(current_arg_name, "-verbose") == 0) {
            verbose = 1;
            continue;
        }

        //if (current_arg_num == argc) {
        //    print_usage(argv[0]);
        //    return -1;
        //}

        current_arg_val = argv[current_arg_num];

        if (strcmp(current_arg_name, "-event") == 0) {
            event_node = current_arg_val;
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-u") == 0) {
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
        } else if (strcmp(current_arg_name, "-id") == 0) {
            item_id = current_arg_val;
	    current_arg_num++;
        } else if (strcmp(current_arg_name, "-stanza") == 0) {
            stanza = 1;
        } else if (strcmp(current_arg_name, "-max") == 0) {
            max_items = atoi(current_arg_val);
	    current_arg_num++;
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
    } else if (event_node == NULL ) {
        fprintf(stderr, "Event Node missing\n");
        print_usage(argv[0]);
        return -1;
    } else if (password == NULL ) {
        fprintf(stdout, "%s's ", username);
        fflush(stdout);
        password = getpass("password: ");
        if (password == NULL ) {
            fprintf(stderr, "Invalid password\n");
            print_usage(argv[0]);
        }
    }

    if (verbose) {
        fprintf(stdout, "XMPP Server: %s\n", xmpp_server);
        fprintf(stdout, "XMPP Server Port: %d\n", xmpp_server_port);
        fprintf(stdout, "XMPP PubSub Server: %s\n", pubsub_server);
        fprintf(stdout, "Username: %s\n", username);
        fprintf(stdout, "Event Node: %s\n", event_node);
        fprintf(stdout, "Verbose: YES\n");
        fprintf(stdout, "\n");

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

    if (!stanza) {
        if (item_id != NULL ) {
            if (strcmp(item_id, "meta") == 0)
                err = mio_item_recent_get(conn, event_node, response, max_items,
                                          item_id, (mio_handler *) mio_handler_meta_query);
            else if (strcmp(item_id, "references") == 0)
                err = mio_item_recent_get(conn, event_node, response, max_items,
                                          item_id, (mio_handler*) mio_handler_references_query);
            else if (strcmp(item_id, "schedule") == 0)
                err = mio_item_recent_get(conn, event_node, response, max_items,
                                          item_id, (mio_handler*) mio_handler_schedule_query);
            else
                err = mio_item_recent_get(conn, event_node, response, max_items,
                                          item_id, NULL);
        } else
            err = mio_item_recent_get(conn, event_node, response, max_items, NULL,
                                      NULL );
        if (err == MIO_OK) {
            mio_response_print(response);
        }
    } else {
        cur_item = strtok(item_id, ",");
        item_count = 0;
	item_holder = malloc(n_items);
        while (cur_item) {
            item_holder[item_count] = cur_item;
            cur_item = strtok(NULL,",");
            item_count++;
            if (item_count > n_items) {
                n_items *= 2;
                realloc(item_holder, n_items);
                if (item_holder == NULL) {
                    break;
                }
            }
        }
        response = mio_response_new();
        err = mio_items_recent_get(conn, event_node, response, max_items, item_holder, item_count ,
                                   (mio_handler*) mio_handler_error);
        if (err != MIO_OK) {
            mio_response_free(response);
        }

        if ((response->stanza == NULL) ||
            (response->stanza->xmpp_stanza == NULL) ||
            (response->stanza->xmpp_stanza->children== NULL)) {
        } else if (err == MIO_OK) {
            if (item_count == 1) {
                item = xmpp_stanza_get_child_by_name(
                           response->stanza->xmpp_stanza->children->children, "item");
            } else {
                item = xmpp_stanza_get_child_by_name(
                           response->stanza->xmpp_stanza->children, "items");
            }
	    buf = malloc(sizeof(buflen));
            if (item != NULL ) {
                xmpp_stanza_to_text(item, (char**) &buf, (size_t*) &buflen);
                fprintf(stdout, "%s", (char*) buf);
                //free(buf);
            }
        }

    }
    mio_response_free(response);
    mio_disconnect(conn);
    mio_conn_free(conn);
    return err;
}
