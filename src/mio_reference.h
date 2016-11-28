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


#ifndef ____mio_reference__
#define ____mio_reference__

#include <stdio.h>
#include <mio.h>

typedef struct mio_reference mio_reference_t;

typedef enum {
    MIO_REFERENCE_UNKOWN, MIO_REFERENCE_CHILD, MIO_REFERENCE_PARENT
} mio_reference_type_t;

enum {
    MIO_NO_REFERENCE_AT_CHILD, MIO_ADD_REFERENCE_AT_CHILD
};

struct mio_reference {
    mio_reference_type_t type;
    char *node;
    char *name;
    mio_meta_type_t meta_type;
    mio_reference_t *next;
};


mio_reference_t *mio_reference_new();
void mio_reference_free(mio_reference_t *ref);
mio_reference_t *mio_reference_tail_get(mio_reference_t *ref);
mio_stanza_t *mio_references_to_item(mio_conn_t* conn, mio_reference_t *ref);

void mio_reference_add(mio_conn_t* conn, mio_reference_t *refs,
                       mio_reference_type_t type, mio_meta_type_t meta_type, char *node);

mio_reference_t *mio_reference_remove(mio_conn_t* conn, mio_reference_t *refs,
                                      mio_reference_type_t type, char *node);
int mio_reference_child_remove(mio_conn_t *conn, char* parent, char *child,
                               mio_response_t *response);
int mio_reference_child_add(mio_conn_t *conn, char* parent, char *child,
                            int add_reference_at_child, mio_response_t *response);
int mio_references_query(mio_conn_t *conn, char *node, mio_response_t *response);
int _mio_reference_meta_type_overwrite_publish(mio_conn_t *conn,
        char *node, char *ref_node, mio_reference_type_t ref_type,
        mio_meta_type_t ref_meta_type, mio_response_t *response);
int mio_handler_references_query(mio_conn_t * const conn,
                                 mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
int mio_reference_update(mio_conn_t *conn, char* event_node_id);
#endif /* defined(____mio_reference__) */
