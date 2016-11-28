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

#include <common.h>
#include <strophe.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "mio_connection.h"
#include "mio_handlers.h"
#include "mio_user.h"
#include "mio_reference.h"
#include "mio_meta.h"
#include "mio_node.h"
#include "mio_schedule.h"
#include "mio_collection.h"

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

mio_log_level_t _mio_log_level = MIO_LEVEL_ERROR;

/**
 * @ingroup Connection
 * Creates a new mio connection, which will output logging data at the level specified in log_level.
 *
 * @param log_level MIO_LEVEL_ERROR for only logging errors, MIO_LEVEL_WARN for logging warnings and errors, MIO_LEVEL_INFO for logging info, warnings and errors, MIO_LEVEL_DEBUG for logging debug information, info, warnings and errors.
 * @returns A newly allocated and initialized mio conn.
 */
mio_conn_t *mio_conn_new(mio_log_level_t log_level) {
    mio_conn_t *conn = malloc(sizeof(mio_conn_t));
    memset(conn,0,sizeof(mio_conn_t));
    xmpp_ctx_t *ctx;
    xmpp_log_t *log;

// Make conn_mutex recursive so that it can be locked multiple times by the same thread
    pthread_mutexattr_t conn_mutex_attr;
    pthread_mutexattr_init(&conn_mutex_attr);
    pthread_mutexattr_settype(&conn_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    conn->presence_status = MIO_PRESENCE_UNKNOWN;
    pthread_mutex_init(&conn->event_loop_mutex, NULL );
    pthread_mutex_init(&conn->send_request_mutex, NULL );
    pthread_mutex_init(&conn->pubsub_rx_queue_mutex, NULL );
    pthread_mutex_init(&conn->conn_mutex, NULL );
//pthread_mutexattr_destroy(&conn_mutex_attr);
    pthread_cond_init(&conn->send_request_cond, NULL );
    pthread_cond_init(&conn->conn_cond, NULL );
    pthread_rwlock_init(&conn->mio_hash_lock, NULL );
    TAILQ_INIT(&conn->pubsub_rx_queue);

#ifdef __linux__
    conn->mio_open_requests = sem_open("mio_open_requests",O_CREAT);
#endif

    switch (log_level) {
    case MIO_LEVEL_DEBUG:
        log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
        break;
    case MIO_LEVEL_ERROR:
        log = xmpp_get_default_logger(XMPP_LEVEL_ERROR);
        break;
    case MIO_LEVEL_INFO:
        log = xmpp_get_default_logger(XMPP_LEVEL_INFO);
        break;
    case MIO_LEVEL_WARN:
        log = xmpp_get_default_logger(XMPP_LEVEL_WARN);
        break;
    default:
        log = xmpp_get_default_logger(XMPP_LEVEL_ERROR);
        break;
    }

    _mio_log_level = log_level;
    ctx = xmpp_ctx_new(NULL, log);
// Create a connection
    conn->xmpp_conn = xmpp_conn_new(ctx);

    return conn;
}

/**
 * @ingroup Connection
 * Frees a mio conn.
 *
 * @param conn A pointer to the allocated mio conn to be freed.
 * */
void mio_conn_free(mio_conn_t *conn) {
    if (conn->xmpp_conn != NULL ) {
        //if(conn->xmpp_conn->ctx != NULL)
        // 	xmpp_ctx_free(conn->xmpp_conn->ctx);
        xmpp_conn_release(conn->xmpp_conn);
    }
    pthread_mutex_destroy(&conn->event_loop_mutex);
    pthread_mutex_destroy(&conn->send_request_mutex);
    pthread_mutex_destroy(&conn->conn_mutex);
    pthread_cond_destroy(&conn->send_request_cond);
    pthread_cond_destroy(&conn->conn_cond);
    free(conn);
}



/**
 * @ingroup Internal
 * Function to unblock a single thread threads waiting on the condition cond. Releases the mutex and updates the predicate.
 *
 * @param cond Condition to signal
 * @param mutex Mutex to release
 * @param predicate Predicate to update
 * @returns 0 on success, otherwise non-zero value indicating error.
 */
int mio_cond_signal(pthread_cond_t *cond, pthread_mutex_t *mutex,
                    int *predicate) {
    int err;
    err = pthread_mutex_lock(mutex);
    if (err)
        return err;
    *predicate = 1;
    err = pthread_cond_signal(cond);
    if (err)
        return err;
    err = pthread_mutex_unlock(mutex);
    return err;
}

/**
 * @ingroup Internal
 * Function to unblock all threads waiting on the condition cond. Releases the mutex and updates the predicate.
 *
 * @param cond Condition to signal
 * @param mutex Mutex to release
 * @param predicate Predicate to update
 * @returns 0 on success, otherwise non-zero value indicating error.
 */
int mio_cond_broadcast(pthread_cond_t *cond, pthread_mutex_t *mutex,
                       int *predicate) {
    int err;
    err = pthread_mutex_lock(mutex);
    if (err)
        return err;
    *predicate = 1;
    err = pthread_cond_broadcast(cond);
    if (err)
        return err;
    err = pthread_mutex_unlock(mutex);
    return err;
}



/**
 * @ingroup Core
 * Function to extract the XMPP server address from a JID.
 *
 * @param jid The JID to extract the server address from
 * @returns A pointer to the first character of the server address inside of the JID that was provided.
 */
char *mio_get_server(const char *jid) {

    // Locate @ symbol in jid
    char *loc = strpbrk(jid, "@");
    if (loc != NULL )
        return loc += 1;
    else
        return NULL ;
}



/**
 * @ingroup Core
 * Adds a payload to a mio packet.
 *
 * @param packet The mio packet to which the payload should be added.
 * @param payload A pointer to the payload to be added to the mio packet.
 * @param type The mio packet type tof the payload being added.
 */
void mio_packet_payload_add(mio_packet_t *packet, void *payload,
                            mio_packet_type_t type) {
    packet->type = type;
    packet->payload = (void*) payload;
}


/** Sends XMPP presence stanza to the XMPP server.
 *
 *  @param conn
 *
 *  @returns MIO_OK on success and MIO_ERROR_DISCONNECTED if no connection to server.
 *  */
int mio_listen_start(mio_conn_t *conn) {
    xmpp_stanza_t *pres;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error("Cannot start listening since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }
//Send initial <presence/> so that we appear online to contacts
    pres = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(pres, "presence");
    mio_debug("Sending presence stanza");
    xmpp_send(conn->xmpp_conn, pres);
    xmpp_stanza_release(pres);
    conn->presence_status = MIO_PRESENCE_PRESENT;
    return MIO_OK;
}

/** Sends XMPP presence unavailable stanza to the XMPP server.
 *
 *  @param conn Active MIO connection.
 *
 *  @returns MIO_OK on success and MIO_ERROR_DISCONNECTED if no connection to server.
 *  */
int mio_listen_stop(mio_conn_t *conn) {
    xmpp_stanza_t *pres;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error("Cannot stop listening since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }
//Send <presence type="unavailable"/> so that we appear unavailable to contacts
    pres = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(pres, "presence");
    xmpp_stanza_set_type(pres, "unavailable");
    mio_debug("Sending presence unavailable stanza");
    xmpp_send(conn->xmpp_conn, pres);
    xmpp_stanza_release(pres);
    conn->presence_status = MIO_PRESENCE_UNAVAILABLE;
    return MIO_OK;
}


/**
 * @ingroup Internal
 * Internal function to get a mio response from a mio request.
 *
 * @param conn A pointer to an active mio connection from which the request should be fetched from.
 * @param id A string containing the UUID of the request to be fetched.
 * @returns A pointer to the mio response fetched from the specified request or NULL if the request could not be found.
 */
mio_response_t *_mio_response_get(mio_conn_t *conn, char *id) {
    mio_request_t *r = _mio_request_get(conn, id);
    if (r != NULL )
        return r->response;
    else
        return NULL ;
}

/**
 * @ingroup Internal
 * Internal function to enqueue a mio response to the received pubsub queue.
 *
 * @param conn A pointer to an active mio connection.
 * @param response A pointer to the mio response to enqueue
 */
void _mio_pubsub_rx_queue_enqueue(mio_conn_t *conn, mio_response_t *response) {
    mio_response_t *temp = NULL;

    if (conn->pubsub_rx_queue_len >= MIO_PUBSUB_RX_QUEUE_MAX_LEN) {
        mio_warn(
            "Pubsub RX queue has reached it's maximum length, dropping oldest response");
        pthread_mutex_lock(&conn->pubsub_rx_queue_mutex);
        temp = (mio_response_t*) TAILQ_FIRST(&conn->pubsub_rx_queue);
        TAILQ_REMOVE(&conn->pubsub_rx_queue, temp, responses);
        TAILQ_INSERT_TAIL(&conn->pubsub_rx_queue, response, responses);
        pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
    } else {
        pthread_mutex_lock(&conn->pubsub_rx_queue_mutex);
        TAILQ_INSERT_TAIL(&conn->pubsub_rx_queue, response, responses);
        conn->pubsub_rx_queue_len++;
        pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
    }
}

/**
 * @ingroup Internal
 * Internal function to pop a mio response off the received pubsub queue.
 *
 * @param conn A pointer to an active mio connection.
 * @returns A pointer to the mio response that was popped off the received pubsub queue.
 */
mio_response_t *_mio_pubsub_rx_queue_dequeue(mio_conn_t *conn) {
    mio_response_t *response = NULL;
    pthread_mutex_lock(&conn->pubsub_rx_queue_mutex);
    if (conn->pubsub_rx_queue_len > 0) {
        response = TAILQ_FIRST(&conn->pubsub_rx_queue);
        TAILQ_REMOVE(&conn->pubsub_rx_queue, response, responses);
        conn->pubsub_rx_queue_len--;
        pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
        return response;
    } else {
        pthread_mutex_unlock(&conn->pubsub_rx_queue_mutex);
        return NULL ;
    }
}


/**
 * @ingroup Internal
 * Internal function to allocate an initialize a new mio request
 *
 * @returns A newly allocated and initialized mio request
 */
mio_request_t *_mio_request_new() {
    mio_request_t *request = (mio_request_t*) malloc(sizeof(mio_request_t));
    memset(request,0,sizeof(mio_request_t));
    pthread_cond_init(&request->cond, NULL );
    pthread_mutex_init(&request->mutex, NULL );
    return request;
}

/**
 * @ingroup Internal
 * Internal function to free an initialized mio request.
 *
 * @param request The allocated mio request to be freed.
 */
void _mio_request_free(mio_request_t *request) {
    pthread_cond_destroy(&request->cond);
    pthread_mutex_destroy(&request->mutex);
    free(request);
}

/**
 * @ingroup Internal
 * Internal function to add a mio request to the request hash table.
 *
 * @param conn A pointer to an active mio connection to which the request should be added to.
 * @param id A string containing the UUID of the request being added.
 * @param handler A pointer to the mio handler associated with the request.
 * @param handler_type MIO_HANDLER_ID for a handler that is executed when a message matching its ID is received, MIO_HANLDER_TIMED for a handler that is executed based at fixed time intervals.
 * @param response A pointer to an allocated mio response, which will contain the server's response.
 * @param request A pointer to the mio request to be added.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int _mio_request_add(mio_conn_t *conn, const char *id,
                     mio_handler handler, mio_handler_type_t handler_type,
                     mio_response_t *response, mio_request_t *request) {
    mio_request_t *find_request = NULL;

//sem_getvalue is not implemented in OSX
#ifdef __linux__
    int sem_value;
    sem_getvalue(conn->mio_open_requests, &sem_value);
    mio_debug("Remaining requests: %d", sem_value);
    if (sem_value <= 0) {
        mio_error("Cannot add request for id %s, open request table full", id);
        return MIO_ERROR_TOO_MANY_OPEN_REQUESTS;
    }
#endif
// Check if ID is already in table
    if (pthread_rwlock_wrlock(&conn->mio_hash_lock) != 0) {
        mio_error("Can't get hash table rw lock");
        return MIO_ERROR_RW_LOCK;
    }
    HASH_FIND_STR(conn->mio_request_table, id, find_request);
    if (find_request == NULL ) {
        if (strcmp(request->id, "") == 0)
            strcpy(request->id, id);
        if (response != NULL ) {
            if (strcmp(request->id, "") == 0)
                strcpy(response->id, id);
        }
        // Add given request to hash table
        HASH_ADD_STR(conn->mio_request_table, id, request);
    }
    pthread_rwlock_unlock(&conn->mio_hash_lock);
// Update members of struct
    request->handler = handler;
    request->handler_type = handler_type;
    request->response = response;
    return MIO_OK;
}

/**
 * @ingroup Internal
 * Internal function to delete a mio request from the request hash table.
 *
 * @param conn A pointer to an active mio connection from which the request should be removed from.
 * @param id A string containing the UUID of the request being deleted.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int _mio_request_delete(mio_conn_t *conn, char *id) {
    mio_request_t *r;
    if (pthread_rwlock_wrlock(&conn->mio_hash_lock) != 0) {
        mio_error("Can't get hash table rw lock");
        return MIO_ERROR_RW_LOCK;
    }
    HASH_FIND_STR(conn->mio_request_table, id, r);
    if (r != NULL ) {
        HASH_DEL(conn->mio_request_table, r);
        pthread_rwlock_unlock(&conn->mio_hash_lock);
        _mio_request_free(r);
        sem_post(conn->mio_open_requests);
        return MIO_OK;
    }
    pthread_rwlock_unlock(&conn->mio_hash_lock);
    return MIO_ERROR_REQUEST_NOT_FOUND;
}

/**
 * @ingroup Internal
 * Internal function to get a mio request from a request hash table.
 *
 * @param conn A pointer to an active mio connection from which the request should be fetched from.
 * @param id A string containing the UUID of the request to be fetched.
 * @returns A pointer to the mio request fetched from the request hash or NULL if the request could not be found.
 */
mio_request_t *_mio_request_get(mio_conn_t *conn, char *id) {
    mio_request_t *r;
    if (pthread_rwlock_rdlock(&conn->mio_hash_lock) != 0) {
        mio_error("Can't get hash table rd lock");
        return NULL ;
    }
    HASH_FIND_STR(conn->mio_request_table, id, r);
    pthread_rwlock_unlock(&conn->mio_hash_lock);
    return r;
}


/**
 * @ingroup Core
 * Allocates and initializes a new mio response.
 *
 * @returns A pointer to a newly allocated an initialized mio response.
 */
mio_response_t *mio_response_new() {
    mio_response_t *response = (mio_response_t *) malloc(
                                   sizeof(mio_response_t));
    memset(response,0,sizeof(mio_response_t));
    response->response_type = MIO_RESPONSE_UNKNOWN;
    return response;
}

/**
 * @ingroup Core
 * Frees an allocated mio error response.
 *
 * @param A pointer to the mio error response to be freed.
 */
void _mio_response_error_free(mio_response_error_t* error) {
    if (error->description != NULL) free(error->description);
    free(error);
}

/**
 * @ingroup Core
 * Frees an allocated mio response.
 *
 * @param A pointer to the mio response to be freed.
 */
void mio_response_free(mio_response_t *response) {
// TODO: free other response types
    if (response == NULL) return;
    switch (response->response_type) {
    case MIO_RESPONSE_ERROR:
        _mio_response_error_free(response->response);
        break;
    case MIO_RESPONSE_PACKET:
        mio_packet_free(response->response);
        break;
    default:
        break;
    }

    if (response->ns != NULL )
        free(response->ns);

    if (response->stanza != NULL )
        mio_stanza_free(response->stanza);
    free(response);
}

/**
 * @ingroup Core
 * Allocates and initializes a new mio error response.
 *
 * @returns A pointer to a newly allocated an initialized mio response.
 */
mio_response_error_t *_mio_response_error_new() {
    mio_response_error_t *error = malloc(sizeof(mio_response_error_t));
    memset(error,0,sizeof(mio_response_error_t));
    return error;
}


/**
 * @ingroup Stanza
 * Converts a mio stanza to a string.
 *
 * @param stanza The mio stanza to be converted to a string.
 * @param buf A pointer to an unallocated 2D character buffer to store the string.
 * @param buflen A pointer to an integer to contain the length of the outputted string.
 * @returns 0 on success, otherwise non-zero integer on error.
 */
int mio_stanza_to_text(mio_stanza_t *stanza, char **buf, int *buflen) {
    return xmpp_stanza_to_text(stanza->xmpp_stanza, buf, (size_t*) buflen);
}

/**
 * @ingroup Stanza
 * Parses a mio stanza into a mio response.

 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param stanza The mio stanza to be converted to a mio response.
 * @returns A mio response containing the parsed stanza.
 */
mio_response_t *mio_stanza_to_response(mio_conn_t *conn, mio_stanza_t *stanza) {
    mio_response_t *response = mio_response_new();
    mio_handler_pubsub_data_receive(conn, stanza, response, NULL );
    return response;
}

/**
 * @ingroup Stanza
 * Frees a mio stanza.
 *
 * @param The pointer to the mio stanza to be freed.
 */
void mio_stanza_free(mio_stanza_t *stanza) {
    if (stanza->xmpp_stanza != NULL)
        xmpp_stanza_release(stanza->xmpp_stanza);
    free(stanza);
}

/**
 * @ingroup Stanza
 * Allocates and initializes a new mio stanza.
 *
 * @returns A pointer to the newly allocated and initialized mio stanza
 */
mio_stanza_t *mio_stanza_new(mio_conn_t *conn) {
    mio_stanza_t *stanza = (mio_stanza_t*) malloc(sizeof(mio_stanza_t));
    memset(stanza,0,sizeof(mio_stanza_t));
    stanza->xmpp_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
    return stanza;
}

/**
 * @ingroup Stanza
 * Performs a deep copy of a mio stanza.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @returns A pointer to the cloned mio stanza.
 */
mio_stanza_t *mio_stanza_clone(mio_conn_t *conn, mio_stanza_t *stanza) {
    mio_stanza_t *clone = malloc(sizeof(mio_stanza_t));
    memset(clone,0x0,sizeof(mio_stanza_t));
    clone->xmpp_stanza = xmpp_stanza_clone(stanza->xmpp_stanza);
    return clone;
}
int mio_response_print(mio_response_t *response) {
    mio_packet_t *packet;
    mio_response_error_t *err;
    mio_data_t *data;
    mio_subscription_t *sub;
    mio_affiliation_t *aff;
    mio_collection_t *coll;
    mio_meta_t *meta;
    mio_transducer_meta_t *t_meta;
    mio_enum_map_meta_t *e_meta;
    mio_property_meta_t *p_meta;
    mio_transducer_data_t *t;
    mio_node_type_t *type;
    mio_event_t *event;
    mio_reference_t *ref;

    switch (response->response_type) {
    case MIO_RESPONSE_OK:
        fprintf(stdout, "\nRequest successful\n\n");
        break;
    case MIO_RESPONSE_ERROR:
        err = response->response;
        fprintf(stderr, "MIO Error:\n");
        fprintf(stderr, "\tError code: %d\n\tError description: %s\n",
                err->err_num, err->description);
        break;
    case MIO_RESPONSE_PACKET:
        packet = (mio_packet_t *) response->response;
        switch (packet->type) {
        case MIO_PACKET_DATA:
            data = (mio_data_t*) packet->payload;
            fprintf(stdout, "MIO Data Packet:\n\tEvent Node:%s\n", data->event);
            t = data->transducers;
            while (t != NULL ) {
                if (t->type == MIO_TRANSDUCER_DATA)
                    fprintf(stdout, "\t\tTrasducerData:\n");
                else
                    fprintf(stdout, "\t\tTrasducerSetData:\n");
                fprintf(stdout,
                        "\t\t\tName: %s\n\t\t\tValue: %s\n\t\t\tTimestamp: %s\n",
                        t->name, t->value, t->timestamp);
                t = t->next;
            }
            break;
        case MIO_PACKET_SUBSCRIPTIONS:
            sub = (mio_subscription_t*) packet->payload;
            fprintf(stdout, "MIO Subscriptions Packet:\n");
            if (sub == NULL ) {
                fprintf(stdout, "\t No subscriptions\n");
                break;
            }
            while (sub != NULL ) {
                fprintf(stdout, "\tSubscription: %s\n\t\tSub ID: %s\n",
                        sub->subscription, sub->sub_id);
                sub = sub->next;
            }
            break;
        case MIO_PACKET_AFFILIATIONS:
            aff = (mio_affiliation_t*) packet->payload;
            fprintf(stdout, "MIO Affiliations Packet:\n");
            if (aff == NULL ) {
                fprintf(stdout, "\t No affiliations\n");
                break;
            }
            while (aff != NULL ) {
                fprintf(stdout, "\tAffiliation: %s\n", aff->affiliation);

                switch (aff->type) {
                case MIO_AFFILIATION_NONE:
                    fprintf(stdout, "\t\tType: None\n");
                    break;
                case MIO_AFFILIATION_OWNER:
                    fprintf(stdout, "\t\tType: Owner\n");
                    break;
                case MIO_AFFILIATION_MEMBER:
                    fprintf(stdout, "\t\tType: Member\n");
                    break;
                case MIO_AFFILIATION_PUBLISHER:
                    fprintf(stdout, "\t\tType: Publisher\n");
                    break;
                case MIO_AFFILIATION_PUBLISH_ONLY:
                    fprintf(stdout, "\t\tType: Publish-only\n");
                    break;
                case MIO_AFFILIATION_OUTCAST:
                    fprintf(stdout, "\t\tType: Outcast\n");
                    break;
                default:
                    mio_error("Unknown or missing affiliation type");
                    return MIO_ERROR_UNKNOWN_AFFILIATION_TYPE;
                    break;
                }
                aff = aff->next;
            }
            break;
        case MIO_PACKET_COLLECTIONS:
            coll = (mio_collection_t*) packet->payload;
            fprintf(stdout, "MIO Collections Packet:\n");
            if (coll == NULL ) {
                fprintf(stdout, "\t No associated collection\n");
                break;
            }
            while (coll != NULL ) {
                fprintf(stdout, "\tName: %s\n", coll->name);
                fprintf(stdout, "\t\tNode: %s\n", coll->node);
                coll = coll->next;
            }
            break;
        case MIO_PACKET_META:
            meta = (mio_meta_t *) packet->payload;
            fprintf(stdout, "MIO Meta Packet:\n");
            switch (meta->meta_type) {
            case MIO_META_TYPE_DEVICE:
                fprintf(stdout, "\tDevice:\n");
                break;
            case MIO_META_TYPE_LOCATION:
                fprintf(stdout, "\tLocation:\n");
                break;
            default:
                fprintf(stdout, "\tNo associated meta data\n");
                break;
            }
            if (meta->name != NULL )
                fprintf(stdout, "\t\tName: %s\n", meta->name);
            if (meta->info != NULL )
                fprintf(stdout, "\t\tInfo: %s\n", meta->info);
            if (meta->timestamp != NULL )
                fprintf(stdout, "\t\tTimestamp: %s\n", meta->timestamp);
            if (meta->geoloc != NULL )
                mio_geoloc_print(meta->geoloc, "\t\t");

            t_meta = meta->transducers;
            while (t_meta != NULL ) {
                fprintf(stdout, "\t\tTransducer:\n");
                if (t_meta->name != NULL )
                    fprintf(stdout, "\t\t\tName: %s\n", t_meta->name);
                if (t_meta->type != NULL )
                    fprintf(stdout, "\t\t\tType: %s\n", t_meta->type);
                if (t_meta->interface != NULL )
                    fprintf(stdout, "\t\t\tInterface: %s\n", t_meta->interface);
                if (t_meta->unit != NULL )
                    fprintf(stdout, "\t\t\tUnit: %s\n", t_meta->unit);
                if (t_meta->interface != NULL )
                    fprintf(stdout, "\t\t\tInterface: %s\n", t_meta->interface);
                if (t_meta->enumeration != NULL ) {
                    fprintf(stdout, "\t\t\tEnum Map:\n");
                    for (e_meta = t_meta->enumeration; e_meta != NULL ; e_meta =
                                e_meta->next) {
                        fprintf(stdout, "\t\t\t\tName: %s\n", e_meta->name);
                        fprintf(stdout, "\t\t\t\tValue: %s\n", e_meta->value);
                    }
                }
                if (t_meta->properties != NULL ) {
                    fprintf(stdout, "\t\t\tProperty:\n");
                    for (p_meta = t_meta->properties; p_meta != NULL ; p_meta =
                                p_meta->next) {
                        fprintf(stdout, "\t\t\t\tName: %s\n", p_meta->name);
                        fprintf(stdout, "\t\t\t\tValue: %s\n", p_meta->value);
                    }
                }
                if (t_meta->manufacturer != NULL )
                    fprintf(stdout, "\t\t\tManufacturer: %s\n",
                            t_meta->manufacturer);
                if (t_meta->serial != NULL )
                    fprintf(stdout, "\t\t\tSerial: %s\n", t_meta->serial);
                if (t_meta->resolution != NULL )
                    fprintf(stdout, "\t\t\tResolution: %s\n",
                            t_meta->resolution);
                if (t_meta->accuracy != NULL )
                    fprintf(stdout, "\t\t\tAccuracy: %s\n", t_meta->accuracy);
                if (t_meta->precision != NULL )
                    fprintf(stdout, "\t\t\tPrecision: %s\n", t_meta->precision);
                if (t_meta->min_value != NULL )
                    fprintf(stdout, "\t\t\tMinimum Value: %s\n",
                            t_meta->min_value);
                if (t_meta->max_value != NULL )
                    fprintf(stdout, "\t\t\tMaximum Value: %s\n",
                            t_meta->max_value);
                if (t_meta->info != NULL )
                    fprintf(stdout, "\t\t\tInfo: %s\n", t_meta->info);
                if (t_meta->geoloc != NULL )
                    mio_geoloc_print(t_meta->geoloc, "\t\t\t");
                t_meta = t_meta->next;
            }
            p_meta = meta->properties;
            while (p_meta != NULL ) {
                fprintf(stdout, "\t\tProperty:\n");
                if (p_meta->name != NULL )
                    fprintf(stdout, "\t\t\tName: %s\n", p_meta->name);
                if (p_meta->value != NULL )
                    fprintf(stdout, "\t\t\tValue: %s\n", p_meta->value);
                p_meta = p_meta->next;
            }
            break;
        case MIO_PACKET_SCHEDULE:
            event = (mio_event_t*) packet->payload;
            fprintf(stdout, "MIO Schedule Packet:");
            if (event == NULL ) {
                fprintf(stdout, "\n\tNo associated schedule\n");
                break;
            }
            while (event != NULL ) {
                fprintf(stdout, "\n\tID: %u\n", event->id);
                if (event->time != NULL )
                    fprintf(stdout, "\tTime: %s\n", event->time);
                if (event->tranducer_name != NULL )
                    fprintf(stdout, "\tTransducer Name: %s\n",
                            event->tranducer_name);
                if (event->transducer_value != NULL )
                    fprintf(stdout, "\tTransducer Value: %s\n",
                            event->transducer_value);
                if (event->info != NULL )
                    fprintf(stdout, "\tInfo: %s\n", event->info);
                if (event->recurrence != NULL ) {
                    fprintf(stdout, "\t\tRecurrence:\n");
                    if (event->recurrence->freq != NULL )
                        fprintf(stdout, "\t\t\tFreqency: %s\n",
                                event->recurrence->freq);
                    if (event->recurrence->interval != NULL )
                        fprintf(stdout, "\t\t\tInterval: %s\n",
                                event->recurrence->interval);
                    if (event->recurrence->count != NULL )
                        fprintf(stdout, "\t\t\tCount: %s\n",
                                event->recurrence->count);
                    if (event->recurrence->until != NULL )
                        fprintf(stdout, "\t\t\tUntil: %s\n",
                                event->recurrence->until);
                    if (event->recurrence->bymonth != NULL )
                        fprintf(stdout, "\t\t\tBy Month: %s\n",
                                event->recurrence->bymonth);
                    if (event->recurrence->byday != NULL )
                        fprintf(stdout, "\t\t\tBy Day: %s\n",
                                event->recurrence->byday);
                    if (event->recurrence->exdate != NULL )
                        fprintf(stdout, "\t\t\tEx Date: %s\n",
                                event->recurrence->exdate);
                }
                event = event->next;
            }
            break;
        case MIO_PACKET_NODE_TYPE:
            type = (mio_node_type_t*) packet->payload;
            fprintf(stdout, "MIO Node Type Packet:\n");
            if (*type == MIO_NODE_TYPE_COLLECTION)
                fprintf(stdout, "\tNode Type: Collection\n");
            else if (*type == MIO_NODE_TYPE_LEAF)
                fprintf(stdout, "\tNode Type: Leaf\n");
            else
                fprintf(stdout, "\tNode Type: Unknown\n");
            break;
        case MIO_PACKET_REFERENCES:
            ref = (mio_reference_t*) packet->payload;
            fprintf(stdout, "MIO References Packet:\n");
            if (ref == NULL ) {
                fprintf(stdout, "\tNo associated references\n");
                break;
            }
            while (ref != NULL ) {
                fprintf(stdout, "\tNode: %s\n", ref->node);
                fprintf(stdout, "\tName: %s\n", ref->name);
                switch (ref->type) {
                case MIO_REFERENCE_CHILD:
                    fprintf(stdout, "\tType: Child\n");
                    break;
                case MIO_REFERENCE_PARENT:
                    fprintf(stdout, "\tType: Parent\n");
                    break;
                default:
                    fprintf(stdout, "\n\tType: Unknown\n");
                    break;
                }
                switch (ref->meta_type) {
                case MIO_META_TYPE_DEVICE:
                    fprintf(stdout, "\tMeta Type: Device\n\n");
                    break;
                case MIO_META_TYPE_LOCATION:
                    fprintf(stdout, "\tMeta Type: Location\n\n");
                    break;
                default:
                    fprintf(stdout, "\tMeta Type: Unknown\n");
                    break;
                }
                ref = ref->next;
            }
            break;

        default:
            mio_error("Unknown packet type");
            return MIO_ERROR_UNKNOWN_PACKET_TYPE;
            break;
        }
        break;
    default:
        mio_error("Unknown response type");
        return MIO_ERROR_UNKNOWN_RESPONSE_TYPE;
        break;
    }

    return MIO_OK;
}





