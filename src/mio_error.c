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

#include "mio_error.h"
#include <mio_connection.h>

extern mio_log_level_t _mio_log_level;

void XMLCALL mio_XMLstart_error(void *data, const char *element_name,
                                const char **attr);

void XMLCALL mio_XMLstart_error(void *data, const char *element_name,
                                const char **attr) {

    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);

    if (response->response_type != MIO_RESPONSE_ERROR)
        response->response_type = MIO_RESPONSE_OK;
}

void mio_XML_error_handler(const char *element_name, const char **attr,
                           mio_response_t *response) {

    int i;
    mio_response_error_t *err = _mio_response_error_new();

    for (i = 0; attr[i]; i += 2) {
        const char* attr_name = attr[i];
        if (strcmp(attr_name, "type") == 0) {
            err->description = malloc(strlen(attr[i + 1]) + 1);
            strcpy(err->description, attr[i + 1]);
            response->response_type = MIO_RESPONSE_ERROR;
            response->response = err;
        } else if (strcmp(attr_name, "code") == 0) {
            err->err_num = atoi(attr[i + 1]);
        }
    }
}

int mio_handler_error(mio_conn_t * const conn, mio_stanza_t * const stanza,
                      mio_response_t *response, void *userdata) {
    mio_stanza_t *stanza_copy;
    mio_request_t *request = _mio_request_get(conn, response->id);
    if (request == NULL ) {
        mio_error("Request with id %s not found, aborting handler",
                  response->id);
        return MIO_ERROR_REQUEST_NOT_FOUND;
    }
    mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
    xml_data->response = response;
    stanza_copy = mio_stanza_clone(conn, stanza);
    response->stanza = stanza_copy;

    int err = mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_error, NULL );

    if (err == MIO_OK)
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

    return err;
}



