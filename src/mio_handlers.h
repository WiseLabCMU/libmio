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

#ifndef HANDLERS_H_
#define HANDLERS_H_

#define NAME_MAX_CHARS 128

#include <expat.h>
#include <uuid/uuid.h>
#include <mio.h>

//// Handler return codes
#define MIO_HANDLER_KEEP 2
#define MIO_HANDLER_REMOVE 3


typedef struct mio_xml_parser_parent_data mio_xml_parser_parent_data_t;

typedef struct mio_parser {
    xmpp_ctx_t *ctx;
    XML_Parser expat;
    void *userdata;
    int depth;
    xmpp_stanza_t *stanza;
} mio_parser_t;

struct mio_xml_parser_parent_data {
    const char *parent;
    mio_xml_parser_parent_data_t* next;
    mio_xml_parser_parent_data_t* prev;
};

typedef struct mio_xml_parser_data {
    mio_response_t *response;
    void* payload;
    XML_Parser *parser;
    const char* curr_element_name;
    const char* prev_element_name;
    const char* curr_attr_name;
    int curr_depth;
    int prev_depth;
    mio_xml_parser_parent_data_t *parent;
} mio_xml_parser_data_t;

typedef struct mio_handler_data {
    mio_request_t *request;
    mio_response_t *response;
    mio_handler handler;
    mio_handler_conn conn_handler;
    mio_conn_t *conn;
    void *userdata;
} mio_handler_data_t;
// MIO HANDLER

// parser structure functions
mio_parser_t *mio_parser_new(mio_conn_t *conn);
void mio_parser_free(mio_parser_t *parser);
mio_xml_parser_data_t *mio_xml_parser_data_new();
mio_handler_data_t *_mio_handler_data_new();


int mio_handler_add(mio_conn_t * conn, mio_handler handler, const char *ns,
                    const char *name, const char *type, mio_request_t *request,
                    void *userdata, mio_response_t *response);
int mio_handler_id_add(mio_conn_t * conn, mio_handler handler, const char *id,
                       mio_request_t *request, void *userdata, mio_response_t *response);
int mio_handler_timed_add(mio_conn_t * conn, mio_handler handler, int period,
                          void *userdata);
void mio_handler_id_delete(mio_conn_t * conn, mio_handler handler,
                           const char *id);
void mio_handler_data_free(mio_handler_data_t *shd);
void mio_callback_set_publish_success(void *f);

void mio_handler_conn_generic(xmpp_conn_t * const, const xmpp_conn_event_t,
                              const int, xmpp_stream_error_t * const, void * const);
int mio_handler_generic(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
                        void * const mio_handler_data);

int mio_handler_generic_id(xmpp_conn_t * const conn,
                           xmpp_stanza_t * const stanza, void * const mio_handler_data);
int mio_handler_generic_timed(xmpp_conn_t * const conn,
                              void * const mio_handler_data);

int mio_handler_keepalive(mio_conn_t * const conn,
                          mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
size_t mio_handler_admin_user_functions(void*, size_t, size_t, void*);
int mio_handler_check_jid_registered(mio_conn_t* const, mio_stanza_t* const);
int mio_handler_item_recent_get(mio_conn_t * const conn,
                                mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
int mio_conn_handler_presence_send(mio_conn_t * const conn,
                                   mio_conn_event_t event, mio_response_t *response, void *userdata);

// parser functions
mio_xml_parser_parent_data_t *mio_xml_parser_parent_data_new();
void mio_xml_parser_parent_data_free(mio_xml_parser_parent_data_t *data);

void _mio_xml_data_update_start_element(mio_xml_parser_data_t *xml_data,
                                        const char* element_name);
void _mio_xml_data_update_end_element(mio_xml_parser_data_t *xml_data,
                                      const char* element_name);
mio_xml_parser_data_t *mio_xml_parser_data_new();

int _mio_xml_parse(mio_conn_t *conn, mio_stanza_t * const stanza,
                   mio_xml_parser_data_t *xml_data, XML_StartElementHandler start,
                   XML_CharacterDataHandler char_handler);
void mio_parser_start_element(void *userdata, const XML_Char *name,
                              const XML_Char **attrs);
void mio_parser_end_element(void *userdata, const XML_Char *name);
void mio_parser_characters(void *userdata, const XML_Char *s, int len);

mio_stanza_t *mio_parse(mio_parser_t *parser, char *string);
void mio_parser_set_attributes(xmpp_stanza_t *stanza,
                               const XML_Char **attrs);
int mio_xml_parse(mio_conn_t *conn, mio_stanza_t * const stanza,
                  mio_xml_parser_data_t *xml_data, XML_StartElementHandler start,
                  XML_CharacterDataHandler char_handler);
void XMLCALL endElement(void *data, const char *element_name);
#endif /* HANDLERS_H_ */
