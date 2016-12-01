/******************************************************************************
 *  Mortar IO (MIO) Library
 *  Standard Data Handlers
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

#include <stdio.h>
#include <expat.h>
#include <unistd.h>

#include <common.h>
#include <string.h>
#include <pthread.h>

//#include <mio.h>
#include <mio_connection.h>
#include <mio_handlers.h>
#include <mio_error.h>
#include <mio_node.h>
#include <mio_affiliations.h>
#include <mio_schedule.h>
#include <mio_geolocation.h>
#include <mio_reference.h>
#include <mio_user.h>
#include <mio_meta.h>
#include <mio_pubsub.h>

extern mio_log_level_t _mio_log_level;

int mio_parser_reset(mio_parser_t *parser);
/**
 * @ingroup Internal
 * Internal function to allocate and initialize a new mio handler data struct.
 *
 * @returns A pointer to a newly allocated and initialized mio handler data struct
 */
mio_handler_data_t *_mio_handler_data_new() {
    mio_handler_data_t *shd = (mio_handler_data_t*) malloc(
                                  sizeof(mio_handler_data_t));
    memset(shd, 0, sizeof(mio_handler_data_t));
    return shd;
}

/**
 * @ingroup Internal
 * Internal function to free a mio handler data struct.
 *
 * @param A pointer to the allocated mio handler data struct to be freed.
 */
void mio_handler_data_free(mio_handler_data_t *shd) {
    free(shd);
}

/**
 * @ingroup Core
 * Add a handler to an active mio conn. The handler will be executed if a message matching the inputted namespace ns, type type or name name is received. Either a namespace, name or type must be passed as a parameter.
 * @param conn A pointer to an active mio conn.
 * @param handler A pointer to the mio handler to execute.
 * @param ns A string containing the XMPP namespace to listen for.
 * @param name A string containing the XMPP name to listen for.
 * @param type A string containing the XMPP type to listen for.
 * @param request A pointer to the mio request to be passed to the handler.
 * @param userdata A pointer to any user data, which should be passed to the handler.
 * @param response A pointer to an allocated mio response struct, which will be passed to the handler.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_handler_add(mio_conn_t * conn, mio_handler handler, const char *ns,
                    const char *name, const char *type, mio_request_t *request,
                    void *userdata, mio_response_t *response) {

    int err = MIO_OK;
    uuid_t uuid;
    mio_handler_data_t *handler_data = _mio_handler_data_new();

    if (ns == NULL && type == NULL && name == NULL) {
        mio_error("Cannot add handler without namespace, name or type");
        return MIO_ERROR_HANDLER_ADD;
    }

    handler_data->conn = conn;
    handler_data->handler = handler;
    handler_data->userdata = userdata;
    handler_data->response = response;

// Copy handler trigger data into response
    if (response != NULL) {
        if (ns != NULL) {
            response->ns = malloc(strlen(ns) + 1);
            strcpy(response->ns, ns);
        }
        if (type != NULL) {
            response->type = malloc(strlen(type) + 1);
            strcpy(response->type, type);
        }
        if (name != NULL) {
            response->type = malloc(strlen(name) + 1);
            strcpy(response->name, name);
        }
    }

    if (request != NULL) {
// Create id for request so we can hash it
        if (strcmp(request->id, "") == 0) {
            uuid_generate(uuid);
            uuid_unparse(uuid, request->id);
        }
        err = _mio_request_add(conn, request->id, handler, MIO_HANDLER,
                               handler_data->response, request);
    }

    if (err == MIO_OK) {
        // Lock the event loop mutex so that we don't interleave addition/deletion of handlers
        pthread_mutex_lock(&conn->event_loop_mutex);
        xmpp_handler_add(conn->xmpp_conn, mio_handler_generic, ns, name, type,
                         handler_data);
        pthread_mutex_unlock(&conn->event_loop_mutex);
    }

    else
        mio_error("Cannot add handler for id %s", request->id);

    return err;
}

/**
 * @ingroup Core
 * Add an ID handler to an active mio conn. The handler will be executed if a message matching the inputed XMPP message ID id, is received.
 * @param conn A pointer to an active mio conn.
 * @param handler A pointer to the mio handler to execute.
 * @param ns A string containing the XMPP mesage ID to listen for.
 * @param request A pointer to the mio request to be passed to the handler.
 * @param userdata A pointer to any user data, which should be passed to the handler.
 * @param response A pointer to an allocated mio response struct, which will be passed to the handler.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_handler_id_add(mio_conn_t * conn, mio_handler handler, const char *id,
                       mio_request_t *request, void *userdata, mio_response_t *response) {

    int err = MIO_OK;
    mio_handler_data_t *handler_data;
    handler_data = _mio_handler_data_new();

    strcpy(request->id, id);

    if (request != NULL)
        err = _mio_request_add(conn, id, handler, MIO_HANDLER_ID, response,
                               request);

    handler_data->conn = conn;
    handler_data->handler = handler;
    handler_data->userdata = userdata;
    handler_data->request = request;
    handler_data->response = response;

    strcpy(handler_data->request->id, id);
    strcpy(handler_data->response->id, id);

    if (err == MIO_OK) {
        // Lock the event loop mutex so that we don't interleve addition/deletion of handlers
        pthread_mutex_lock(&conn->event_loop_mutex);
        xmpp_id_handler_add(conn->xmpp_conn, mio_handler_generic_id, id,
                            (void*) handler_data);
        pthread_mutex_unlock(&conn->event_loop_mutex);
    } else
        mio_error("Cannot add id handler for id %s", id);

    return err;
}

/**
 * @ingroup Core
 * Add a timed handler to an active mio conn. The handler will be executed periodically every period seconds.
 * @param conn A pointer to an active mio conn.
 * @param handler A pointer to the mio handler to execute.
 * @param period The period in seconds in which the handler should be executed.
 * @param userdata A pointer to any user data, which should be passed to the handler.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_handler_timed_add(mio_conn_t * conn, mio_handler handler, int period,
                          void *userdata) {
    mio_handler_data_t *handler_data = _mio_handler_data_new();
    handler_data->handler = handler;
    handler_data->conn = conn;
    handler_data->userdata = userdata;
// Lock the event loop mutex so that we don't interleve addition/deletion of handlers
    pthread_mutex_lock(&conn->event_loop_mutex);
    xmpp_timed_handler_add(conn->xmpp_conn,
                           (xmpp_timed_handler) mio_handler_generic_timed, period,
                           handler_data);
    pthread_mutex_unlock(&conn->event_loop_mutex);
    return MIO_OK;
}

/**
 * @ingroup Core
 * Remove an existing mio handler by ID.
 *
 * @param conn A pointer to the active mio conn containing the handler.
 * @param handler A pointer to the mio handler to delete.
 * @param id A string containing the ID of the handler to delete.
 *
 * @returns MIO_OK on success, otherwise an error.
 */
void mio_handler_id_delete(mio_conn_t * conn, mio_handler handler,
                           const char *id) {
// Lock the event loop mutex so that we don't interleve addition/deletion of handlers
    pthread_mutex_lock(&conn->event_loop_mutex);
    xmpp_id_handler_delete(conn->xmpp_conn, mio_handler_generic_id, id);
    pthread_mutex_unlock(&conn->event_loop_mutex);
}

mio_xml_parser_data_t *mio_xml_parser_data_new() {
    mio_xml_parser_data_t *xml_data = malloc(sizeof(mio_xml_parser_data_t));
    memset(xml_data, 0, sizeof(mio_xml_parser_data_t));
    return xml_data;
}

/**
 * @ingroup Stanza
 * Internal function to reset the mio parser.
 *
 * @param parser A pointer to the parser to be reset.
 * @returns MIO_OK on success, MIO_ERROR_PARSER otherwise.
 */
int mio_parser_reset(mio_parser_t *parser) {
    if (parser->expat)
        XML_ParserFree(parser->expat);

    if (parser->stanza)
        xmpp_stanza_release(parser->stanza);

    parser->expat = XML_ParserCreate(NULL);
    if (!parser->expat)
        return MIO_ERROR_PARSER;

    parser->depth = 0;
    parser->stanza = NULL;

    XML_SetUserData(parser->expat, parser);
    XML_SetElementHandler(parser->expat, mio_parser_start_element,
                          mio_parser_end_element);
    XML_SetCharacterDataHandler(parser->expat, mio_parser_characters);

    return MIO_OK;
}

/**
 * @ingroup Stanza
 * Internal function called by expat to parse each element of an XML string
 *
 * @param userdata A pointer to user specified data passed into expat. Unused.
 * @param name The name of the current element.
 * @param attrs A 2D array of the attributes associated with the current element.
 */
void mio_parser_start_element(void *userdata, const XML_Char *name,
                              const XML_Char **attrs) {
    mio_parser_t *parser = (mio_parser_t *) userdata;
    xmpp_stanza_t *child;

    if (!parser->stanza) {
        /* starting a new toplevel stanza */
        parser->stanza = xmpp_stanza_new(parser->ctx);
        if (!parser->stanza) {
            /* FIXME: can't allocate, disconnect */
        }
        xmpp_stanza_set_name(parser->stanza, name);
        mio_parser_set_attributes(parser->stanza, attrs);
    } else {
        /* starting a child of parser->stanza */
        child = xmpp_stanza_new(parser->ctx);
        if (!child) {
            /* FIXME: can't allocate, disconnect */
        }
        xmpp_stanza_set_name(child, name);
        mio_parser_set_attributes(child, attrs);

        /* add child to parent */
        xmpp_stanza_add_child(parser->stanza, child);

        /* the child is owned by the toplevel stanza now */
        xmpp_stanza_release(child);

        /* make child the current stanza */
        parser->stanza = child;
    }
    parser->depth++;
}

/**
 * @ingroup Stanza
 * Internal function called by expat when the end of an element being parsed was reached
 *
 * @param userdata A pointer to user specified data passed into expat. Unused.
 * @param name The name of the current element.
 */
void mio_parser_end_element(void *userdata, const XML_Char *name) {
    mio_parser_t *parser = (mio_parser_t *) userdata;

    parser->depth--;

    if (parser->depth != 0 && parser->stanza->parent) {
        /* we're finishing a child stanza, so set current to the parent */
        parser->stanza = parser->stanza->parent;
    }
}

/**
 * @ingroup Stanza
 * Internal function called by expat to parse an XML string into an xmpp stanza.
 *
 * @param userdata A pointer to user specified data passed into expat. Unused.
 * @param s Pointer to the string currently being being parsed.
 * @param len The length of the string currently being parsed
 */
void mio_parser_characters(void *userdata, const XML_Char *s, int len) {
    mio_parser_t *parser = (mio_parser_t *) userdata;
    xmpp_stanza_t *stanza;

    if (parser->depth < 2)
        return;

    /* create and populate stanza */
    stanza = xmpp_stanza_new(parser->ctx);
    if (!stanza) {
        /* FIXME: allocation error, disconnect */
        return;
    }
    xmpp_stanza_set_text_with_size(stanza, s, len);

    xmpp_stanza_add_child(parser->stanza, stanza);
    xmpp_stanza_release(stanza);
}

void mio_handler_conn_generic(xmpp_conn_t * const conn,
                              const xmpp_conn_event_t status, const int error,
                              xmpp_stream_error_t * const stream_error, void * const mio_handler_data) {
 
    mio_request_t *request, *request_tmp;
    mio_handler_data_t *shd = (mio_handler_data_t *) mio_handler_data;
    mio_conn_t *mio_conn = shd->conn;
    if (shd->response == NULL)
        shd->response = mio_response_new();
    mio_debug("In conn_handler");

    if (status == XMPP_CONN_CONNECT) {
        shd->response->response_type = MIO_RESPONSE_OK;
        mio_debug("In conn_handler : Connected");
        if (shd->conn_handler != NULL)
            shd->conn_handler(shd->conn, MIO_CONN_CONNECT, shd->response,
                              shd->userdata);

	mio_conn->has_connected = 1;
        // If a reconnect happened, clean up all waiting threads
        if (mio_conn->retries > 0) {
            mio_conn->retries = 0;

            // If we were listening before the reconnect, start listening again and wake listing thread if there is anything in the RX queue
            if (mio_conn->pubsub_rx_listening)
                mio_listen_start(mio_conn);
            if (mio_conn->pubsub_rx_queue_len > 0) {
                request = _mio_request_get(mio_conn, "pubsub_data_rx");
                if (request != NULL)
                    mio_cond_signal(&request->cond, &request->mutex,
                                    &request->predicate);
            }

            // Trigger all remaining requests
            HASH_ITER(hh, mio_conn->mio_request_table, request, request_tmp)
            {
                if (strcmp(request->id, "pubsub_data_rx") == 0)
                    continue;
                mio_cond_signal(&request->cond, &request->mutex,
                                &request->predicate);
                HASH_DEL(mio_conn->mio_request_table, request);
                _mio_request_free(request);
            }
        }

        mio_cond_broadcast(&shd->conn->conn_cond, &shd->conn->conn_mutex,
                           &shd->conn->conn_predicate);

        // If the connection fails, try to reconnect up to MIO_CONNECTION_RETRIES times
    } else if ( status == XMPP_CONN_FAIL ) {
        mio_response_error_t *err;
        mio_error(
            "XMPP connection failed. Check jid's domain domain.");
        err = _mio_response_error_new();
        err->err_num = MIO_ERROR_CONNECTION;
        err->description = strdup("MIO_ERROR_CONNECTION");
        shd->response->response = err;
        shd->response->response_type = MIO_RESPONSE_ERROR;
        mio_cond_broadcast(&shd->conn->conn_cond, &shd->conn->conn_mutex,
                           &shd->conn->conn_predicate);
    } else if (!mio_conn->has_connected) { 
	mio_response_error_t *err;
        mio_error(
            "XMPP connection failed. Check jid's domain domain.");
        err = _mio_response_error_new();
        err->err_num = MIO_ERROR_CONNECTION;
        err->description = strdup("MIO_ERROR_CONNECTION");
        shd->response->response = err;
        shd->response->response_type = MIO_RESPONSE_ERROR;
        mio_cond_broadcast(&shd->conn->conn_cond, &shd->conn->conn_mutex,
                           &shd->conn->conn_predicate);
    }
#ifdef MIO_CONNECTION_RETRIES
    else if (mio_conn->retries < MIO_CONNECTION_RETRIES) {
#else
    else {
#endif
        mio_warn(
            "Disconnected from server, attempting to reconnect, attempt: %u",
            mio_conn->retries);
        if (mio_conn->retries > 0)
            usleep(1000000*MIO_RECONNECTION_TIMEOUT_S);

        mio_reconnect(mio_conn);
        //mio_disconnect(mio_conn);

        //mio_connect(mio_conn->xmpp_conn->jid, mio_conn->xmpp_conn->pass, NULL,
        //  NULL, mio_conn);
        //xmpp_connect_client(conn, NULL, 0, mio_handler_conn_generic, shd);
        // If the connection still fails, return an error
    }
#ifdef MIO_CONNECTION_RETRIES
    else {
        mio_response_error_t *err;
        mio_error(
            "Disconnected from server, all retries failed. Check connection.");
        err = _mio_response_error_new();
        err->err_num = MIO_ERROR_CONNECTION;
        err->description = strdup("MIO_ERROR_CONNECTION");
        shd->response->response = err;
        shd->response->response_type = MIO_RESPONSE_ERROR;
        mio_cond_broadcast(&shd->conn->conn_cond, &shd->conn->conn_mutex,
                           &shd->conn->conn_predicate);
    }
#endif
}

int mio_handler_generic(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
                        void * const mio_handler_data) {

    int err;
    mio_handler_data_t *shd = (mio_handler_data_t*) mio_handler_data;
    mio_stanza_t *s = malloc(sizeof(mio_stanza_t));
    s->xmpp_stanza = stanza;
    err = shd->handler(shd->conn, s, shd->response, shd->userdata);
    //mio_stanza_free(s);
    // Returning 0 removes the handler
    if (err == MIO_HANDLER_KEEP) {
        return 1;
    } else if (err == MIO_HANDLER_REMOVE) {
        mio_handler_data_free(shd);
        return 0;
    }
    return 1;
}

int mio_handler_generic_id(xmpp_conn_t * const conn,
                           xmpp_stanza_t * const stanza, void * const mio_handler_data) {

    int err;
    mio_handler_data_t *shd = (mio_handler_data_t*) mio_handler_data;
    mio_stanza_t *s = malloc(sizeof(mio_stanza_t));
    s->xmpp_stanza = stanza;

    err = shd->handler(shd->conn, s, shd->response, shd->userdata);
    //mio_stanza_free(s);

    // Returning 0 removes the handler
    if (err != MIO_OK)
        return 1;
    else {
        mio_handler_data_free(shd);
        return 0;
    }
}

int mio_handler_generic_timed(xmpp_conn_t * const conn,
                              void * const mio_handler_data) {

    int ret;

    mio_handler_data_t *shd = (mio_handler_data_t*) mio_handler_data;
    ret = shd->handler(shd->conn, NULL, shd->response, shd->userdata);
    if (ret == MIO_HANDLER_KEEP)
        return 1;
    else {
        mio_handler_data_free(shd);
        return 0;
    }
}

int mio_handler_keepalive(mio_conn_t * const conn, mio_stanza_t * const stanza,
                          mio_response_t *response, void *userdata) {
    // Send single space every KEEPALIVE_PERIOD ms to keep connection alive
    xmpp_send_raw(conn->xmpp_conn, " ", 1);
    return MIO_HANDLER_KEEP;
}

// XMLParser func called whenever an end of element is encountered
void XMLCALL endElement(void *data, const char *element_name) {
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    if (xml_data != NULL)
        _mio_xml_data_update_end_element(xml_data, element_name);
}

void _mio_xml_data_update_start_element(mio_xml_parser_data_t *xml_data,
                                        const char* element_name) {
    mio_xml_parser_parent_data_t *parent_data;

    xml_data->prev_element_name = xml_data->curr_element_name;
    xml_data->curr_element_name = element_name;
    xml_data->curr_depth++;
    if (xml_data->prev_depth < xml_data->curr_depth) {
        if (xml_data->parent == NULL) {
            xml_data->parent = mio_xml_parser_parent_data_new();
            parent_data = xml_data->parent;
        } else {
            if (xml_data->parent->next != NULL)
                mio_xml_parser_parent_data_free(xml_data->parent->next);
            xml_data->parent->next = mio_xml_parser_parent_data_new();
            parent_data = xml_data->parent->next;
            parent_data->prev = xml_data->parent;
        }
        parent_data->parent = xml_data->prev_element_name;
        xml_data->parent = parent_data;
    }
    xml_data->prev_depth = xml_data->curr_depth;
}

int mio_xml_parse(mio_conn_t *conn, mio_stanza_t * const stanza,
                  mio_xml_parser_data_t *xml_data, XML_StartElementHandler start,
                  XML_CharacterDataHandler char_handler) {

    XML_Parser p;
    char *buf;
    size_t buflen;

    if (stanza == NULL) {
        mio_error("XML Parser received NULL stanza");
        return MIO_ERROR_XML_NULL_STANZA;
    }

//Convert received stanza to text for XML parser
    xmpp_stanza_to_text(stanza->xmpp_stanza, &buf, &buflen);

//Creates an instance of the XML Parser to parse the event packet
    p = XML_ParserCreate(NULL);
    if (!p) {
        mio_error("Could not allocate XML parser");
        return MIO_ERROR_XML_PARSER_ALLOCATION;
    }
//Sets the handlers to call when parsing the start and end of an XML element
    XML_SetElementHandler(p, start, endElement);
    if (char_handler != NULL)
        XML_SetCharacterDataHandler(p, char_handler);
    XML_SetUserData(p, xml_data);
    xml_data->parser = &p;

    if (XML_Parse(p, buf, strlen(buf), 1) == XML_STATUS_ERROR) {
        mio_error("XML parse error at line %u:\n%s\n",
                  (int ) XML_GetCurrentLineNumber(p),
                  XML_ErrorString(XML_GetErrorCode(p)));
        return MIO_ERROR_XML_PARSING;
    }

    XML_ParserFree(p);
    xmpp_free(conn->xmpp_conn->ctx, buf);
    free(xml_data);

    return MIO_OK;
}

int mio_handler_item_recent_get(mio_conn_t * const conn,
                                mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
    mio_stanza_t *stanza_copy;
    mio_request_t *request = _mio_request_get(conn, response->id);
    if (request == NULL) {
        mio_error("Request with id %s not found, aborting handler",
                  response->id);
        return MIO_ERROR_REQUEST_NOT_FOUND;
    }
    mio_packet_t *packet = mio_packet_new();
    mio_data_t *data = mio_data_new();
    mio_packet_payload_add(packet, (void*) data, MIO_PACKET_DATA);
    mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
    xml_data->response = response;
    stanza_copy = mio_stanza_clone(conn, stanza);
    response->stanza = stanza_copy;

    response->response = packet;
    int err = mio_xml_parse(conn, stanza, xml_data,
                            mio_XMLstart_pubsub_data_receive, NULL);

    if (err == MIO_OK) {
        response->response_type = MIO_RESPONSE_PACKET;
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
    }

    return err;
}

/**
 * @ingroup Stanza
 * Allocate a new mio parser. Resets the parser on creation.
 *
 * @param conn An active mio connection struct which will be used to allocate the stanzas being produced.
 * @returns A new mio parser.
 */
mio_parser_t *mio_parser_new(mio_conn_t *conn) {
    mio_parser_t *parser;

    parser = malloc(sizeof(mio_parser_t));
    if (parser != NULL) {
        memset(parser, 0, sizeof(mio_parser_t));
        parser->ctx = conn->xmpp_conn->ctx;
        parser->userdata = conn->xmpp_conn;
        mio_parser_reset(parser);
    }
    return parser;
}

/**
 * @ingroup Stanza
 * Frees a mio parser.
 *
 * @param parser mio parser to be freed.
 */
void mio_parser_free(mio_parser_t *parser) {
    if (parser->expat)
        XML_ParserFree(parser->expat);
    free(parser);
}

/**
 * @ingroup Stanza
 * Internal function to set attributes of an xmpp stanza struct.
 *
 * @param stanza An XMPP stanza struct to add attrubutes to
 * @param attrs A 2D array of attributes
 */
void mio_parser_set_attributes(xmpp_stanza_t *stanza, const XML_Char **attrs) {
    int i;

    if (!attrs)
        return;

    for (i = 0; attrs[i]; i += 2) {
        xmpp_stanza_set_attribute(stanza, attrs[i], attrs[i + 1]);
    }
}

/**
 * @ingroup Stanza
 * Parses an XML string into a mio_stanza_t struct.
 *
 * @param parser mio parser to do the parsing.
 * @param string XML string to be parsed.
 * @returns The parsed XML string as a mio stanza.
 */
mio_stanza_t *mio_parse(mio_parser_t *parser, char *string) {
    mio_stanza_t *stanza = malloc(sizeof(mio_stanza_t));
    XML_Parse(parser->expat, string, strlen(string), 0);
    stanza->xmpp_stanza = parser->stanza;
    parser->stanza = NULL;
    parser->depth = 0;
    return stanza;
}

