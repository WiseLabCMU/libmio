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

#ifndef ____mio_pubsub__
#define ____mio_pubsub__

#include <strophe.h>
#include <common.h>
#include <mio.h>
//#include <mio_user.h>
//#include <mio_affiliations.h>
//#include <stdio.h>
//#include <mio_connection.h>
//#include <mio_transducer.h>
//#include <mio_error.h>

// pubsub item functinos
int mio_item_recent_get(mio_conn_t* conn, const char *node,
                        mio_response_t * response, int max_items, const char *item_id,
                        mio_handler *handler);
int mio_items_recent_get(mio_conn_t* conn, const char *node,
                         mio_response_t * response, int max_items, char **item_ids,int item_count,
                         mio_handler *handler);


mio_stanza_t *mio_pubsub_item_new(mio_conn_t *conn, char* item_type);
mio_stanza_t *mio_pubsub_item_data_new(mio_conn_t *conn);
mio_stanza_t *mio_pubsub_item_interface_new(mio_conn_t *conn, char* interface);
mio_stanza_t *mio_pubsub_stanza_new(mio_conn_t *conn, const char *node);
mio_stanza_t *mio_pubsub_iq_get_stanza_new(mio_conn_t *conn,
        const char *node);
mio_stanza_t *mio_pubsub_iq_stanza_new(mio_conn_t *conn,
                                       const char *node);
int mio_handler_pubsub_data_receive(mio_conn_t * const conn,
                                    mio_stanza_t * const stanza, mio_response_t *response, void *userdata);

mio_stanza_t *mio_pubsub_set_stanza_new(mio_conn_t *conn,
                                        const char *node) ;
int mio_send_nonblocking(mio_conn_t *conn, mio_stanza_t *stanza);
int mio_send_blocking(mio_conn_t *conn, mio_stanza_t *stanza,
                      mio_handler handler, mio_response_t * response) ;

void XMLCALL mio_XMLstart_pubsub_data_receive(void *data,
        const char *element_name, const char **attr);

mio_stanza_t *mio_pubsub_get_stanza_new(mio_conn_t *conn,
                                        const char *node);
#endif /* defined(____mio_pubsub__) */
