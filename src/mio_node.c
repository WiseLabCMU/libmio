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


#include <strophe.h>
#include <common.h>
#include "mio_connection.h"
#include "mio_schedule.h"
#include "mio_node.h"
#include <uuid/uuid.h>
#include "mio_error.h"
#include "mio_handlers.h"
#include "mio_transducer.h"
#include <mio_pubsub.h>
#include <mio_user.h>
#include <mio_meta.h>
#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

extern mio_log_level_t _mio_log_level;

int mio_handler_node_register(mio_conn_t * const conn,
                              mio_stanza_t * const stanza, mio_data_t * const mio_data);

int mio_handler_acl_affiliations_query(mio_conn_t * const conn,
                                       mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
int mio_handler_node_delete(mio_conn_t* const, mio_stanza_t* const,
                            mio_data_t* const);


/**
 * @ingroup Core
 * Gets the current time and date and returns them as a string.
 *
 * @returns Pointer to a string containing the current time and date. String needs to be freed when no longer in use.
 */
char *mio_timestamp_create() {

    char fmt[64], buf[64];
    struct timeval tv;
    struct tm *tm;
    char *time_str = NULL;

    gettimeofday(&tv, NULL );
    if ((tm = localtime(&tv.tv_sec)) != NULL ) {
        strftime(fmt, sizeof fmt, "%Y-%m-%dT%H:%M:%S.%%06u%z", tm);
        time_str = malloc(strlen(fmt) + 7);
        snprintf(time_str, sizeof(buf), fmt, tv.tv_usec);
        return time_str;
    }
    return NULL ;
}


/**
 * @ingroup Scheduler
 * Allocate and initialize a new mio event struct.
 *
 * @returns A pointer to a newly allocated and initialized mio event struct.
 */
mio_event_t * mio_event_new() {
    mio_event_t *event = malloc(sizeof(mio_event_t));
    memset(event, 0, sizeof(mio_event_t));
    event->id = -1;
    return event;
}


/**
 * @ingroup Scheduler
 * Frees an allocated mio event struct.
 *
 * @param event A pointer to the allocated mio event struct to be freed.
 */
void mio_event_free(mio_event_t *event) {
    if (event->info != NULL )
        free(event->info);
    if (event->time != NULL )
        free(event->time);
    if (event->tranducer_name != NULL )
        free(event->tranducer_name);
    if (event->transducer_value != NULL )
        free(event->transducer_value);
    if (event->recurrence != NULL )
        mio_recurrence_free(event->recurrence);
    free(event);
}


/**
 * @ingroup Scheduler
 * Returns the tail of a mio event linked list
 *
 * @param event A pointer to an element in a mio event linked list.
 * @returns A pointer to the tail of a mio event linked list.
 */
mio_event_t *mio_event_tail_get(mio_event_t *event) {
    mio_event_t *tail = NULL;

    while (event != NULL ) {
        tail = event;
        event = event->next;
    }
    return tail;
}

/** Publishes mio_stanza to event node over a mio connection.
 *
 * @param conn
 * @param item
 * @param node
 * @param response
 * */
int mio_item_publish(mio_conn_t *conn, mio_stanza_t *item, const char *node,
                     mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *publish = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process publish request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_set_stanza_new(conn, node);
    publish = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(publish, "publish");
    xmpp_stanza_set_attribute(publish, "node", node);

// Build xmpp message
    xmpp_stanza_add_child(publish, item->xmpp_stanza);
    xmpp_stanza_add_child(iq->xmpp_stanza->children, publish);

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
                            response);

// Release the stanza
    xmpp_stanza_release(publish);
    mio_stanza_free(iq);

    return err;
}

/** Publishes an item to an event node without waiting for a response
 *
 * @param conn Active mio connection.
 * @param item Mio stanza that contains the item to send.
 * @param node The target event node's uuid.
 * */
int mio_item_publish_nonblocking(mio_conn_t *conn, mio_stanza_t *item,
                                 const char *node) {
    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *publish = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process publish request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_set_stanza_new(conn, node);

    publish = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(publish, "publish");
    xmpp_stanza_set_attribute(publish, "node", node);

// Build xmpp message
    xmpp_stanza_add_child(publish, item->xmpp_stanza);
    xmpp_stanza_add_child(iq->xmpp_stanza->children, publish);

// Send out the stanza
    err = mio_send_nonblocking(conn, iq);

// Release the stanza
    xmpp_stanza_release(publish);
    mio_stanza_free(iq);

    return err;
}


/**
 *
 * @param item
 * @param id
 * @param type
 * @param description
 * @param serial_number
 * */
int mio_item_device_add(mio_stanza_t *item, const char *id, const char *type,
                        const char *description, const char *serial, const char *timestamp) {

    xmpp_stanza_t *Device = NULL;
    xmpp_stanza_t *device = NULL;
    int err, write_ns = 1;

// Check if input item stanza is NULL and return error if true
    if (item == NULL )
        return -1;

//Check if device node with same id already exists
    if (xmpp_stanza_get_children(item->xmpp_stanza) != NULL ) {
        device = xmpp_stanza_get_children(item->xmpp_stanza);
        while (device != NULL ) {
            if (strcmp(xmpp_stanza_get_name(device), "device") == 0) {
                // Check if MIO ns has already been added and do not add it again
                if (strcmp(xmpp_stanza_get_ns(device),
                           "http://jabber.org/protocol/mio") == 0)
                    write_ns = 0;
                if (strcmp(xmpp_stanza_get_attribute(device, "id"), id) == 0)
                    return -1;
            }

            device = xmpp_stanza_get_next(device);
        }
    }

// Create a new stanza for a device installation and set its attributes
    Device = xmpp_stanza_new(item->xmpp_stanza->ctx);
    if ((err = xmpp_stanza_set_name(Device, "device")) < 0)
        return err;
    if (write_ns) {
        if ((err = xmpp_stanza_set_ns(Device, "http://jabber.org/protocol/mio"))
                < 0)
            return err;
    }
    if ((err = xmpp_stanza_set_attribute(Device, "id", id)) < 0)
        return err;
    if (type != NULL ) {
        if ((err = xmpp_stanza_set_type(Device, type)) < 0)
            return err;
    }
    if (description != NULL ) {
        if ((err = xmpp_stanza_set_attribute(Device, "description", description))
                < 0)
            return err;
    }
    if (serial != NULL ) {
        if ((err = xmpp_stanza_set_attribute(Device, "serial", serial)) < 0)
            return err;
    }
    if ((err = xmpp_stanza_set_attribute(Device, "timestamp", timestamp)) < 0)
        return err;

// Attach device installation stanza as child of inputted stanza
    xmpp_stanza_add_child(item->xmpp_stanza, Device);

// Release created device installation stanza
    xmpp_stanza_release(Device);

    return 0;
}

/** Creates a new event node.
 *
 * @param conn Active MIO connection
 * @param node The node id of the event node. Should be uuid.
 * @param title The title of the event node.
 * @param access The access model of the event node
 * @param [out] response The response to the create node request
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on failed connection.
 * */
int mio_node_create(mio_conn_t * conn, const char *node, const char* title,
                    const char* access, mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *create = NULL;
    xmpp_stanza_t *title_field = NULL;
    xmpp_stanza_t *title_value = NULL;
    xmpp_stanza_t *title_val = NULL;
    xmpp_stanza_t *maxitems_field = NULL;
    xmpp_stanza_t *maxitems_value = NULL;
    xmpp_stanza_t *maxitems_val = NULL;
    xmpp_stanza_t *access_field = NULL;
    xmpp_stanza_t *access_value = NULL;
    xmpp_stanza_t *access_val = NULL;
    xmpp_stanza_t *configure = NULL;
    xmpp_stanza_t *x = NULL;
    xmpp_stanza_t *form_field = NULL;
    xmpp_stanza_t *form_field_value = NULL;
    xmpp_stanza_t *form_field_value_text = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process node creation request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_set_stanza_new(conn, node);

// Create configure stanza
    configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(configure, "configure");

// Create create node stanza
    create = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(create, "create");
    xmpp_stanza_set_attribute(create, "node", node);

// Create x stanza
    x = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(x, "x");
    xmpp_stanza_set_ns(x, "jabber:x:data");
    xmpp_stanza_set_type(x, "submit");

// Create form field of config 
        form_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
        form_field_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
        form_field_value_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(form_field_value, "value");
        xmpp_stanza_set_text(form_field_value_text,
                             "http://jabber.org/protocol/pubsub#node_config");
        xmpp_stanza_set_name(form_field, "field");
        xmpp_stanza_set_attribute(form_field, "var", "FORM_TYPE");
        xmpp_stanza_set_type(form_field, "hidden");
        xmpp_stanza_add_child(form_field_value, form_field_value_text);
        xmpp_stanza_add_child(form_field, form_field_value);
 

        maxitems_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
        maxitems_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
        maxitems_val = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(maxitems_field, "field");
        xmpp_stanza_set_attribute(maxitems_field, "var", "pubsub#max_items");
        xmpp_stanza_set_name(maxitems_value, "value");
        err = xmpp_stanza_set_text(maxitems_val, "500");

// Build max+items field
        xmpp_stanza_add_child(maxitems_value, maxitems_val);
        xmpp_stanza_add_child(maxitems_field, maxitems_value);

// Release unneeded stanzas
        xmpp_stanza_release(maxitems_val);
        xmpp_stanza_release(maxitems_value);
 
    if (title != NULL ) {
// Create title field and value stanza
        title_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
        title_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
        title_val = xmpp_stanza_new(conn->xmpp_conn->ctx);
       xmpp_stanza_set_name(title_field, "field");
        xmpp_stanza_set_attribute(title_field, "var", "pubsub#title");
        xmpp_stanza_set_name(title_value, "value");
        err = xmpp_stanza_set_text(title_val, title);

// Build title field
       xmpp_stanza_add_child(title_value, title_val);
        xmpp_stanza_add_child(title_field, title_value);

// Release unneeded stanzas
        xmpp_stanza_release(title_val);
        xmpp_stanza_release(title_value);
    }

    if (access != NULL ) {
// Create access field and value stanza
        access_field = xmpp_stanza_new(conn->xmpp_conn->ctx);
        access_value = xmpp_stanza_new(conn->xmpp_conn->ctx);
        access_val = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(access_field, "field");
        xmpp_stanza_set_attribute(access_field, "var", "pubsub#access_model");
        xmpp_stanza_set_name(access_value, "value");
        xmpp_stanza_set_text(access_val, access);

// Build access field
        xmpp_stanza_add_child(access_value, access_val);
        xmpp_stanza_add_child(access_field, access_value);

// Release unneeded stanzas
        xmpp_stanza_release(access_val);
        xmpp_stanza_release(access_value);
    }

    xmpp_stanza_add_child(x, form_field);
    xmpp_stanza_add_child(x, maxitems_field);
// Build xmpp message
    if (access != NULL || title != NULL ) {
        if (title != NULL ) {
            xmpp_stanza_add_child(x, title_field);
            // Release unneeded stanza
            xmpp_stanza_release(title_field);
        }

        if (access != NULL ) {
            err = xmpp_stanza_add_child(x, access_field);
            // Release unneeded stanza
            xmpp_stanza_release(access_field);
        }

        xmpp_stanza_add_child(configure, x);
    }
    if (create) {
        xmpp_stanza_add_child(iq->xmpp_stanza->children, create);
        xmpp_stanza_release(create);
    }

//TODO: BAD WAY OF DOING THIS
    if (access != NULL || title != NULL ) {
        xmpp_stanza_add_child(iq->xmpp_stanza->children, configure);

// Release unneeded stanzas
        xmpp_stanza_release(x);
        xmpp_stanza_release(configure);
    }

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
                            response);

    if (form_field != NULL )
        xmpp_stanza_release(form_field);
    if (form_field_value != NULL )
        xmpp_stanza_release(form_field_value);
    if (form_field_value_text != NULL )
        xmpp_stanza_release(form_field_value_text);
// Release the stanzas
    mio_stanza_free(iq);

    return err;
}



mio_stanza_t *mio_node_config_new(mio_conn_t * conn, const char *node,
                                  mio_node_type_t type) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *configure = NULL, *x = NULL, *field_form = NULL, *value_ns =
            NULL, *field_pubsub = NULL, *value_collection = NULL,
                   *value_ns_text = NULL, *value_collection_text = NULL;

// Create stanzas
    iq = mio_pubsub_set_stanza_new(conn, node);

    configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(configure, "configure");
    xmpp_stanza_set_attribute(configure, "node", node);

    x = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(x, "x");
    xmpp_stanza_set_ns(x, "jabber:x:data");
    xmpp_stanza_set_attribute(x, "type", "submit");

    field_form = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(field_form, "field");
    xmpp_stanza_set_attribute(field_form, "var", "FORM_TYPE");
    xmpp_stanza_set_attribute(field_form, "type", "hidden");

    value_ns = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(value_ns, "value");

    value_ns_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_text(value_ns_text,
                         "http://jabber.org/protocol/pubsub#node_config");
    xmpp_stanza_add_child(x, field_form);

    if (type != MIO_NODE_TYPE_UNKNOWN) {
        field_pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(field_pubsub, "field");
        xmpp_stanza_set_attribute(field_pubsub, "var", "pubsub#node_type");

        value_collection = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(value_collection, "value");

        value_collection_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
        if (type == MIO_NODE_TYPE_COLLECTION)
            xmpp_stanza_set_text(value_collection_text, "collection");
        else
            xmpp_stanza_set_text(value_collection_text, "leaf");

        xmpp_stanza_add_child(value_collection, value_collection_text);
        xmpp_stanza_add_child(field_pubsub, value_collection);
        xmpp_stanza_add_child(x, field_pubsub);
    }
// Build xmpp message
    xmpp_stanza_add_child(value_ns, value_ns_text);
    xmpp_stanza_add_child(field_form, value_ns);
    xmpp_stanza_add_child(configure, x);
    xmpp_stanza_add_child(iq->xmpp_stanza->children, configure);

    return iq;
}



int mio_node_type_query(mio_conn_t * conn, const char *node,
                        mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *query = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process node type query request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_iq_get_stanza_new(conn, node);

// Create query stanza
    query = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#info");
    xmpp_stanza_set_attribute(query, "node", node);

// Build xmpp message
    xmpp_stanza_add_child(iq->xmpp_stanza, query);

// Send out the stanza
    err = mio_send_blocking(conn, iq,
                            (mio_handler) mio_handler_node_type_query, response);

// Release the stanzas
    xmpp_stanza_release(query);
    mio_stanza_free(iq);

    return err;
}

/** Delete an event node by the event node's id.
 *
 * @param conn Active MIO connection
 * @param node Event node id of node to delete.
 * @param response Response to deletion request.
 *
 * @returns SOX_OK on success, SOX_ERROR_DISCONNECTED if connection disconnected.
 * */
int mio_node_delete(mio_conn_t * conn, const char *node,
                    mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *delete = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process node deletion request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_set_stanza_new(conn, node);
    xmpp_stanza_set_ns(iq->xmpp_stanza->children,
                       "http://jabber.org/protocol/pubsub#owner");

// Create delete stanza
    delete = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(delete, "delete");
    xmpp_stanza_set_attribute(delete, "node", node);

// Build xmpp message
    xmpp_stanza_add_child(iq->xmpp_stanza->children, delete);

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
                            response);

// Release the stanzas
    xmpp_stanza_release(delete);
    mio_stanza_free(iq);

    return err;
}


/** Registers an event node.
 *
 * @param conn An active MIO connection.
 * @param new_node The node id of the new event node. Should be a UUID.
 * @param creation_node The node id of the creation node listener.
 * @param response The response to the node registration request.
 * */
int mio_node_register(mio_conn_t *conn, const char *new_node,
                      const char *creation_node, mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *publish = NULL;
    xmpp_stanza_t *item = NULL;
    xmpp_stanza_t *data = NULL;
    xmpp_stanza_t *node_register = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process node registration request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_set_stanza_new(conn, creation_node);

    publish = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(publish, "publish");
    xmpp_stanza_set_attribute(publish, "node", creation_node);

// Create a new item stanzas
    item = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(item, "item");
    xmpp_stanza_set_attribute(item, "id", "current");

// Create a new data stanza
    data = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(data, "data");
    xmpp_stanza_set_ns(data, "http://jabber.org/protocol/mio#service");

// Create a new node register
    node_register = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(node_register, "nodeRegister");
    xmpp_stanza_set_attribute(node_register, "id", new_node);

// Build xmpp message
    xmpp_stanza_add_child(data, node_register);
    xmpp_stanza_add_child(item, data);
    xmpp_stanza_add_child(publish, item);
    xmpp_stanza_add_child(iq->xmpp_stanza->children, publish);

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
                            response);

// Release the stanzas
    xmpp_stanza_release(node_register);
    xmpp_stanza_release(data);
    xmpp_stanza_release(item);
    xmpp_stanza_release(publish);
    mio_stanza_free(iq);

    return err;
}

