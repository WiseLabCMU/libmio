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

#ifndef MIO_PACKET_H
#define MIO_PACKET_H


typedef enum {
    MIO_PACKET_UNKNOWN,
    MIO_PACKET_DATA,
    MIO_PACKET_META,
    MIO_PACKET_SUBSCRIPTIONS,
    MIO_PACKET_AFFILIATIONS,
    MIO_PACKET_COLLECTIONS,
    MIO_PACKET_NODE_TYPE,
    MIO_PACKET_SCHEDULE,
    MIO_PACKET_REFERENCES
} mio_packet_type_t;

typedef struct mio_packet {
    char *id;
    mio_packet_type_t type;
    int num_payloads;
    void *payload;
} mio_packet_t;

mio_packet_t *mio_packet_new();
void mio_packet_free(mio_packet_t* packet);

void mio_packet_payload_add(mio_packet_t *packet, void *payload,
                            mio_packet_type_t type);
#endif