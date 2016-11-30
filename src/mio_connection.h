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


#ifndef ____mio_connection__
#define ____mio_connection__

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <expat.h>
#include <strophe.h>
#include <sys/queue.h>
#include <uthash.h>

#define KEEPALIVE_PERIOD 30000 // ms
#define MIO_MAX_OPEN_REQUESTS 	100

#define MIO_BLOCKING 1
#define MIO_NON_BLOCKING 2

#define MIO_REQUEST_TIMEOUT_S		1000
#define MIO_RECONNECTION_TIMEOUT_S	5
//#define MIO_CONNECTION_RETRIES	3	// Comment out to retry indefinitely
#define MIO_SEND_RETRIES			3

#define MIO_RESPONSE_TIMEOUT 10000  //ms
#define MIO_EVENT_LOOP_TIMEOUT 1 //ms
#define MIO_SEND_REQUEST_TIMEOUT 1000 //µs
#define MIO_PUBSUB_RX_QUEUE_MAX_LEN 100

typedef enum {
    MIO_LEVEL_ERROR, MIO_LEVEL_WARN, MIO_LEVEL_INFO, MIO_LEVEL_DEBUG
} mio_log_level_t;

#define mio_debug(...) \
do { if(_mio_log_level == MIO_LEVEL_DEBUG){ fprintf(stderr, "MIO DEBUG: %s:%d:%s(): ", __FILE__, \
                                                    __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }} while (0)

#define mio_info(...) \
do { if(_mio_log_level >= MIO_LEVEL_INFO){ fprintf(stderr, "MIO INFO: %s:%d:%s(): ", __FILE__, \
                                                   __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }} while (0)

#define mio_warn(...) \
do { if(_mio_log_level >= MIO_LEVEL_WARN){ fprintf(stderr, "MIO WARNING: %s:%d:%s(): ", __FILE__, \
                                                   __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }} while (0)

#define mio_error(...) \
do { if(_mio_log_level >= MIO_LEVEL_ERROR){ fprintf(stderr, "MIO ERROR: %s:%d:%s(): ", __FILE__, \
                                                    __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}} while (0)

typedef enum {
    MIO_CONN_CONNECT, MIO_CONN_DISCONNECT, MIO_CONN_FAIL
} mio_conn_event_t;

typedef enum {
    MIO_PRESENCE_UNKNOWN, MIO_PRESENCE_PRESENT, MIO_PRESENCE_UNAVAILABLE
} mio_presence_status_t;

// Request and response

typedef enum {
    MIO_RESPONSE_UNKNOWN,
    MIO_RESPONSE_OK,
    MIO_RESPONSE_ERROR,
    MIO_RESPONSE_PACKET
} mio_response_type_t;

typedef struct mio_response_error {
    int err_num;
    char *description;
} mio_response_error_t;

typedef struct mio_stanza {
    xmpp_stanza_t *xmpp_stanza;
    char id[37];
    struct mio_stanza * next;
} mio_stanza_t;

typedef struct mio_response {
    char id[37];
    char *ns;
    char *name;
    char *type;
    void *response;
    mio_response_type_t response_type;
    TAILQ_ENTRY(mio_response)
    responses;
    mio_stanza_t *stanza;
} mio_response_t;

typedef struct mio_request mio_request_t;


typedef struct {
    xmpp_conn_t *xmpp_conn;
    mio_presence_status_t presence_status;
    pthread_mutex_t event_loop_mutex, send_request_mutex, conn_mutex,
                    pubsub_rx_queue_mutex;
    pthread_cond_t send_request_cond, conn_cond;
    TAILQ_HEAD(mio_pubsub_rx_queue, mio_response)
    pubsub_rx_queue;
    sem_t *mio_open_requests;
    pthread_rwlock_t mio_hash_lock;
    mio_request_t *mio_request_table;
    int pubsub_rx_queue_len, pubsub_rx_listening, send_request_predicate,
        conn_predicate, retries, has_connected;
    pthread_t *mio_run_thread;
} mio_conn_t;

typedef enum {
    MIO_HANDLER, MIO_HANDLER_ID, MIO_HANDLER_TIMED
} mio_handler_type_t;

typedef void (*mio_handler_conn)(mio_conn_t * conn,
                                 const mio_conn_event_t event, const mio_response_t *response,
                                 const void *userdata);

typedef int (*mio_handler)(mio_conn_t * conn, mio_stanza_t * stanza,
                           const mio_response_t *response, const void *userdata);

struct mio_request {
    char id[37];    // UUID of sent stanza as key
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int predicate;
    mio_handler handler; // Handler to be called when response is received
    mio_handler_type_t handler_type;
    mio_response_t *response;
    UT_hash_handle hh;
};


// stanza structure funcitons
void mio_stanza_free(mio_stanza_t *stanza);
mio_stanza_t *mio_stanza_new(mio_conn_t *conn);
mio_stanza_t *mio_stanza_clone(mio_conn_t *conn, mio_stanza_t *stanza);
int mio_stanza_to_text(mio_stanza_t *stanza, char **buf, int *buflen);

// response structure funcitons
void mio_response_free(mio_response_t *response);
void _mio_response_error_free(mio_response_error_t* error);
mio_response_error_t* _mio_response_error_new();
int mio_response_print(mio_response_t *response);

// mio request functions
mio_request_t *_mio_request_new();
int _mio_request_delete(mio_conn_t *conn, char *id);
void _mio_request_free(mio_request_t *request);
int _mio_request_add(mio_conn_t *conn, const char *id,
                     mio_handler handler, mio_handler_type_t handler_type,
                     mio_response_t *response, mio_request_t *request);
mio_request_t *_mio_request_get(mio_conn_t *conn, char *id);

// mio_connection settup
mio_conn_t *mio_conn_new(mio_log_level_t log_level);
void mio_conn_free(mio_conn_t *conn);
int mio_listen_start(mio_conn_t *conn);
int mio_listen_stop(mio_conn_t *conn);

char *mio_get_server(const char *string);

// rx queue functions
int mio_response_print(mio_response_t *response);
mio_response_t *_mio_response_get(mio_conn_t *conn, char *id);
mio_response_t *_mio_pubsub_rx_queue_dequeue(mio_conn_t *conn);
void _mio_pubsub_rx_queue_enqueue(mio_conn_t *conn, mio_response_t *response);
int mio_cond_signal(pthread_cond_t *cond, pthread_mutex_t *mutex,
                    int *predicate);
int mio_cond_broadcast(pthread_cond_t *cond, pthread_mutex_t *mutex,
                       int *predicate);

mio_response_t *mio_stanza_to_response(mio_conn_t *conn, mio_stanza_t *stanza);
mio_response_t *mio_response_new();
#endif /* defined(____mio_connection__) */
