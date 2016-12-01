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
            "Usage: %s <-event event_node> <-name meta_name> <-type meta_type> [-info meta_info] <-j username> <-p password> [-verbose]\n",
            prog_name);
    fprintf(stdout, "Usage: %s -help\n", prog_name);
    fprintf(stdout, "\t-event event_node = name of event node to publish to\n");
    fprintf(stdout, "\t-name meta_name = name of meta information to add\n");
    fprintf(stdout,
            "\t-type meta_type = type of meta information to add: either \"device\" or \"location\"\n");
    fprintf(stdout,
            "\t-info meta_info = description of the meta information to add\n");
    fprintf(stdout,
            "\t-j username = JID (give the full JID, i.e. user@domain)\n");
    fprintf(stdout, "\t-p password = JID user password\n");
    fprintf(stdout, "\t-help = print this usage and exit\n");
    fprintf(stdout, "\t-verbose = print info\n");
    fprintf(stdout, "\t-a = adds meta information if not already present.\n");
    fprintf(stdout,
            "\t-c = change meta information, creates if no meta information present\n");
    fprintf(stdout,
            "\t-q = query meta information\n");
    fprintf(stdout,
            "\t-cop = copies meta information from template.\n");
    fprintf(stdout,
            "\t-template = query meta information\n");
    fprintf(stdout,
            "\t-unit transducer_unit = units of output of transducer to add\n");
    fprintf(stdout,
            "\t-enum_names enumeration_unit_names = comma separated names of enum unit values e.g. \"red,green,blue\"\n");
    fprintf(stdout,
            "\t-enum_names enumeration_unit_values = comma separated values of enum unit values e.g. \"0,1,2\"\n");
    fprintf(stdout,
            "\t-min transducer_min_value = minimum output value of transducer to add\n");
    fprintf(stdout,
            "\t-min transducer_max_value = maximum output value of transducer to add\n");
    fprintf(stdout,
            "\t-resolution transducer_resolution = resolution of output of transducer to add\n");
    fprintf(stdout,
            "\t-precision transducer_precision = precision of output of transducer to add\n");
    fprintf(stdout,
            "\t-accuracy transducer_precision = accuracy of output of transducer to add\n");
    fprintf(stdout,
            "\t-serial transducer_serial_number = serial number of the transducer to add\n");
    fprintf(stdout,
            "\t-manufacturer transducer_manufacturer = manufacturer of the transducer to add\n");
    fprintf(stdout,
            "\t-info trans_info = description of the transducer to add\n");
    fprintf(stdout,
            "\t-interface trans_interface = interface of the transducer to add\n");


    fprintf(stdout,
            "\t-accuracy accuracy = horizontal GPS error in meters (decimal) e.g. \"10\"\n");
    fprintf(stdout,
            "\t-alt altitude = altitude in meters above or below sea level (decimal) e.g. \"1609\"\n");
    fprintf(stdout,
            "\t-area area = a named area such as a campus or neighborhood e.g. \"Central Park\"\n");
    fprintf(stdout,
            "\t-bearing bearing = GPS bearing (direction in which the entity is heading to reach its next waypoint), measured in decimal degrees relative to true north (decimal) e.g. \"-07:00\"\n");
    fprintf(stdout,
            "\t-building building = a specific building on a street or in an area e.g. \"The Empire State Building\"\n");
    fprintf(stdout,
            "\t-country country = the nation where the device/transducer is located e.g. \"United States\"\n");
    fprintf(stdout,
            "\t-country_code country_code = the ISO 3166 two-letter country code e.g. \"US\"\n");
    fprintf(stdout, "\t-datum datum = GPS datum e.g. \"-07:00\"\n");
    fprintf(stdout,
            "\t-description description = a natural-language name for or description of the location e.g. \"Bill's house\"\n");
    fprintf(stdout,
            "\t-floor floor = a particular floor in a building e.g. \"102\"\n");
    fprintf(stdout,
            "\t-lat latitude = latitude in decimal degrees north (decimal) e.g. \"39.75\"\n");
    fprintf(stdout,
            "\t-locality locality = a locality within the administrative region, such as a town or city e.g. \"New York City\"\n");
    fprintf(stdout,
            "\t-lon longitude = longitude in decimal degrees East (decimal) e.g. \"-104.99\"\n");
    fprintf(stdout,
            "\t-zip zip_code = a code used for postal delivery e.g. \"10118\"\n");
    fprintf(stdout,
            "\t-region region = an administrative region of the nation, such as a state or province e.g. \"New York\"\n");
    fprintf(stdout,
            "\t-room room = a particular room in a building e.g. \"Observatory\"\n");
    fprintf(stdout,
            "\t-speed speed = the speed at which the device/transducer is moving, in meters per second (decimal) e.g. \"52.69\"\n");
    fprintf(stdout,
            "\t-street street = a thoroughfare within the locality, or a crossing of two thoroughfares e.g. \"350 Fifth Avenue / 34th and Broadway\"\n");
    fprintf(stdout,
            "\t-text text = a catch-all element that captures any other information about the location e.g. \"Northwest corner of the lobby\"\n");
    fprintf(stdout,
            "\t-tzo time_zone_offset = the time zone offset from UTC for the current location e.g. \"-07:00\"\n");
    fprintf(stdout,
            "\t-uri uri = a URI or URL pointing to a map of the location e.g. \"http://www.esbnyc.com/map.jpg\"\n");
}

void int_handler(int s) {
    mio_disconnect(conn);
    mio_conn_free(conn);
    if (response != NULL) {
        mio_response_free(response);
    }
    exit(1);
}

int main(int argc, char **argv) {
    char *username = NULL;
    char *password = NULL;
    char *xmpp_server = NULL;
    char pubsub_server[80];
    int xmpp_server_port = 5223;
    char *event_node = NULL;
    mio_meta_t *meta ;
    int verbose = 0;
    int transducer_meta = 0;
    int current_arg_num = 1;
    char *current_arg_name = NULL;
    char *current_arg_val = NULL;
    char *template = NULL;
    int command = 0;
    int transducer = 0;
    struct sigaction sig_int_handler;
    mio_response_t *response;
    meta = mio_meta_new();
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

        if (strcmp(current_arg_name, "-verbose") == 0) {
            verbose = 1;
            continue;
        }

        if (current_arg_num == argc) {
            print_usage(argv[0]);
            return -1;
        }

        current_arg_val = argv[current_arg_num++];

        if (strcmp(current_arg_name, "-transducer") == 0) {
            transducer_meta = 1;
        }
        if (strcmp(current_arg_name, "-location") == 0) {
            transducer_meta = 1;
        }

        if (strcmp(current_arg_name, "-c") == 0) {
            command = 'c';
        } else if (strcmp(current_arg_name, "-q") == 0) {
            command = 'q';
        } else if (strcmp(current_arg_name, "-a") == 0) {
            command = 'a';
        }

        if (strcmp(current_arg_name, "-event") == 0) {
            event_node = current_arg_val;
        } else if (strcmp(current_arg_name, "-name") == 0) {
            meta->name = malloc(sizeof(char) * (strlen(current_arg_val) + 1));
            strcpy(meta->name, current_arg_val);
        } else if (strcmp(current_arg_name, "-info") == 0) {
            meta->info = malloc(sizeof(char) * (strlen(current_arg_val) + 1));
            strcpy(meta->info, current_arg_val);
        } else if (strcmp(current_arg_name, "-type") == 0) {
            if (strcmp(current_arg_val, "location") == 0)
                meta->meta_type = MIO_META_TYPE_LOCATION;
            if (strcmp(current_arg_val, "device") == 0)
                meta->meta_type = MIO_META_TYPE_DEVICE;
            if (strcmp(current_arg_val, "gateway") == 0)
                meta->meta_type = MIO_META_TYPE_GATEWAY;
            if (strcmp(current_arg_val, "adapter") == 0)
                meta->meta_type = MIO_META_TYPE_ADAPTER;
        } else if (strcmp(current_arg_name, "-j") == 0) {
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
        } else if (strcmp(current_arg_name, "-template") == 0) {
            template = current_arg_val;
        } else if (strcmp(current_arg_name, "-unit") == 0) {

        } else if (strcmp(current_arg_name, "-max_value") == 0) {

        } else if (strcmp(current_arg_name, "-min_value") == 0) {

        } else if (strcmp(current_arg_name, "-type") == 0) {

        } else if (strcmp(current_arg_name, "-pn") == 0) {

        } else if (strcmp(current_arg_name, "-pv") == 0) {

        } else if (strcmp(current_arg_name, "-resolution") == 0) {

        } else if (strcmp(current_arg_name, "-accuracy") == 0) {

        } else if (strcmp(current_arg_name, "-interface") == 0) {

        } else if (strcmp(current_arg_name, "-manufacturer") == 0) {

        } else if (strcmp(current_arg_name, "-precision") == 0) {

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
    } else if (meta->name == NULL ) {
        fprintf(stderr, "Meta name missing\n");
        print_usage(argv[0]);
        return -1;
    } else if (meta->meta_type == MIO_META_TYPE_UKNOWN) {
        fprintf(stderr, "Meta type missing or invalid\n");
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
    meta->timestamp = mio_timestamp_create();
    if (command == 'r') {
        if (!transducer) {
            err = mio_meta_merge_publish(conn, event_node, meta, NULL, NULL, response);
        } else {
            err = mio_meta_merge_publish(conn, event_node, meta, NULL, NULL, response);
        }
    } else if (command == 'c') {
        err = mio_meta_publish(conn, event_node, meta, response);
    } else if (command == 'q') {
        err = mio_meta_query(conn, event_node, response);
    }

    mio_response_print(response);
    mio_response_free(response);
    mio_meta_free(meta);
    mio_disconnect(conn);
    mio_conn_free(conn);
    return MIO_OK;
}
