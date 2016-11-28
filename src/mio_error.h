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

#ifndef _mio_error_h
#define _mio_error_h

//#include <mio_connection.h>
//#include <mio_handlers.h>

#include <mio.h>

// Handler return codes
#define MIO_HANDLER_KEEP 2
#define MIO_HANDLER_REMOVE 3

// Error codes
#define MIO_OK                  1       // Success
#define MIO_ERROR_RUN_THREAD    -1      // Error starting _mio_run_thread
#define MIO_ERROR_CONNECTION    -2      // Connection error
#define MIO_ERROR_DISCONNECTED  -3      // Error because connection not active
#define MIO_ERROR_REQUEST_NOT_FOUND -4
#define MIO_ERROR_MUTEX         -5
#define MIO_ERROR_COND_WAIT -6
#define MIO_ERROR_TOO_MANY_OPEN_REQUESTS -7
#define MIO_ERROR_PREDICATE     -8
#define MIO_ERROR_NULL_STANZA_ID    -9
#define MIO_ERROR_XML_NULL_STANZA -10
#define MIO_ERROR_XML_PARSER_ALLOCATION -11
#define MIO_ERROR_XML_PARSING   -12
#define MIO_ERROR_HANDLER_ADD   -13
#define MIO_ERROR_UNKNOWN_PACKET_TYPE -14
#define MIO_ERROR_UNKNOWN_AFFILIATION_TYPE -15
#define MIO_ERROR_UNKNOWN_RESPONSE_TYPE -16
#define MIO_ERROR_TIMEOUT               -17
#define MIO_ERROR_NULL_STANZA           -18
#define MIO_ERROR_ALREADY_SUBSCRIBED    -19
#define MIO_ERROR_RW_LOCK           -20
#define MIO_ERROR_INVALID_JID -21
#define MIO_ERRROR_EVENT_LOOP_NOT_STARTED -22
#define MIO_ERROR_NO_RESPONSE -23
#define MIO_ERROR_DUPLICATE_ENTRY -24
#define MIO_ERROR_NOT_AFFILIATED -25
#define MIO_ERROR_UNEXPECTED_RESPONSE -26
#define MIO_ERROR_UNEXPECTED_PAYLOAD -27
#define MIO_ERROR_INCOMPLETE_ENUMERATION -28
#define MIO_ERROR_REFERENCE_LOOP -29
#define MIO_ERROR_UNKNOWN_META_TYPE -30;
#define MIO_ERROR_PARSER -31;
#define MIO_ERRROR_TRANSDUCER_NULL_NAME -32
#define MIO_ERROR_TRANSDUCER_NULL_VALUE -33

int mio_handler_error(mio_conn_t * const conn, mio_stanza_t * const stanza,
                      mio_response_t *response, void *userdata);
void mio_XML_error_handler(const char *element_name, const char **attr,
                           mio_response_t *response);
#endif
