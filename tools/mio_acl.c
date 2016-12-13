/******************************************************************************
 *  Mortar IO (MIO) Library Command Line Tools
 *  Query Affiliations Tool
 *	C Strophe Implementation
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
#include <stdlib.h>
#include <signal.h>
#include "mio.h"

mio_response_t *response = NULL, *meta_response = NULL;
mio_conn_t *conn;

void print_usage(char *prog_name) {
    fprintf(stdout,
            "\"%s\" Command line utility to interact with event node affilations.\n",
            prog_name);
    fprintf(stdout,
            "Usage: %s <-j jid> <-p password (optional)> <-c command> <-u user's jid> <-event event_node> [-verbose] [-stanza]  \n",
            prog_name);
    fprintf(stdout, "\t-event event_node = name of node to query\n");
    fprintf(stdout,
            "\t-j jid = JID (give the full JabberID, i.e. user@domain)\n");
    fprintf(stdout, "\t-p password = JID user password\n");
    fprintf(stdout, "\t-help = print this usage and exit\n");
    fprintf(stdout, "\t-verbose = print info\n");
    fprintf(stdout, "\t-server = server where event node resides, if not same as jid\n");
    fprintf(stdout, "\t-stanza = only print the affiliations stanza\n\n");
    fprintf(stdout, "Commands:\n");
    fprintf(stdout, "\t-r = remove\n");
    fprintf(stdout, "\t-a = add\n");
    fprintf(stdout, "\t-q = query\n\n");

    fprintf(stdout, "Options:\n");
    fprintf(stdout, "\t-affil [affiliaiton] = affiliation, publisher, outcast, none, onlypublish, owner, member\n");
    fprintf(stdout, "\t-group [group jid] = group to add to or remove from\n");
    fprintf(stdout, "\t-event [event id] = event node id\n");
    fprintf(stdout, "\t-u [user jid] = user to add or remove from acl\n");
}

typedef struct {
    char* event;
    char* password;
    char* jid;
    char* acl_jid;
    char* group;
    char* pubsub_server;
    char* xmpp_server;
    char command;
    int verbose;
    int affil;
    int stanza;
    int help;
} arg_t;

arg_t * parse_arguments(int argc, char** argv) {
    char *current_arg_name = NULL;
    char *current_arg_val = NULL;
    int current_arg_num = 1;
    int affil_set = 0;
    arg_t *args = malloc(sizeof(arg_t));
    memset(args,0x0,sizeof(arg_t));
    while (current_arg_num < argc) {
        current_arg_name = argv[current_arg_num];
        if (current_arg_num+1 < argc) {
            current_arg_val = argv[current_arg_num+1];
        }
        current_arg_num++;

        if (strcmp(current_arg_name, "-r") == 0) {
            args->command = 'a';
            args->affil = MIO_AFFILIATION_NONE;
            continue;
        }
        if (strcmp(current_arg_name, "-a") == 0) {
            args->command = 'a';
            continue;
        }
        if (strcmp(current_arg_name, "-q") == 0) {
            args->command = 'q';
            continue;
        }
        if (strcmp(current_arg_name, "-server") == 0) {
            args->pubsub_server = current_arg_val;
            continue;
        }
        if (strcmp(current_arg_name, "-affil") == 0) {
            current_arg_num++;
            affil_set = 1;
            if (strcmp(current_arg_val,"publisher") == 0) {
                args->affil = MIO_AFFILIATION_PUBLISHER;
            } else if (strcmp(current_arg_val,"none") == 0) {
                args->affil = MIO_AFFILIATION_NONE;
            } else if (strcmp(current_arg_val,"owner") == 0) {
                args->affil = MIO_AFFILIATION_OWNER;
            } else if (strcmp(current_arg_val,"outcast") == 0) {
                args->affil = MIO_AFFILIATION_OUTCAST;
            } else if (strcmp(current_arg_val,"onlypublish") == 0) {
                args->affil = MIO_AFFILIATION_PUBLISH_ONLY;
            } else if (strcmp(current_arg_val,"member") == 0) {
                args->affil = MIO_AFFILIATION_MEMBER;
            } else {
                free(args);
                fprintf(stderr,
                        "Unknown afill type. Optionsin are publisher, outcast, none, onlypubhlish, and member\n");
                return NULL;
            }
            affil_set = 1;
            continue;
        }
        if (strcmp(current_arg_name, "-group") == 0) {
            args->group = current_arg_val;
            current_arg_num++;
            continue;
        }
        if (strcmp(current_arg_name, "-help") == 0) {
            print_usage(argv[0]);
            return NULL;
        }
        if (strcmp(current_arg_name, "-verbose") == 0) {
            args->verbose = 1;
            continue;
        }
        if (strcmp(current_arg_name, "-stanza") == 0) {
            args->stanza = 1;
            continue;
        }
        if (strcmp(current_arg_name, "-c") == 0) {
            if (strcmp(current_arg_val, "remove") == 0) {
                args->command = 'r';
            } else if (strcmp(current_arg_val, "add") == 0) {
                args->command = 'a';
            } else if (strcmp(current_arg_val, "query") == 0) {
                args->command = 'q';
            }
        } else if (strcmp(current_arg_name,"-r")) {
            args->command = 'r';
        } else if (strcmp(current_arg_name,"-a")) {
            args->command = 'a';
        } else if (strcmp(current_arg_name,"-q")) {
            args->command = 'r';
        }
        if (strcmp(current_arg_name, "-event") == 0) {
            args->event = current_arg_val;
            current_arg_num++;
        } else if (strcmp(current_arg_name, "-u") == 0) {
            args->jid = current_arg_val;
            args->xmpp_server = mio_get_server(args->jid);
            if (args->xmpp_server == NULL ) {
                fprintf(stderr, "Invalid JID, use format user@domain\n");
                return NULL;
            }
            current_arg_num++;
        } else if (strcmp(current_arg_name, "-p") == 0) {
            args->password = current_arg_val;
            current_arg_num++;
        } else if (strcmp(current_arg_name, "-u") == 0) {
            args->acl_jid = current_arg_val;
            current_arg_num++;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", current_arg_name);
            print_usage(argv[0]);
            return NULL;
        }
    }

    if (args->jid == NULL ) {
        fprintf(stderr, "Username missing\n");
        print_usage(argv[0]);
        return NULL;
    } else if (args->password == NULL ) {
        fprintf(stdout, "%s's ", args->jid);
        fflush(stdout);
        args->password = getpass("password: ");
        if (args->password == NULL ) {
            fprintf(stderr, "Invalid password\n");
            print_usage(argv[0]);
            return NULL;
        }
    } else if (args->command == '\0') {
        fprintf(stderr, "Command is missing\n");
        print_usage(argv[0]);
        return NULL;
    } else if (args->command == 'r' && (args->acl_jid == NULL || args->event == NULL)) {
        fprintf(stderr, "User to remove is missing\n");
        print_usage(argv[0]);
        return NULL;
    } else if (args->command == 'a' && ((args->acl_jid == NULL && args->group == NULL) ||
                                        affil_set == 0 || args->event == NULL)) {
        fprintf(stderr, "Adding user requires username, affiliation, and event node.\n");
        print_usage(argv[0]);
        return NULL;
    }
    return args;
}

// Handler to catch SIGINT (Ctrl+C)
void int_handler(int s) {
    mio_disconnect(conn);
    mio_conn_free(conn);
    if (response != NULL )
        mio_response_free(response);
    if (meta_response != NULL )
        mio_response_free(meta_response);
    exit(1);
}

int main(int argc, char **argv) {
    char **buf;
    int err, buflen;
    xmpp_stanza_t *stanza, *affiliation;
    arg_t *args;

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
    args = parse_arguments(argc, argv);
    if (args->verbose) {
        fprintf(stdout, "XMPP Server: %s\n", args->xmpp_server);
        fprintf(stdout, "Username: %s\n", args->jid);
        fprintf(stdout, "Event Node: %s\n", args->event);
        fprintf(stdout, "\n");

        conn = mio_conn_new(MIO_LEVEL_DEBUG);
        err = mio_connect(args->jid, args->password, NULL, NULL, conn);
    } else {
        conn = mio_conn_new(MIO_LEVEL_ERROR);
        err = mio_connect(args->jid, args->password, NULL, NULL, conn);
    }
    if (err != MIO_OK) {
	fprintf(stdout, "Could not connect to XMPP server\n");
        mio_conn_free(conn);
        return err;
    }

    response = mio_response_new();
    if (args->command == 'q') {
        err = mio_acl_affiliations_query(conn, args->event, response);
        if (args->stanza != 0 && response->response_type != MIO_RESPONSE_PACKET) {
            stanza = xmpp_stanza_get_child_by_name(
                         response->stanza->xmpp_stanza->children, "affiliations");
            if (stanza != NULL ) {
                affiliation = xmpp_stanza_get_children(stanza);
                xmpp_stanza_to_text(affiliation, (char**) &buf, (size_t*) &buflen);
                fprintf(stdout, "%s\n", (char*) buf);
                free(buf);
            } else {
                fprintf(stderr, "Error reading affiliations\n");
            }
        }
    } else {
        err = mio_acl_affiliation_set(conn, args->event, args->acl_jid,
                                      args->affil, response);
    }
    mio_response_print(response);
    mio_response_free(response);
    mio_disconnect(conn);
    mio_conn_free(conn);
    return err;
}
