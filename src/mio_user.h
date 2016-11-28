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

#ifndef MIO_USER_H
#define MIO_USER_H
#include <mio.h>
//#include <mio_connection.h>
// Connection related functions.
int mio_connect(char *jid, char *pass, mio_handler_conn conn_handler,
                void *conn_handler_user_data, mio_conn_t *conn);
int mio_disconnect(mio_conn_t *conn);
int mio_password_change(mio_conn_t * conn, const char *new_pass,
                        mio_response_t * response);
int mio_reconnect(mio_conn_t *conn);
#endif
