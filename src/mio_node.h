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


#ifndef MIO_NODE_H
#define MIO_NODE_H

#include <stdio.h>

//#include <mio_connection.h>
//#include <mio_handlers.h>
#include <mio.h>

typedef enum {
    MIO_NODE_TYPE_UNKNOWN,
    MIO_NODE_TYPE_LEAF,
    MIO_NODE_TYPE_COLLECTION,
    MIO_NODE_TYPE_EVENT
} mio_node_type_t;




char *mio_timestamp_create();

int mio_node_create(mio_conn_t * conn, const char *node, const char* title,
                    const char* access, mio_response_t * response);
int mio_node_info_query(mio_conn_t * conn, const char *node,
                        mio_response_t * response);
int mio_node_delete(mio_conn_t * conn, const char *node,
                    mio_response_t * response);

int mio_node_register(mio_conn_t *conn, const char *new_node,
                      const char *creation_node, mio_response_t * response);

int mio_node_type_query(mio_conn_t * conn, const char *node,
                        mio_response_t * response);


mio_stanza_t *mio_node_config_new(mio_conn_t * conn, const char *node,
                                  mio_node_type_t type);

int mio_item_publish(mio_conn_t *conn, mio_stanza_t *item, const char *node,
                     mio_response_t * response);
#endif /* defined(____mio_node__) */
