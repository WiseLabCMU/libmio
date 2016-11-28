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


#ifndef ____mio_geolocation__
#define ____mio_geolocation__

#include <stdio.h>
#include <mio.h>

mio_geoloc_t *mio_geoloc_new();
void mio_geoloc_free(mio_geoloc_t *geoloc);
void mio_geoloc_merge(mio_geoloc_t *geoloc_to_update, mio_geoloc_t *geoloc);
void mio_meta_geoloc_remove(mio_meta_t *meta,
                            mio_transducer_meta_t *transducers, char *transducer_name);
int mio_meta_geoloc_remove_publish(mio_conn_t *conn, char *node,
                                   char *transducer_name, mio_response_t *response);
void mio_geoloc_print(mio_geoloc_t *geoloc, char* tabs);
mio_stanza_t *mio_geoloc_to_stanza(mio_conn_t* conn, mio_geoloc_t *geoloc);
void XMLCALL mio_XMLString_geoloc(void *data, const XML_Char *s, int len);
#endif /* defined(____mio_geolocation__) */
