
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

#ifndef _MIO_COLLECTION_H
#define _MIO_COLLECTION_H

//#include "mio_node.h"
#include "mio.h"
typedef struct mio_collection mio_collection_t;

struct mio_collection {
    char *node;
    char *name;
    mio_collection_t *next;
};

// Collection struct utilities
mio_collection_t *mio_collection_new();
void mio_collection_free(mio_collection_t *collection);
mio_collection_t *mio_collection_tail_get(mio_collection_t *collection);
void mio_collection_add(mio_packet_t *pkt, char *node, char *name);

int mio_collection_node_create(mio_conn_t * conn, const char *node,
                               const char *name, mio_response_t * response);
int mio_collection_node_configure(mio_conn_t * conn, const char *node,
                                  mio_stanza_t *collection_stanza, mio_response_t * response);
int mio_collection_config_child_add(mio_conn_t * conn, const char *child,
                                    mio_stanza_t *collection_stanza);
int mio_collection_config_parent_add(mio_conn_t * conn, const char *parent,
                                     mio_stanza_t *collection_stanza);
int mio_collection_children_query(mio_conn_t * conn, const char *node,
                                  mio_response_t * response);
int mio_collection_parents_query(mio_conn_t * conn, const char *node,
                                 mio_response_t * response);
int mio_collection_child_add(mio_conn_t * conn, const char *child,
                             const char *parent, mio_response_t *response);
int mio_collection_child_remove(mio_conn_t * conn, const char *child,
                                const char *parent, mio_response_t *response);
int mio_handler_collection_children_query(mio_conn_t * const conn,
        mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
int mio_handler_collection_parents_query(mio_conn_t * const conn,
        mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
int mio_handler_collection_children_query(mio_conn_t * const conn,
        mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
#endif
