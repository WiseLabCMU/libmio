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

#include <mio_packet.h>
#include <mio_reference.h>
#include <mio_meta.h>
#include <mio_connection.h>
#include <mio_pubsub.h>
#include <mio_affiliations.h>
#include <mio_user.h>
#include <mio_collection.h>
#include <mio_schedule.h>
#include <mio_error.h>
extern mio_log_level_t _mio_log_level;

/**
 * @ingroup PubSub
 * Frees an allocated mio packet.
 *
 * @param packet A pointer to the allocated mio packet to be freed.
 */
void mio_packet_free(mio_packet_t* packet) {
    mio_data_t *data;
    mio_subscription_t *sub, *curr_sub;
    mio_affiliation_t *aff, *curr_aff;
    mio_collection_t *coll, *curr_coll;
    mio_event_t *event, *curr_event;
    mio_reference_t *ref, *curr_ref;
    mio_meta_t *meta;

    if (packet->payload != NULL ) {
        switch (packet->type) {
        case MIO_PACKET_DATA:
            data = packet->payload;
            mio_data_free(data);
            break;

        case MIO_PACKET_SUBSCRIPTIONS:
            curr_sub = (mio_subscription_t*) packet->payload;
            while (curr_sub != NULL ) {
                sub = curr_sub->next;
                mio_subscription_free(curr_sub);
                curr_sub = sub;
            }
            break;

        case MIO_PACKET_AFFILIATIONS:
            curr_aff = (mio_affiliation_t*) packet->payload;
            while (curr_aff != NULL ) {
                aff = curr_aff->next;
                mio_affiliation_free(curr_aff);
                curr_aff = aff;
            }
            break;

        case MIO_PACKET_COLLECTIONS:
            curr_coll = (mio_collection_t*) packet->payload;
            while (curr_coll != NULL ) {
                coll = curr_coll->next;
                mio_collection_free(curr_coll);
                curr_coll = coll;
            }
            break;
        case MIO_PACKET_META:
            meta = packet->payload;
            mio_meta_free(meta);
            break;

        case MIO_PACKET_NODE_TYPE:
            free(packet->payload);
            break;

        case MIO_PACKET_SCHEDULE:
            curr_event = (mio_event_t*) packet->payload;
            while (curr_event != NULL ) {
                event = curr_event->next;
                mio_event_free(curr_event);
                curr_event = event;
            }
            break;

        case MIO_PACKET_REFERENCES:
            curr_ref = (mio_reference_t*) packet->payload;
            while (curr_ref != NULL ) {
                ref = curr_ref->next;
                mio_reference_free(curr_ref);
                curr_ref = ref;
            }
            break;

        default:
            mio_error("Cannot free packet of unknown type");
            return;
            break;
        }
    }

    if (packet->id != NULL )
        free(packet->id);

    free(packet);
}


/**
 * @ingroup PubSub
 *
 * Allocates and initializes a new mio packet.
 *
 * @returns The newly allocated and initialized mio packet.
 */
mio_packet_t *mio_packet_new() {
    mio_packet_t *packet = malloc(sizeof(mio_packet_t));
    memset(packet,0,sizeof(mio_packet_t));
    packet->type = MIO_PACKET_UNKNOWN;
    return packet;
}
