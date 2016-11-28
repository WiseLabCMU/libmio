/******************************************************************************
 *  Mortar IO (MIO) Library
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


#ifndef ____mio_data__
#define ____mio_data__

#include <stdio.h>
//#include <mio_connection.h>
//#include <mio_packet.h>
//#include "mio_connection.h"
//#include <mio_packet.h>
#include <mio.h>

typedef enum {
    MIO_TRANSDUCER_DATA, MIO_TRANSDUCER_SET_DATA
} mio_transducer_data_type_t;

typedef struct mio_transducer_data mio_transducer_data_t;

struct mio_transducer_data {
    mio_transducer_data_type_t type;
    char *name;
    char *value;
    char *timestamp;
    mio_transducer_data_t *next;
};

typedef struct mio_data {
    char *event;
    int num_transducers;
    mio_transducer_data_t *transducers;
} mio_data_t;

//stanza
int mio_handler_node_register(mio_conn_t * const conn,
                              mio_stanza_t * const stanza, mio_data_t * const mio_data);

mio_data_t *mio_data_new();
void mio_data_free(mio_data_t *data);
void mio_transducer_data_free(mio_transducer_data_t *t_value);
int mio_data_transducer_data_add(mio_data_t *data,
                                 mio_transducer_data_type_t type, char *name, char *value,
                                 char *timestamp);
mio_transducer_data_t *_mio_transducer_data_tail_get(
    mio_transducer_data_t *t_value);

mio_transducer_data_t *_mio_transducer_data_tail_get(
    mio_transducer_data_t *t_value);

int mio_publish_data(mio_conn_t *conn, mio_data_t *data,
                     mio_response_t *response);
int mio_item_transducer_data_actuate_add(mio_stanza_t *item,
        const char *deviceID, const char *value, const char *timestamp);
int mio_item_transducer_data_add(mio_stanza_t *item, const char *deviceID,
                                 const char *id, const char *value,
                                 const char *timestamp);
int mio_item_publish_data(mio_conn_t *conn, mio_stanza_t *item,
                          const char *node, mio_response_t * response);

void _mio_subscription_add(mio_packet_t *pkt, char *subscription, char *sub_id);

mio_transducer_data_t * mio_transducer_data_new();
void mio_transducer_data_free(mio_transducer_data_t *t_value);

int mio_pubsub_data_receive(mio_conn_t *conn, mio_response_t *response);
int mio_pubsub_data_listen_start(mio_conn_t *conn);
int mio_pubsub_data_listen_stop(mio_conn_t *conn);
void mio_pubsub_data_rx_queue_clear(mio_conn_t *conn);
mio_stanza_t* mio_transducer_data_to_item(mio_conn_t *conn, mio_transducer_data_t* transducer);
int mio_transducer_data_add(mio_transducer_data_t *target, mio_transducer_data_t *transducer);

#endif /* defined(____mio_data__) */
