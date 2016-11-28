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
#include "mio_error.h"
#include "mio_handlers.h"
#include "mio_node.h"
#include "mio_pubsub.h"
#include <mio_reference.h>

extern mio_log_level_t _mio_log_level;

void _mio_recurrence_element_add(mio_conn_t *conn, xmpp_stanza_t *parent,
                                 char *name, char *text);
void _mio_recurrence_merge(mio_recurrence_t* rec_to_update,
                           mio_recurrence_t *rec);

void XMLCALL mio_XMLstart_references_query(void *data,
        const char *element_name, const char **attr);

/**
 * @ingroup Scheduler
 * Allocates a new mio recurrence. All values inside the returned struct are zeroed.
 *
 * @returns A pointer to a newly allocated mio recurrence.
 */
mio_recurrence_t *mio_recurrence_new() {
    mio_recurrence_t *rec = malloc(sizeof(mio_recurrence_t));
    memset(rec, 0, sizeof(mio_recurrence_t));
    return rec;
}

/**
 * @ingroup Scheduler
 * Frees a mio recurrence.
 *
 * @param rec mio recurrence to free.
 */
void mio_recurrence_free(mio_recurrence_t *rec) {
    if (rec->byday != NULL )
        free(rec->byday);
    if (rec->bymonth != NULL )
        free(rec->bymonth);
    if (rec->count != NULL )
        free(rec->count);
    if (rec->freq != NULL )
        free(rec->freq);
    if (rec->interval != NULL )
        free(rec->interval);
    if (rec->until != NULL )
        free(rec->until);
    if (rec->exdate != NULL )
        free(rec->exdate);
    free(rec);
}

/**
 * @ingroup Scheduler
 * Internal function to add an element to mio recurrence.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param parent The parent xmpp stanza to add the element to
 * @param name A string containing the name of the element to add.
 * @param text A string containing the text of the element to add.
 */
void _mio_recurrence_element_add(mio_conn_t *conn, xmpp_stanza_t *parent,
                                 char *name, char *text) {
    xmpp_stanza_t *child, *child_text;
    child = xmpp_stanza_new(conn->xmpp_conn->ctx);
    child_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(child, name);
    xmpp_stanza_set_text(child_text, text);
    xmpp_stanza_add_child(child, child_text);
    xmpp_stanza_add_child(parent, child);
    xmpp_stanza_release(child_text);
    xmpp_stanza_release(child);
}

/**
 * @ingroup Scheduler
 * Internal function to merge a new occurrence with an existing one. Any members populated in rec will be used to overwrite the corresponding members in rec_to_update.
 *
 * @param rec_to_update The mio reference to be updated
 * @param rec_to_update The mio reference containing the up to date data.
 */
void _mio_recurrence_merge(mio_recurrence_t* rec_to_update,
                           mio_recurrence_t *rec) {
    if (rec->byday != NULL ) {
        if (rec_to_update->byday != NULL )
            free(rec_to_update->byday);
        rec_to_update->byday = strdup(rec->byday);
    }
    if (rec->bymonth != NULL ) {
        if (rec_to_update->bymonth != NULL )
            free(rec_to_update->bymonth);
        rec_to_update->bymonth = strdup(rec->bymonth);
    }
    if (rec->count != NULL ) {
        if (rec_to_update->count != NULL )
            free(rec_to_update->count);
        rec_to_update->count = strdup(rec->count);
    }
    if (rec->freq != NULL ) {
        if (rec_to_update->freq != NULL )
            free(rec_to_update->freq);
        rec_to_update->freq = strdup(rec->freq);
    }
    if (rec->interval != NULL ) {
        if (rec_to_update->interval != NULL )
            free(rec_to_update->interval);
        rec_to_update->interval = strdup(rec->interval);
    }
    if (rec->until != NULL ) {
        if (rec_to_update->until != NULL )
            free(rec_to_update->until);
        rec_to_update->until = strdup(rec->until);
    }
    if (rec->exdate != NULL ) {
        if (rec_to_update->exdate != NULL )
            free(rec_to_update->exdate);
        rec_to_update->exdate = strdup(rec->exdate);
    }
}

/**
 * @ingroup Scheduler
 * Internal function to convert a mio recurrence to a mio stanza.
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param rec The mio reference to be converted to a mio stanza
 * @returns A mio stanza containing the recurrence.
 */
static mio_stanza_t *_mio_recurrence_to_stanza(mio_conn_t *conn,
        mio_recurrence_t* rec) {
    xmpp_stanza_t *recurrence;
    mio_stanza_t *ret = mio_stanza_new(conn);

    // Create recurrence stanza, children and add them
    recurrence = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(recurrence, "recurrence");
    if (rec->freq != NULL )
        _mio_recurrence_element_add(conn, recurrence, "freq", rec->freq);
    if (rec->interval != NULL )
        _mio_recurrence_element_add(conn, recurrence, "interval",
                                    rec->interval);
    if (rec->count != NULL )
        _mio_recurrence_element_add(conn, recurrence, "count", rec->count);
    if (rec->until != NULL )
        _mio_recurrence_element_add(conn, recurrence, "until", rec->until);
    if (rec->bymonth != NULL )
        _mio_recurrence_element_add(conn, recurrence, "bymonth", rec->bymonth);
    if (rec->byday != NULL )
        _mio_recurrence_element_add(conn, recurrence, "byday", rec->bymonth);
    if (rec->exdate != NULL )
        _mio_recurrence_element_add(conn, recurrence, "exdate", rec->exdate);

    // Add recurrence stanza to mio_stanza_t struct and return
    ret->xmpp_stanza = recurrence;
    return ret;
}

/**
 * @ingroup Scheduler
 * Converts a linked list of mio events to a mio item stanza
 *
 * @param conn A pointer to a mio connection containing an xmpp context to allocate new stanzas. The connection does not need to be active.
 * @param event A linked list of mio events to be converted.
 * @returns A mio stanza containing the events.
 */
mio_stanza_t *mio_schedule_event_to_item(mio_conn_t* conn, mio_event_t *event) {
    mio_event_t *curr;
    xmpp_stanza_t *event_stanza, *schedule_stanza;
    char id[8];
    mio_stanza_t *recurrence;
    mio_stanza_t *item = mio_pubsub_item_new(conn, "schedule");

    schedule_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(schedule_stanza, "schedule");
    for (curr = event; curr != NULL ; curr = curr->next) {
        event_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(event_stanza, "event");

        // Populate stanza with attributes from event
        snprintf(id, 8, "%u", curr->id);
        xmpp_stanza_set_attribute(event_stanza, "id", id);
        if (curr->info != NULL )
            xmpp_stanza_set_attribute(event_stanza, "info", curr->info);
        if (curr->time != NULL )
            xmpp_stanza_set_attribute(event_stanza, "time", curr->time);
        if (curr->tranducer_name != NULL )
            xmpp_stanza_set_attribute(event_stanza, "transducerName",
                                      curr->tranducer_name);
        if (curr->transducer_value != NULL )
            xmpp_stanza_set_attribute(event_stanza, "transducerValue",
                                      curr->transducer_value);
        if (curr->recurrence != NULL ) {
            recurrence = _mio_recurrence_to_stanza(conn, curr->recurrence);
            xmpp_stanza_add_child(event_stanza, recurrence->xmpp_stanza);
        }

        xmpp_stanza_add_child(schedule_stanza, event_stanza);
        xmpp_stanza_release(event_stanza);
    }
    xmpp_stanza_add_child(item->xmpp_stanza, schedule_stanza);
    xmpp_stanza_release(schedule_stanza);

    return item;
}

/**
 * @ingroup Scheduler
 * Internal function to renumber a linked list of mio events from 0 to n.
 *
 * @param events A linked list of mio events to be renumbered.
 */
static void _mio_event_renumber(mio_event_t *events) {
    mio_event_t *curr;
    int i = 0;
    for (curr = events; curr != NULL ; curr = curr->next) {
        curr->id = i;
        i++;
    }
}

/**
 * @ingroup Scheduler
 * Removes an event according to its ID from a linked list of mio events.
 *
 * @param events The linked list of mio events to remove the event from
 * @param id The integer ID of the event to be removed.
 * @returns A pointer to the first element of the linked list of mio events which had the event removed. Will return NULL if no events remain in the list.
 */
mio_event_t *mio_schedule_event_remove(mio_event_t *events, int id) {
    mio_event_t *prev = NULL, *curr;
    curr = events;
    while (curr != NULL ) {
        if (curr->id == id) {
            if (prev == NULL ) {
                curr = curr->next;
                _mio_event_renumber(curr);
                return curr;
            }
            prev->next = curr->next;
            curr->next = NULL;
            mio_event_free(curr);
            _mio_event_renumber(events);
            return events;
        }
        prev = curr;
        curr = curr->next;
    }
    _mio_event_renumber(events);
    return events;
}

/**
 * @ingroup Scheduler
 * Merge a new event with an existing one contained in a linked list of mio event. Events are merged by ID.
 *
 * @param event_to_update A linked list of mio events to be updated.
 * @param event The mio event containing the up to date data.
 */
void mio_schedule_event_merge(mio_event_t *event_to_update, mio_event_t *event) {
    mio_event_t *curr;
    while (event != NULL ) {
        for (curr = event_to_update; curr != NULL ; curr = curr->next) {
            if (curr->id == event->id)
                break;
        }
        if (curr == NULL ) {
            curr = mio_event_tail_get(event_to_update);
            curr->next = mio_event_new();
            curr = curr->next;
        }
        if (event->info != NULL ) {
            if (curr->info != NULL )
                free(curr->info);
            curr->info = strdup(event->info);
        }
        if (event->recurrence != NULL ) {
            if (curr->recurrence != NULL )
                mio_recurrence_free(curr->recurrence);
            curr->recurrence = mio_recurrence_new();
            _mio_recurrence_merge(curr->recurrence, event->recurrence);
        }
        if (event->time != NULL ) {
            if (curr->time != NULL )
                free(curr->time);
            curr->time = strdup(event->time);
        }
        if (event->tranducer_name != NULL ) {
            if (curr->tranducer_name != NULL )
                free(curr->tranducer_name);
            curr->tranducer_name = strdup(event->tranducer_name);
        }
        if (event->transducer_value != NULL ) {
            if (curr->transducer_value != NULL )
                free(curr->transducer_value);
            curr->transducer_value = strdup(event->transducer_value);
        }
        event = event->next;
    }
// Renumber event IDs
    _mio_event_renumber(event_to_update);
}

/**
 * @ingroup Scheduler
 * Merges an existing schedule at an XMPP event node with a linked list of events. Events are merged by ID.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the event node to publish to.
 * @param event A linked list of events containing the up to date data to be merged.
 * @param response A pointer to an allocated mio response which will contain the XMPP server's response to the publication being sent out.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int mio_schedule_event_merge_publish(mio_conn_t *conn, char *node,
                                     mio_event_t *event, mio_response_t *response) {
    int err;
    mio_packet_t *packet;
    mio_event_t *query_events;
    mio_stanza_t *item;
    mio_response_t *query_response = mio_response_new();

// Query existing schedule
    mio_schedule_query(conn, node, query_response);
    if (query_response->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) query_response->response;
        // If the node has a schedule, merge it with the new events
        if (packet->type != MIO_PACKET_SCHEDULE) {
            response->response = query_response->response;
            response->response_type = query_response->response_type;
            query_response->response = NULL;
            mio_response_free(query_response);
            return MIO_ERROR_UNEXPECTED_PAYLOAD;
        }
        query_events = (mio_event_t*) packet->payload;
        if(query_events != NULL) {
            mio_schedule_event_merge(query_events, event);
            item = mio_schedule_event_to_item(conn, query_events);
        } else {
            _mio_event_renumber(event);
            item = mio_schedule_event_to_item(conn, event);
        }
    } else {
        // Ensure that events are properly numbered
        _mio_event_renumber(event);
        item = mio_schedule_event_to_item(conn, event);
    }
    err = mio_item_publish(conn, item, node, response);
    mio_response_free(query_response);
    return err;
}

/**
 * @ingroup Scheduler
 * Removes an event from an existing schedule at an XMPP event node.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the event node to publish to.
 * @param id The integer ID of the event to be removed.
 * @param response A pointer to an allocated mio response which will contain the XMPP server's response to the publication being sent out.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int mio_schedule_event_remove_publish(mio_conn_t *conn, char *node, int id,
                                      mio_response_t *response) {
    int err;
    mio_packet_t *packet;
    mio_event_t *query_events;
    mio_stanza_t *item;
    mio_response_t *query_response = mio_response_new();

// Query existing schedule
    mio_schedule_query(conn, node, query_response);
    if (query_response->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) query_response->response;
        // If the node has a schedule, merge it with the new events
        if (packet->type != MIO_PACKET_SCHEDULE) {
            response->response = query_response->response;
            response->response_type = query_response->response_type;
            query_response->response = NULL;
            mio_response_free(query_response);
            return MIO_ERROR_UNEXPECTED_PAYLOAD;
        }
        query_events = (mio_event_t*) packet->payload;
        query_events = mio_schedule_event_remove(query_events, id);
        packet->payload = query_events;
        item = mio_schedule_event_to_item(conn, query_events);
    } else {
        response->response = query_response->response;
        response->response_type = query_response->response_type;
        query_response->response = NULL;
        mio_response_free(query_response);
        return MIO_ERROR_UNEXPECTED_RESPONSE;
    }
    err = mio_item_publish(conn, item, node, response);
    mio_response_free(query_response);
    return err;
}

/**
 * @ingroup Scheduler
 * Queries an XMPP event node for its current schedule.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the event node to publish to.
 * @param response A pointer to an allocated mio response which will contain the XMPP server's response to the query being sent out.
 * @returns MIO_OK on success, otherwise a non-zero integer error number.
 */
int mio_schedule_query(mio_conn_t *conn, const char *node,
                       mio_response_t *response) {

    return mio_item_recent_get(conn, node, response, 1, "schedule",
                               (mio_handler*) mio_handler_schedule_query);
}


static void XMLCALL mio_XMLString_recurrence(void *data, const XML_Char *s,
        int len) {
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_event_t *event = (mio_event_t*) xml_data->payload;
    mio_recurrence_t *rec = event->recurrence;

    if (strcmp(xml_data->curr_element_name, "freq") == 0 && rec->freq == NULL ) {
        rec->freq = malloc(sizeof(char) * len + 1);
        snprintf(rec->freq, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "interval")
            == 0&& rec->interval == NULL) {
        rec->interval = malloc(sizeof(char) * len + 1);
        snprintf(rec->interval, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "count")
            == 0&& rec->count == NULL) {
        rec->count = malloc(sizeof(char) * len + 1);
        snprintf(rec->count, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "until")
            == 0&& rec->until == NULL) {
        rec->until = malloc(sizeof(char) * len + 1);
        snprintf(rec->until, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "bymonth")
            == 0&& rec->bymonth == NULL) {
        rec->bymonth = malloc(sizeof(char) * len + 1);
        snprintf(rec->bymonth, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "byday")
            == 0&& rec->byday == NULL) {
        rec->byday = malloc(sizeof(char) * len + 1);
        snprintf(rec->byday, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "exdate")
            == 0&& rec->exdate == NULL) {
        rec->exdate = malloc(sizeof(char) * len + 1);
        snprintf(rec->exdate, len + 1, "%s", s);
    }
}

static void XMLCALL mio_XMLstart_schedule_query(void *data,
        const char *element_name, const char **attr) {

    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    mio_event_t *event = NULL;
    mio_recurrence_t *recurrence;
    xml_data->prev_element_name = xml_data->curr_element_name;
    xml_data->curr_element_name = element_name;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "items") == 0) {
        response->response_type = MIO_RESPONSE_PACKET;
        packet->type = MIO_PACKET_SCHEDULE;
    }
    if (strcmp(element_name, "event") == 0) {
        XML_SetCharacterDataHandler(*xml_data->parser, NULL );
        if (packet->payload == NULL ) {
            packet->payload = mio_event_new();
            event = (mio_event_t*) packet->payload;
        } else {
            event = (mio_event_t*) packet->payload;
            while (event->next != NULL )
                event = event->next;
            event->next = mio_event_new();
            event = event->next;
        }
        xml_data->payload = event;
        packet->num_payloads++;

        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "time") == 0) {
                event->time = strdup(attr[i + 1]);
            } else if (strcmp(attr_name, "info") == 0) {
                event->info = strdup(attr[i + 1]);
            } else if (strcmp(attr_name, "transducerName") == 0) {
                event->tranducer_name = strdup(attr[i + 1]);
            } else if (strcmp(attr_name, "transducerValue") == 0) {
                event->transducer_value = strdup(attr[i + 1]);
            } else if (strcmp(attr_name, "id") == 0) {
                sscanf(attr[i + 1], "%u", &event->id);
            }
        }
    } else if (strcmp(element_name, "recurrence") == 0) {
        event = (mio_event_t*) xml_data->payload;
        recurrence = mio_recurrence_new();
        event->recurrence = recurrence;
        XML_SetCharacterDataHandler(*xml_data->parser,
                                    mio_XMLString_recurrence);
    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}

int mio_handler_schedule_query(mio_conn_t * const conn,
                               mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
    mio_stanza_t *stanza_copy;
    int err;
    mio_request_t *request = _mio_request_get(conn, response->id);
    if (request == NULL ) {
        mio_error("Request with id %s not found, aborting handler",
                  response->id);
        return MIO_ERROR_REQUEST_NOT_FOUND;
    }
    mio_packet_t *packet = mio_packet_new();
    mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
    xml_data->response = response;
    stanza_copy = mio_stanza_clone(conn, stanza);
    response->stanza = stanza_copy;

    response->response = packet;
    err = mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_schedule_query,
                        NULL );

    if (err == MIO_OK) {
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
    }

    return err;
}







