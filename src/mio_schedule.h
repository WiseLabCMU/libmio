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


#ifndef ____mio_schedule__
#define ____mio_schedule__

#include <stdio.h>
//#include <mio_connection.h>
#include <mio.h>

typedef struct mio_recurrence {
    char *freq;
    char *interval;
    char *count;
    char *until;
    char *bymonth;
    char *byday;
    char *exdate;
} mio_recurrence_t;

typedef struct mio_event mio_event_t;

struct mio_event {
    int id;
    char *tranducer_name;
    char *transducer_value;
    char *time;
    char *info;
    mio_recurrence_t *recurrence;
    mio_event_t *next;
};

mio_recurrence_t *mio_recurrence_new();
void mio_recurrence_free(mio_recurrence_t *rec);

mio_event_t *mio_schedule_event_remove(mio_event_t *events, int id);
void mio_schedule_event_merge(mio_event_t *event_to_update, mio_event_t *event);
mio_event_t * mio_event_new();
void mio_event_free(mio_event_t *event);
mio_event_t *mio_event_tail_get(mio_event_t *event);
int mio_schedule_query(mio_conn_t *conn, const char *event,
                       mio_response_t *response);
int mio_schedule_event_merge_publish(mio_conn_t *conn, char *node,
                                     mio_event_t *event, mio_response_t *response);
int mio_schedule_event_remove_publish(mio_conn_t *conn, char *node, int id,
                                      mio_response_t *response);
mio_stanza_t *mio_schedule_event_to_item(mio_conn_t* conn, mio_event_t *event);
int mio_handler_schedule_query(mio_conn_t * const conn,
                               mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
#endif /* defined(____mio_schedule__) */
