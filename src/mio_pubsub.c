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

#include <expat.h>
#include <stdio.h>
#include <common.h>
#include <strophe.h>
#include <mio_error.h>
#include <mio_connection.h>
#include <mio_user.h>
#include <mio_pubsub.h>
#include <errno.h>

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

extern mio_log_level_t _mio_log_level;

int mio_handler_publish(mio_conn_t * const conn, mio_stanza_t * const stanza,
                        mio_response_t *response, void *userdata);
mio_stanza_t *_mio_pubsub_get_stanza_new(mio_conn_t *conn,
        const char *node);
mio_stanza_t *_mio_pubsub_stanza_new(mio_conn_t *conn, const char *node);
/** Indicates to XMPP server that conn should receive pubsub data.
 *
 *  @param conn Active MIO connection.
 *
 *  @returns SOX_OK on success and MIO_ERROR_DISCONNECTED if no connection to server.
 *  */
int mio_pubsub_data_listen_start(mio_conn_t *conn) {
    mio_request_t *request = NULL;
    int err;
    // Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot start listening for data since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }
    // Tell the sever that we can receive publications if we haven't already done so
    if (conn->presence_status != MIO_PRESENCE_PRESENT)
        mio_listen_start(conn);

    request = _mio_request_new();
    strcpy(request->id, "pubsub_data_rx");

    // If we have too many open requests, wait
    sem_wait(conn->mio_open_requests);

    err = mio_handler_add(conn, (mio_handler) mio_handler_pubsub_data_receive,
                          "http://jabber.org/protocol/pubsub#event", NULL, NULL, request,
                          NULL, NULL );
    if (err == MIO_OK)
        conn->pubsub_rx_listening = 1;

    return err;
}

/** Indicates to XMPP server that client will stop listining for pubsub data.
 *
 *  @param conn Active MIO connection.
 *
 *  @returns SOX_OK on success.
 *  */
int mio_pubsub_data_listen_stop(mio_conn_t *conn) {
    mio_request_t *request;

    // Signal mio_pubsub_data_receive if it is waiting and delete pubsub_data_rx request if we were listening
    request = _mio_request_get(conn, "pubsub_data_rx");
    if (request != NULL ) {
        // FIXME: Potential deadlock on mio_pubsub_data_receive() function when condition signaled without function waiting
        conn->pubsub_rx_listening = 0;
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
        _mio_request_delete(conn, "pubsub_data_rx");
    }
    return MIO_OK;
}

/** Clears the packet receive queue for pubsub packets.
 *
 * @param conn Active MIO connection.
 *  */
void mio_pubsub_data_rx_queue_clear(mio_conn_t *conn) {
    mio_response_t *response = _mio_pubsub_rx_queue_dequeue(conn);
    while (response != NULL ) {
        response = _mio_pubsub_rx_queue_dequeue(conn);
        mio_response_free(response);
    }
}

/** Clears the packet receive queue for pubsub packets.
 *
 * @param conn Active MIO connection.
 *
 * @returns MIO_OK on sccuess, and MIO_ERROR_DISCONNECTED on failure.
 *  */
int mio_pubsub_data_receive(mio_conn_t *conn, mio_response_t *response) {
    int err;
    mio_request_t *request;
    mio_response_t *rx_response = NULL;

    // Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process data_receive request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    // If there is a pubsub response waiting on the pubsub_rx_queue, return it. Else block until we receive one.
    rx_response = _mio_pubsub_rx_queue_dequeue(conn);
    if (rx_response != NULL ) {
        strcpy(response->id, rx_response->id);
        response->name = rx_response->name;
        response->ns = rx_response->ns;
        response->response_type = rx_response->response_type;
        response->type = rx_response->type;
        response->response = rx_response->response;
        response->stanza = rx_response->stanza;
        free(rx_response);
        return MIO_OK;
    }

    // Call mio_pubsub_data_listen_start() if we aren't listening yet
    request = _mio_request_get(conn, "pubsub_data_rx");
    if (request == NULL ) {
        err = mio_pubsub_data_listen_start(conn);
        if (err != MIO_OK)
            return err;
        request = _mio_request_get(conn, "pubsub_data_rx");
    }
    while (rx_response == NULL && conn->pubsub_rx_queue_len <= 0
            && conn->pubsub_rx_listening) {
        while (!request->predicate) {
            err = pthread_mutex_lock(&request->mutex);
            if (err != 0) {
                mio_error("Unable to lock mutex for request %s", request->id);
                return MIO_ERROR_MUTEX;
            }
            // Wait for response from server
            err = pthread_cond_wait(&request->cond, &request->mutex);
            if (err != 0) {
                mio_error("Conditional wait for request with id %s failed",
                          request->id);
                pthread_mutex_unlock(&request->mutex);
                return MIO_ERROR_COND_WAIT;
            } else {
                mio_debug("Got response for request with id %s", request->id);
                pthread_mutex_unlock(&request->mutex);
            }
        }
    }
    request->predicate = 0;
    rx_response = _mio_pubsub_rx_queue_dequeue(conn);
    if (rx_response == NULL )
        return MIO_ERROR_NO_RESPONSE;

    strcpy(response->id, rx_response->id);
    response->name = rx_response->name;
    response->ns = rx_response->ns;
    response->response_type = rx_response->response_type;
    response->type = rx_response->type;
    response->response = rx_response->response;
    response->stanza = rx_response->stanza;
    free(rx_response);
    return MIO_OK;
}

void XMLCALL mio_XMLstart_pubsub_data_receive(void *data,
        const char *element_name, const char **attr) {

    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    mio_data_t *mio_data = (mio_data_t*) packet->payload;
    mio_transducer_data_t *t, *parent;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "items") == 0) {
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "node") == 0) {
                mio_data->event = malloc(strlen(attr[i + 1]) + 1);
                strcpy(mio_data->event, attr[i + 1]);
                // TODO: Support for multiple payloads
                packet->num_payloads++;
            }
        }

    } else if (strcmp(element_name, "transducerData") == 0
               || strcmp(element_name, "transducerSetData") == 0) {

        if (xml_data->payload == NULL) {
            parent = mio_transducer_data_new();
            xml_data->payload = parent;
            t = parent;
            response->response_type = MIO_RESPONSE_PACKET;
            mio_data->transducers = parent;
            mio_data->num_transducers++;
        } else {
            parent = xml_data->payload;
            t = mio_transducer_data_new();
            mio_data->num_transducers++;
        }

        if (strcmp(element_name, "transducerData") == 0)
            t->type = MIO_TRANSDUCER_DATA;
        else
            t->type = MIO_TRANSDUCER_SET_DATA;

        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "value") == 0)
                t->value = strdup(attr[i + 1]);
            else if (strcmp(attr_name, "timestamp") == 0)
                t->timestamp = strdup(attr[i + 1]);
            else if (strcmp(attr_name, "name") == 0)
                t->name = strdup(attr[i + 1]);
        }
        if (t != parent)
            mio_transducer_data_add(parent, t);
    }
}


/**
 * @ingroup Internal
 * Internal function to send out an XMPP message in a blocking fashion. The function returns once the server's response has been processed or an error occurs.
 *
 * @param conn A pointer to an active mio conn.
 * @param stanza A pointer to the mio stanza to be sent.
 * @param handler A pointer to the mio handler which should parse the server's response.
 * @param response A pointer to an allocated mio response struct which will be populated with the server's response.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_send_blocking(mio_conn_t *conn, mio_stanza_t *stanza,
                      mio_handler handler, mio_response_t * response) {

    mio_request_t *request;
    struct timespec ts, ts_reconnect;
    struct timeval tp;
    int err;
    int send_attempts = 0;

    request = _mio_request_new();

// Set timeout
    gettimeofday(&tp, NULL );
    ts.tv_sec = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += MIO_REQUEST_TIMEOUT_S;

// If we have too many open requests, wait
    sem_wait(conn->mio_open_requests);

    mio_handler_id_add(conn, handler, stanza->id, request, NULL, response);


// Send out the stanza, then wait for response
    while (mio_send_nonblocking(conn, stanza) == MIO_ERROR_CONNECTION) {
        // If sending fails, wait for connection to be reestablished
        send_attempts++;
        if (send_attempts >= MIO_SEND_RETRIES) {
            xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
                                   request->id);
            _mio_request_delete(conn, request->id);
            return MIO_ERROR_CONNECTION;
        }

        // Set timeout
        gettimeofday(&tp, NULL );
        ts_reconnect.tv_sec = tp.tv_sec;
        ts_reconnect.tv_nsec = tp.tv_usec * 1000;
        ts_reconnect.tv_sec += MIO_RECONNECTION_TIMEOUT_S;

        pthread_mutex_lock(&conn->conn_mutex);
        // Wait for connection to be established before returning
        while (!conn->conn_predicate) {
            err = pthread_cond_timedwait(&conn->conn_cond, &conn->conn_mutex,
                                         &ts_reconnect);
            if (err == ETIMEDOUT) {
                pthread_mutex_unlock(&conn->conn_mutex);
                xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
                                       request->id);
                _mio_request_delete(conn, request->id);
                mio_error("Sending attempt timed out");
                return MIO_ERROR_TIMEOUT;
            } else if (err != 0) {
                pthread_mutex_unlock(&conn->conn_mutex);
                xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
                                       request->id);
                _mio_request_delete(conn, request->id);
                mio_error("Conditional wait for connection attempt timed out");
                return MIO_ERROR_COND_WAIT;
            } else {
                pthread_mutex_unlock(&conn->conn_mutex);
            }
        }
    }

    while (!request->predicate) {
        err = pthread_mutex_lock(&request->mutex);
        if (err != 0) {
            mio_error("Unable to lock mutex for request %s", request->id);
            return MIO_ERROR_MUTEX;
        }
        // Wait for response from server
        err = pthread_cond_timedwait(&request->cond, &request->mutex, &ts);
        if (err == ETIMEDOUT) {
            xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
                                   request->id);
            pthread_mutex_unlock(&request->mutex);
            _mio_request_delete(conn, request->id);
            mio_error("Request with id %s timed out", request->id);
            return MIO_ERROR_TIMEOUT;
        } else if (err != 0) {
            xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id,
                                   request->id);
            pthread_mutex_unlock(&request->mutex);
            _mio_request_delete(conn, request->id);
            mio_error("Conditional wait for request with id %s failed",
                      request->id);
            return MIO_ERROR_COND_WAIT;
        } else {
            mio_debug("Got response for request with id %s", request->id);
            response = request->response;
            pthread_mutex_unlock(&request->mutex);
            _mio_request_delete(conn, request->id);
            return MIO_OK;
        }
    }
    mio_error("Predicate of request with ID %s == 0", request->id);
    return MIO_ERROR_PREDICATE;
}

/**
 * @ingroup Internal
 * Internal function to send out an XMPP message in a non-blocking fashion. The function returns once the message has been sent out or an error occurs. No handlers are added.
 *
 * @param conn A pointer to an active mio conn.
 * @param stanza A pointer to the mio stanza to be sent.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_send_nonblocking(mio_conn_t *conn, mio_stanza_t *stanza) {

    char *buf = NULL;
    size_t len = 0;
    int err;

// Render stanza first instead of using xmpp_send so that we can keep the mutex unlocked longer
    if (xmpp_stanza_to_text(stanza->xmpp_stanza, &buf, &len) == 0) {
        if (!conn->xmpp_conn->authenticated) {
            xmpp_free(conn->xmpp_conn->ctx, buf);
            mio_error("Sending failed because not connected to server");
            return MIO_ERROR_CONNECTION;
        }
        // Make sure we are the only one writing to the send queue
        err = pthread_mutex_lock(&conn->event_loop_mutex);
        if (err != 0) {
            mio_error("Unable to lock event loop mutex");
            return MIO_ERROR_MUTEX;
        }
        // Send request to server and forget it
        xmpp_send_raw(conn->xmpp_conn, buf, len);
        pthread_mutex_unlock(&conn->event_loop_mutex);
        xmpp_debug(conn->xmpp_conn->ctx, "conn", "SENT: %s", buf);
        xmpp_free(conn->xmpp_conn->ctx, buf);
        // Signal the _mio_run thread that it should run the event loop in case it is waiting
        mio_cond_signal(&conn->send_request_cond, &conn->send_request_mutex,
                        &conn->send_request_predicate);
    }
    return MIO_OK;
}


/**
 * @ingroup PubSub
 * Allocates and initializes a new mio stanza containing an item stanza.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param type A string containing the item type to be created e.g. "data", "meta", etc.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
mio_stanza_t *mio_pubsub_item_new(mio_conn_t *conn, char* item_type) {
    mio_stanza_t * item = mio_stanza_new(conn);
    xmpp_stanza_set_name(item->xmpp_stanza, "item");
    xmpp_stanza_set_id(item->xmpp_stanza, item_type);
    return item;
}

/**
 * @ingroup PubSub
 * Allocates and initializes a new mio stanza containing an item stanza of type data.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
mio_stanza_t *mio_pubsub_item_data_new(mio_conn_t *conn) {

    xmpp_stanza_t *data = NULL;
    mio_stanza_t *item = mio_pubsub_item_new(conn, "data");
    data = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(data, "data");

    xmpp_stanza_add_child(item->xmpp_stanza, data);
    xmpp_stanza_release(data);

    return item;
}

/**
 * @ingroup PubSub
 * Allocates and initializes a new mio stanza containing an item stanza of type interface.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
mio_stanza_t *mio_pubsub_item_interface_new(mio_conn_t *conn, char* interface) {

    xmpp_stanza_t *data = NULL;
    mio_stanza_t *item = mio_pubsub_item_new(conn, interface);
    data = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(data, "data");

    xmpp_stanza_add_child(item->xmpp_stanza, data);
    xmpp_stanza_release(data);

    return item;
}

mio_stanza_t *mio_pubsub_iq_get_stanza_new(mio_conn_t *conn,
        const char *node) {
    mio_stanza_t *iq = mio_pubsub_iq_stanza_new(conn, node);
    xmpp_stanza_set_type(iq->xmpp_stanza, "get");
    return iq;
}

mio_stanza_t *mio_pubsub_iq_stanza_new(mio_conn_t *conn,
                                       const char *node) {
    char *pubsub_server, *pubsub_server_target = NULL;
    uuid_t uuid;
    mio_stanza_t *iq = mio_stanza_new(conn);

// Try to get server address from node, otherwise from JID
    if (node != NULL )
        pubsub_server_target = mio_get_server(node);
    if (pubsub_server_target == NULL )
        pubsub_server_target = mio_get_server(conn->xmpp_conn->jid);

    pubsub_server = malloc(sizeof(char) * (strlen(pubsub_server_target) + 8));

    sprintf(pubsub_server, "pubsub.%s", pubsub_server_target);

// Create id for iq stanza using new UUID
    uuid_generate(uuid);
    uuid_unparse(uuid, iq->id);

    xmpp_stanza_set_name(iq->xmpp_stanza, "iq");
    xmpp_stanza_set_attribute(iq->xmpp_stanza, "to", pubsub_server);
    xmpp_stanza_set_attribute(iq->xmpp_stanza, "id", iq->id);
    xmpp_stanza_set_attribute(iq->xmpp_stanza, "from", conn->xmpp_conn->jid);

    return iq;
}

mio_stanza_t *mio_pubsub_set_stanza_new(mio_conn_t *conn,
                                        const char *node) {
    mio_stanza_t *iq = mio_pubsub_stanza_new(conn, node);
    xmpp_stanza_set_type(iq->xmpp_stanza, "set");
    return iq;
}

mio_stanza_t *mio_pubsub_get_stanza_new(mio_conn_t *conn,
                                        const char *node) {
    mio_stanza_t *iq = mio_pubsub_stanza_new(conn, node);
    xmpp_stanza_set_type(iq->xmpp_stanza, "get");
    return iq;
}

mio_stanza_t *mio_pubsub_stanza_new(mio_conn_t *conn, const char *node) {
    xmpp_stanza_t *pubsub = NULL;
    uuid_t uuid;
    mio_stanza_t *iq = mio_stanza_new(conn);
    pubsub = xmpp_stanza_new(conn->xmpp_conn->ctx);
    char *pubsub_server, *pubsub_server_target = NULL;

// Try to get server address from node, otherwise from JID
    if (node != NULL )
        pubsub_server_target = mio_get_server(node);
    if (pubsub_server_target == NULL )
        pubsub_server_target = mio_get_server(conn->xmpp_conn->jid);

    pubsub_server = malloc(sizeof(char) * (strlen(pubsub_server_target) + 8));

    sprintf(pubsub_server, "pubsub.%s", pubsub_server_target);

// Create id for iq stanza using new UUID
    uuid_generate(uuid);
    uuid_unparse(uuid, iq->id);

// Create a new iq stanza
    xmpp_stanza_set_name(iq->xmpp_stanza, "iq");
    xmpp_stanza_set_attribute(iq->xmpp_stanza, "to", pubsub_server);
    xmpp_stanza_set_attribute(iq->xmpp_stanza, "id", iq->id);

    xmpp_stanza_set_name(pubsub, "pubsub");
    xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");

    xmpp_stanza_add_child(iq->xmpp_stanza, pubsub);

    xmpp_stanza_release(pubsub);
    free(pubsub_server);

    return iq;
}

/** Gets n of the most recent published items.
 *
 * @param conn
 * @param node
 * @param response
 * @param max_items
 * @param item_id
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on disconnect.
 * */
int mio_item_recent_get(mio_conn_t* conn, const char *node,
                        mio_response_t * response, int max_items, const char *item_id,
                        mio_handler *handler) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *items = NULL;
    xmpp_stanza_t *item = NULL;
    int err;
    char max_items_s[16];

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process recent item get request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_get_stanza_new(conn, node);

// Create items stanza
    items = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(items, "items");
    xmpp_stanza_set_attribute(items, "node", node);

// If an item id is specified, attach it to the stanza, otherwise retrieve up to max_items most recent items
    if (item_id != NULL ) {
        item = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(item, "item");
        xmpp_stanza_set_id(item, item_id);
        xmpp_stanza_add_child(items, item);
    } else {
        if (max_items != 0) {
            snprintf(max_items_s, 16, "%u", max_items);
            xmpp_stanza_set_attribute(items, "max_items", max_items_s);
        }
    }

// Build xmpp message
    xmpp_stanza_add_child(iq->xmpp_stanza->children, items);

// If a handler is specified use it, otherwise use default handler
    if (handler == NULL )
// Send out the stanza
        err = mio_send_blocking(conn, iq,
                                (mio_handler) mio_handler_item_recent_get, response);
    else
        err = mio_send_blocking(conn, iq, (mio_handler) handler, response);

// Release the stanzas
    xmpp_stanza_release(items);
    mio_stanza_free(iq);

    return err;
}

int mio_items_recent_get(mio_conn_t* conn, const char *node,
                        mio_response_t * response, int max_items, char **item_id, int n_items,
                        mio_handler *handler) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *items = NULL;
    xmpp_stanza_t *item = NULL;
    int err;
    int i;
    char max_items_s[16];

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process recent item get request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_get_stanza_new(conn, node);

// Create items stanza
    items = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(items, "items");
    xmpp_stanza_set_attribute(items, "node", node);

    for (i = 0; i < n_items; i++) {
// If an item id is specified, attach it to the stanza, otherwise retrieve up to max_items most recent items
        if (item_id[i] != NULL ) {
            item = xmpp_stanza_new(conn->xmpp_conn->ctx);
            xmpp_stanza_set_name(item, "item");
            xmpp_stanza_set_id(item, item_id[i]);
            xmpp_stanza_add_child(items, item);
        } 
    }
    if (n_items == 0) {
        if (max_items != 0) {
            snprintf(max_items_s, 16, "%u", max_items);
            xmpp_stanza_set_attribute(items, "max_items", max_items_s);
        }
    }

    if (max_items != 0) {
        snprintf(max_items_s, 16, "%u", max_items);
        xmpp_stanza_set_attribute(items, "max_items", max_items_s);
    }
        
// Build xmpp message
   xmpp_stanza_add_child(iq->xmpp_stanza->children, items);
// If a handler is specified use it, otherwise use default handler
    if (handler == NULL )
// Send out the stanza
        err = mio_send_blocking(conn, iq,
                                (mio_handler) mio_handler_item_recent_get, response);
    else
        err = mio_send_blocking(conn, iq, (mio_handler) handler, response);

// Release the stanzas
    xmpp_stanza_release(items);
    mio_stanza_free(iq);

    return err;
}

int mio_handler_pubsub_data_receive(mio_conn_t * const conn,
                                    mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
    mio_stanza_t *stanza_copy;
    mio_request_t *request = _mio_request_get(conn, "pubsub_data_rx");

    if (request == NULL ) {
        mio_error("Request with id %s not found, aborting handler",
                  response->id);
        return MIO_ERROR_REQUEST_NOT_FOUND;
    }
    mio_packet_t *packet = mio_packet_new();
    mio_data_t *data = mio_data_new();
    mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();

    if (response != NULL )
        request = _mio_request_get(conn, response->id);
    else
        request = _mio_request_get(conn, "pubsub_data_rx");

    mio_packet_payload_add(packet, (void*) data, MIO_PACKET_DATA);

    if (response == NULL ) {
        response = mio_response_new();
        strcpy(response->id, "pubsub_data_rx");
        response->ns = malloc(
                           strlen("http://jabber.org/protocol/pubsub#event") + 1);
        strcpy(response->ns, "http://jabber.org/protocol/pubsub#event");
    }
    response->response = packet;
    xml_data->response = response;

    int err = mio_xml_parse(conn, stanza, xml_data,
                            mio_XMLstart_pubsub_data_receive, NULL );

    if (err != MIO_OK)
        return err;
    else if (response->response_type == MIO_RESPONSE_PACKET && request != NULL ) {
        stanza_copy = mio_stanza_clone(conn, stanza);
        response->stanza = stanza_copy;
        _mio_pubsub_rx_queue_enqueue(conn, response);
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
        return MIO_HANDLER_KEEP;
    } else {
        mio_packet_free(packet);
        return MIO_HANDLER_KEEP;
    }
}


