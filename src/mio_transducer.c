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


#include "strophe.h"
#include "common.h"
#include "mio_transducer.h"
#include "mio_connection.h"
#include "mio_node.h"
#include "mio_handlers.h"
#include <mio_pubsub.h>
#include <mio_user.h>

extern mio_log_level_t _mio_log_level;

/**
 * @ingroup PubSub
 *
 * Allocates and initializes a new mio data strict.
 *
 * @returns The newly allocated and initialized mio data struct.
 */
mio_data_t *mio_data_new() {
    mio_data_t *data = malloc(sizeof(mio_data_t));
    memset(data,0x0,sizeof(mio_data_t));
    return data;
}

/**
 * @ingroup PubSub
 * Frees an allocated mio data struct.
 *
 * @param data A pointer to the allocated mio data struct to be freed.
 */
void mio_data_free(mio_data_t *data) {
    mio_transducer_data_t *t, *curr_t;
    curr_t = (mio_transducer_data_t*) data->transducers;
    while (curr_t != NULL ) {
        t = curr_t->next;
        mio_transducer_data_free(curr_t);
        curr_t = t;
    }
    if (data->event != NULL )
        free(data->event);
    free(data);
}

int mio_publish_data(mio_conn_t *conn, mio_data_t *data, mio_response_t *response) {
    int i, err;
    mio_transducer_data_t *tran_tmp;
    mio_stanza_t *data_item;
    mio_response_t *response_tmp = NULL;
    tran_tmp = data->transducers;

    for (i = 0; i < data->num_transducers; i++) {
        if (i == (data->num_transducers-1) ) {
             if (response_tmp != NULL) mio_response_free(response_tmp);
             response_tmp = response;
        } else {
             if(response_tmp != NULL)mio_response_free(response_tmp);
             response_tmp = mio_response_new();
        }
        data_item = mio_transducer_data_to_item(conn,tran_tmp);
        err = mio_item_publish_data(conn, data_item, data->event,response_tmp);
        if (err != MIO_OK) {
            mio_response_free(response);
            response = response_tmp;
            return err;
        }
        mio_stanza_free(data_item);
        tran_tmp = tran_tmp->next;
    }
    return MIO_OK; 
}

mio_stanza_t* mio_transducer_data_to_item(mio_conn_t *conn, mio_transducer_data_t* transducer) {
    xmpp_stanza_t *t = NULL, *head_item = NULL, *tail_item = NULL, *item = NULL;
    mio_stanza_t *items;
    mio_transducer_data_t *curr;
    char *t_string;

    if(transducer == NULL)
        return NULL;

    // Iterate over linked list of transducers, converting them to stanzas
    for(curr = transducer; curr != NULL; curr = curr->next) {
        // Create a new item of type transducer_transducername
        t_string = malloc(sizeof(char)*strlen(curr->name) + 2);
        sprintf(t_string, "_%s", curr->name);
        item = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(item, "item");
        xmpp_stanza_set_id(item, t_string);
        xmpp_stanza_set_ns(item, "http://jabber.org/protocol/mio");

        // Populate transducer stanza
        t = xmpp_stanza_new(conn->xmpp_conn->ctx);
        if(curr->type == MIO_TRANSDUCER_DATA)
            xmpp_stanza_set_name(t, "transducerData");
        else
            xmpp_stanza_set_name(t, "transducerSetData");
        xmpp_stanza_set_attribute(t, "name", curr->name);
        xmpp_stanza_set_attribute(t, "value", curr->value);
        xmpp_stanza_set_attribute(t, "timestamp", curr->timestamp);

        // Add child stanzas and clean up
        xmpp_stanza_add_child(item, t);

        if(tail_item == NULL) {
            tail_item = item;
            head_item = item;
        }
        else {
            tail_item->next = item;
            tail_item = item;
        }

        free(t_string);
        xmpp_stanza_release(t);
        // todo: fix iteration as only one data item sent at a time
        break;
    }
    items = mio_stanza_new(conn);
    xmpp_stanza_release(items->xmpp_stanza);
    items->xmpp_stanza = head_item;

    return items;
}

/** Adds transducer value to data item. Can specify the value type
 *       of the data.
 *
 * @param d_meta
 * @param t_meta
 *  */
int mio_data_transducer_data_add(mio_data_t *data,
                                 mio_transducer_data_type_t type, char *name, char *value,
                                 char *timestamp) {

    mio_transducer_data_t *t;

    if (data->transducers == NULL ) {
        data->transducers = mio_transducer_data_new();
        t = data->transducers;
    } else {
        t = _mio_transducer_data_tail_get(data->transducers);
        t->next = mio_transducer_data_new();
        t = t->next;
    }

    t->type = type;
    if (name != NULL ) {
        t->name = malloc(strlen(name) + 1);
        strcpy(t->name, name);
    }
    if (value != NULL ) {
        t->value = malloc(strlen(value) + 1);
        strcpy(t->value, value);
    }
    if (timestamp != NULL ) {
        t->timestamp = malloc(strlen(timestamp) + 1);
        strcpy(t->timestamp, timestamp);
    }

    data->num_transducers++;
    return MIO_OK;
}


/** Publishes the data item to the given connection. Allows node specification
 * and response.
 *
 * @param conn Active connection associated with publishing jid.
 * @param t_meta
 *  */
int mio_item_publish_data(mio_conn_t *conn, mio_stanza_t *item,
                          const char *node, mio_response_t * response) {
    int err;

    err = mio_item_publish(conn, item, node, response);
    return err;
}
/**
 * @ingroup PubSub
 * Allocates and initializes a new mio transducer value struct.
 *
 * @returns A pointer to the newly allocated and initialized mio transducer value struct.
 */
mio_transducer_data_t * mio_transducer_data_new() {
    mio_transducer_data_t *t_value = malloc(sizeof(mio_transducer_data_t));
    memset(t_value,0,sizeof(mio_transducer_data_t));
    return t_value;
}

/**
 * @ingroup PubSub
 * Frees a mio transducer value struct. Does not free any successive members in the linked list.
 *
 * @param A pointer to the allocated mio transducer value struct to be freed.
 */
void mio_transducer_data_free(mio_transducer_data_t *t_value) {
    if (t_value->name != NULL)
        free(t_value->name);
    if (t_value->value != NULL)
        free(t_value->value);
    if (t_value->timestamp != NULL)
        free(t_value->timestamp);
    free(t_value);
}

/**
 * @ingroup PubSub
 * Returns the tail of a mio transducer value linked list
 *
 * @param A pointer to an element in a mio transducer value linked list.
 * @returns A pointer to the tail of a mio transducer value linked list.
 */
mio_transducer_data_t *_mio_transducer_data_tail_get(
    mio_transducer_data_t *t_value) {
    mio_transducer_data_t *tail = NULL;

    while (t_value != NULL ) {
        tail = t_value;
        t_value = t_value->next;
    }
    return tail;
}



void _mio_xml_data_update_end_element(mio_xml_parser_data_t *xml_data,
                                      const char* element_name) {
    if (xml_data->prev_depth > xml_data->curr_depth) {
        xml_data->parent = xml_data->parent->prev;
        mio_xml_parser_parent_data_free(xml_data->parent->next);
        xml_data->parent->next = NULL;
        if(xml_data->parent->parent == NULL)
            mio_xml_parser_parent_data_free(xml_data->parent);
    }
    xml_data->prev_depth = xml_data->curr_depth;
    xml_data->curr_depth--;
}

mio_xml_parser_parent_data_t *mio_xml_parser_parent_data_new() {
    mio_xml_parser_parent_data_t *data = malloc(
            sizeof(mio_xml_parser_parent_data_t));
    memset(data, 0, sizeof(mio_xml_parser_parent_data_t));
    return data;
}

void mio_xml_parser_parent_data_free(mio_xml_parser_parent_data_t *data) {
    free(data);
}

int mio_transducer_data_add(mio_transducer_data_t *target, mio_transducer_data_t *transducer) {
    mio_transducer_data_t *curr, *prev = NULL;

    if(transducer->name == NULL)
        return MIO_ERRROR_TRANSDUCER_NULL_NAME;
    if(transducer->value == NULL)
        return MIO_ERROR_TRANSDUCER_NULL_VALUE;
    // Add timestamp if it is missing
    if(transducer->timestamp == NULL)
        transducer->timestamp = mio_timestamp_create();

    // Traverse linked list of transducers to see if a transducer with an identical name is already present, if not add one to the list
    for(curr = target; curr != NULL; curr = curr->next) {
        if(curr->name != NULL) {
            if(strcmp(curr->name, transducer->name) == 0)
                break;
        }
        prev = curr;
    }
    // Insert transducer
    if(prev != NULL)
        prev->next = transducer;
    if(curr != NULL) {
        transducer->next = curr->next;
        mio_transducer_data_free(curr);
    }

    return MIO_OK;
}

