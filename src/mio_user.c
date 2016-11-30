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

#include "mio_user.h"
#include <mio_connection.h>
#include <common.h>
#include <strophe.h>
#include <mio_pubsub.h>
#include <mio_error.h>
#include <mio_handlers.h>
#include <mio_meta.h>
#include <mio_schedule.h>
#include <mio_reference.h>
#include <mio_node.h>
#include <mio_collection.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <mio_geolocation.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

extern mio_log_level_t _mio_log_level;

int mio_handler_password_change(mio_conn_t * const conn,
                                mio_stanza_t * const stanza, mio_data_t * const mio_data);
/** Allows user to change passowrd for connected jid.
 *
 * @param conn Active MIO connection.
 * @param new_pass The new password to connect via the jid logged in with.
 * @param response The response to the change password request.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on disconnect.
 * */
int mio_password_change(mio_conn_t * conn, const char *new_pass,
                        mio_response_t * response) {

    xmpp_stanza_t *iq = NULL, *query = NULL, *username = NULL, *username_tag =
                                           NULL, *password = NULL, *password_tag = NULL;
    char *server = NULL, *id = NULL, *user = NULL;
    int user_length = 0, err;
    mio_stanza_t *stanza = mio_stanza_new(conn);

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process subscribe request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

// Extract username from JID
    user_length = strlen(conn->xmpp_conn->jid) - strlen(server) - 1;
    user = malloc((user_length + 1) * sizeof(char));
    user = strncpy(user, conn->xmpp_conn->jid, user_length);

// Create a new iq stanza
    iq = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(iq, "iq");
    xmpp_stanza_set_type(iq, "set");
    xmpp_stanza_set_attribute(iq, "to", server);
    xmpp_stanza_set_attribute(iq, "id", id);

// Create query stanza
    query = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, "jabber:iq:register");

// Create username stanza
    username = xmpp_stanza_new(conn->xmpp_conn->ctx);
    username_tag = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(username, "username");
    xmpp_stanza_set_text(username_tag, user);

// Create password stanza
    password = xmpp_stanza_new(conn->xmpp_conn->ctx);
    password_tag = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(password, "password");
    xmpp_stanza_set_text(password_tag, new_pass);

// Build xmpp message
    xmpp_stanza_add_child(password, password_tag);
    xmpp_stanza_add_child(username, username_tag);
    xmpp_stanza_add_child(query, username);
    xmpp_stanza_add_child(query, password);
    xmpp_stanza_add_child(iq, query);
    stanza->xmpp_stanza = iq;

// Release unneeded stanzas and strings
    xmpp_stanza_release(query);
    xmpp_stanza_release(username);
    xmpp_stanza_release(password);
    xmpp_stanza_release(username_tag);
    xmpp_stanza_release(password_tag);
    free(user);

// Send out the stanza
    err = mio_send_blocking(conn, stanza, (mio_handler) mio_handler_error,
                            response);

// Release the stanzas
    mio_stanza_free(stanza);

    return err;
}

/**
 * @ingroup Internal
 * Main event loop thread which connects to the XMPP server passed into mio_connect() and processes incoming and outgoing communication.
 *
 * @param mio_handler_data mio handler data struct containing the mio conn used in the connection.
 */
static void *_mio_run(mio_handler_data_t *mio_handler_data) {

    struct timespec ts;
    struct timeval tp, tp_add;
    int err;
    mio_conn_t *conn = mio_handler_data->conn;
    xmpp_ctx_t *ctx = conn->xmpp_conn->ctx;

    tp_add.tv_sec = 0;
    tp_add.tv_usec = MIO_SEND_REQUEST_TIMEOUT;


    ctx->loop_status = XMPP_LOOP_RUNNING;
    while (ctx->loop_status == XMPP_LOOP_RUNNING) {
        // Only run the event loop if we have locked down the mutex
        // This prevents interleaving on the send queue
        err = pthread_mutex_lock(&conn->event_loop_mutex);
        if (err != 0) {
            mio_error("Unable to lock event loop mutex");
            pthread_exit((void*) MIO_ERROR_MUTEX);
        }

        xmpp_run_once(ctx, MIO_EVENT_LOOP_TIMEOUT);
        pthread_mutex_unlock(&conn->event_loop_mutex);

        // Set timeout
        gettimeofday(&tp, NULL);
        ts.tv_sec = tp.tv_sec;
        timeradd(&tp, &tp_add, &tp);
        ts.tv_nsec = tp.tv_usec * 1000;

        err = pthread_mutex_lock(&conn->send_request_mutex);
        if (err != 0) {
            mio_error("Unable to lock send request mutex");
            pthread_exit((void*) MIO_ERROR_MUTEX);
        }
        // Wait for MIO_SEND_REQUEST_TIMEOUT or until new send request comes in (whichever is first) to run event loop again
        while (!conn->send_request_predicate) {
            err = pthread_cond_timedwait(&conn->send_request_cond,
                                         &conn->send_request_mutex, &ts);
            // If we time out, run the event loop
            if (err == ETIMEDOUT)
                conn->send_request_predicate = 1;
            // If waiting for the condition fails other than for a timeout, sleep instead
            else if (err != 0)
                usleep(MIO_SEND_REQUEST_TIMEOUT);
        }
        conn->send_request_predicate = 0;
        pthread_mutex_unlock(&conn->send_request_mutex);
    }
    mio_debug("Event loop completed");

    pthread_exit((void*) MIO_OK);
}

/**
 * @ingroup Core
 * Set up a connection to an XMPP server and start running the internal event loop.
 *
 * @param jid The full JID of the user connecting to the XMPP server.
 * @param pass The password of the user connecting to the XMPP server.
 * @param conn_handler A pointer to a user defined connection handler (optional). Pass NULL if no user defined connection handler should be used.
 * @param conn_handler_user_data A pointer to user defined data to be passed to the user defined connection handler (optional).
 * @param conn A pointer to an inactive mio conn struct.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_connect(char *jid, char *pass, mio_handler_conn conn_handler,
                void *conn_handler_user_data, mio_conn_t *conn) {

    int err = MIO_OK;
    mio_handler_data_t *shd;
    mio_response_t *response;
    mio_response_error_t *response_err;
    struct timespec ts;
    struct timeval tp;
    xmpp_ctx_t *ctx;
    pthread_attr_t attr;

    shd = _mio_handler_data_new();
    shd->conn = conn;
    shd->userdata = conn_handler_user_data;
    shd->conn_handler = conn_handler;
    shd->response = mio_response_new();

    sem_unlink(jid);
    conn->mio_open_requests = sem_open(jid, O_CREAT, S_IROTH | S_IWOTH,
                                       MIO_MAX_OPEN_REQUESTS);

// Initialize libstrophe
    xmpp_initialize();

// Setup authentication information
    xmpp_conn_set_jid(conn->xmpp_conn, jid);
    xmpp_conn_set_pass(conn->xmpp_conn, pass);

    ctx = conn->xmpp_conn->ctx;
    // Ignore broken pipe signal
    signal(SIGPIPE, SIG_IGN);

// Initiate connection
    mio_info("Connecting to XMPP server %s using JID %s",
             shd->conn->xmpp_conn->domain,
             shd->conn->xmpp_conn->jid);
    err = xmpp_connect_client(shd->conn->xmpp_conn, NULL, 0,
                              mio_handler_conn_generic, shd);
    if (err < 0) {
        mio_error("Error connecting to XMPP server %s using JID %s",
                  shd->conn->xmpp_conn->domain,
                  shd->conn->xmpp_conn->jid);
	fprintf(stdout,"Error connecting %d %d\n", MIO_OK, MIO_ERROR_CONNECTION);
        return MIO_ERROR_CONNECTION;
    }

// Add keepalive handler
    mio_handler_timed_add(shd->conn,
                          (mio_handler) mio_handler_keepalive, KEEPALIVE_PERIOD, NULL);

    mio_debug("Starting Event Loop");
    conn->presence_status = MIO_PRESENCE_UNKNOWN;

// Run event loop
    if (ctx->loop_status != XMPP_LOOP_NOTSTARTED) {
        mio_error("Cannot run event loop because not started");
        return MIO_ERRROR_EVENT_LOOP_NOT_STARTED;
    }
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    conn->mio_run_thread = malloc(sizeof(pthread_t));
    err = pthread_create(conn->mio_run_thread, &attr, (void *) _mio_run, shd);
    pthread_attr_destroy(&attr);

    if (err == 0)
        mio_debug("mio_run thread successfully spawned.");
    else {
        mio_error("Error spawning mio_run thread, pthread error code %d", err);
        return MIO_ERROR_RUN_THREAD;
    }

// Set timeout
    gettimeofday(&tp, NULL);
    ts.tv_sec = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += MIO_REQUEST_TIMEOUT_S;

    pthread_mutex_lock(&conn->conn_mutex);
// Wait for connection to be established before returning
    while (!conn->conn_predicate) {
        err = pthread_cond_timedwait(&conn->conn_cond, &conn->conn_mutex, &ts);
        if (err == ETIMEDOUT) {
            mio_error("Connection attempt timed out");
            pthread_mutex_unlock(&conn->conn_mutex);
            mio_handler_data_free(shd);
            conn->conn_predicate = 0;
            return MIO_ERROR_TIMEOUT;
        } else if (err != 0) {
            mio_error("Conditional wait for connection attempt timed out");
            pthread_mutex_unlock(&conn->conn_mutex);
            mio_handler_data_free(shd);
            conn->conn_predicate = 0;
            return MIO_ERROR_COND_WAIT;
        } else {
            // On success, do not free shd until disconnected
            response = shd->response;
            if (response->response_type == MIO_RESPONSE_OK) {
                mio_info("Connected to XMPP server %s using JID %s",
                         conn->xmpp_conn->domain, conn->xmpp_conn->jid);
                err = MIO_OK;
            } else {
                response_err = response->response;
                err = response_err->err_num;
                mio_response_print(response);
            }
            pthread_mutex_unlock(&conn->conn_mutex);
            conn->conn_predicate = 0;
            mio_response_free(response);
            return err;
        }
    }
    fprintf(stdout,"ERROR rpredicate\n");
    mio_handler_data_free(shd);
    return MIO_ERROR_PREDICATE;
    //return !MIO_OK;
}

int mio_reconnect(mio_conn_t *conn) {

    mio_handler_data_t *shd;
    int err = -1;
    xmpp_conn_t *new_conn;
    xmpp_connlist_t *item, *prev;

    if (conn->xmpp_conn->state == XMPP_STATE_CONNECTED)
        return MIO_OK;

    do {
        // Free unneeded elements in connection
        if (conn->xmpp_conn->tls) {
            tls_stop(conn->xmpp_conn->tls);
            tls_free(conn->xmpp_conn->tls);
        }

        if (conn->xmpp_conn->stream_error) {
            xmpp_stanza_release(conn->xmpp_conn->stream_error->stanza);
            if (conn->xmpp_conn->stream_error->text)
                xmpp_free(conn->xmpp_conn->ctx,
                          conn->xmpp_conn->stream_error->text);
            xmpp_free(conn->xmpp_conn->ctx, conn->xmpp_conn->stream_error);
            conn->xmpp_conn->stream_error = NULL;
        }

        parser_free(conn->xmpp_conn->parser);

        if (conn->xmpp_conn->domain) {
            xmpp_free(conn->xmpp_conn->ctx, conn->xmpp_conn->domain);
            conn->xmpp_conn->domain = NULL;
        }
        if (conn->xmpp_conn->bound_jid) {
            xmpp_free(conn->xmpp_conn->ctx, conn->xmpp_conn->bound_jid);
            conn->xmpp_conn->bound_jid = NULL;
        }
        if (conn->xmpp_conn->stream_id) {
            xmpp_free(conn->xmpp_conn->ctx, conn->xmpp_conn->stream_id);
            conn->xmpp_conn->stream_id = NULL;
        }

        // Remove old connection from context's connlist
        if (conn->xmpp_conn->ctx->connlist->conn == conn->xmpp_conn) {
            item = conn->xmpp_conn->ctx->connlist;
            conn->xmpp_conn->ctx->connlist = item->next;
            xmpp_free(conn->xmpp_conn->ctx, item);
        } else {
            prev = NULL;
            item = conn->xmpp_conn->ctx->connlist;
            while (item && item->conn != conn->xmpp_conn) {
                prev = item;
                item = item->next;
            }

            if (!item) {
                xmpp_error(conn->xmpp_conn->ctx, "xmpp",
                           "Connection not in context's list\n");
            } else {
                prev->next = item->next;
                xmpp_free(conn->xmpp_conn->ctx, item);
            }
        }
        // Setup new connection and copy handlers and send queue from old one
        new_conn = xmpp_conn_new(conn->xmpp_conn->ctx);
        xmpp_conn_set_jid(new_conn, conn->xmpp_conn->jid);
        xmpp_conn_set_pass(new_conn, conn->xmpp_conn->pass);
        new_conn->send_queue_head = conn->xmpp_conn->send_queue_head;
        new_conn->send_queue_tail = conn->xmpp_conn->send_queue_tail;
        new_conn->send_queue_len = conn->xmpp_conn->send_queue_len;
        new_conn->send_queue_max = conn->xmpp_conn->send_queue_max;
        new_conn->handlers = conn->xmpp_conn->handlers;
        new_conn->id_handlers = conn->xmpp_conn->id_handlers;
        new_conn->timed_handlers = conn->xmpp_conn->timed_handlers;
        xmpp_free(conn->xmpp_conn->ctx, conn->xmpp_conn);

        conn->xmpp_conn = new_conn;
        shd = _mio_handler_data_new();
        shd->conn = conn;

        // Try to reconnect
        mio_info("Attempting to reconnect to XMPP server %s using JID %s",
                 shd->conn->xmpp_conn->domain, shd->conn->xmpp_conn->jid);
        err = xmpp_connect_client(conn->xmpp_conn, NULL, 0,
                                  mio_handler_conn_generic, shd);
        conn->retries++;
        if (err < 0) {
            mio_error(
                "Error connecting to XMPP server %s using JID %s, retry %d\n",
                shd->conn->xmpp_conn->domain, shd->conn->xmpp_conn->jid,
                conn->retries);
            usleep(1000000 * MIO_RECONNECTION_TIMEOUT_S);
        }
#ifdef MIO_CONNECTION_RETRIES
    } while(err < 0 && conn->retries < MIO_CONNECTION_RETRIES);
#else
    }
    while (err < 0);
#endif
    return MIO_OK;
}

/**
 * @ingroup Core
 * Prints the data contained in a mio response to stdout.
 *
 * @param response A pointer to the mio response to be printed.
 * @returns MIO_OK on success, otherwise an error.
 */

/**
 * @ingroup Core
 * Disconnect from the XMPP server and stop running all mio threads.
 * @param conn A pointer to an active mio conn.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_disconnect(mio_conn_t *conn) {

    mio_listen_stop(conn);
    mio_pubsub_data_listen_stop(conn);
    mio_debug("Stopping libstrophke...");
// Free mio_handler_data in connection struct
//	if (conn->xmpp_conn->userdata != NULL )
//		_mio_handler_data_free(conn->xmpp_conn->userdata);
    xmpp_stop(conn->xmpp_conn->ctx);
    if (conn->xmpp_conn->authenticated) {
        mio_info("Disconnecting from XMPP server %s", conn->xmpp_conn->domain);
        xmpp_disconnect(conn->xmpp_conn);
        mio_info("Disconnected");
    }
    xmpp_shutdown();
    sem_close(conn->mio_open_requests);
    sem_unlink(conn->xmpp_conn->jid);
    pthread_mutex_unlock(&conn->event_loop_mutex);
    return MIO_OK;
}
