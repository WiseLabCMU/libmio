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


#ifndef ____mio_affiliations__
#define ____mio_affiliations__

#include <stdio.h>
/*#include <mio_connection.h>
#include <mio_packet.h>
#include <mio_handlers.h>*/
#include <mio.h>

typedef enum {
    MIO_AFFILIATION_NONE,
    MIO_AFFILIATION_OWNER,
    MIO_AFFILIATION_MEMBER,
    MIO_AFFILIATION_PUBLISHER,
    MIO_AFFILIATION_PUBLISH_ONLY,
    MIO_AFFILIATION_OUTCAST,
    MIO_AFFILIATION_UNKNOWN
} mio_affiliation_type_t;

typedef struct mio_subscription mio_subscription_t;

struct mio_subscription {
    char *subscription;
    char *sub_id;
    mio_subscription_t *next;
};

typedef struct mio_affiliation mio_affiliation_t;

struct mio_affiliation {
    char *affiliation;
    mio_affiliation_type_t type;
    mio_affiliation_t *next;
};

// Subscription struct utilities
mio_subscription_t *mio_subscription_new();
void mio_subscription_free(mio_subscription_t *sub);
mio_subscription_t *mio_subscription_tail_get(mio_subscription_t *sub);

// affiliation utilities
mio_affiliation_t *mio_affiliation_new();
void mio_affiliation_free(mio_affiliation_t *affiliation);
mio_affiliation_t *mio_affiliation_tail_get(mio_affiliation_t *affiliation);
void mio_affiliation_add(mio_packet_t *pkt, char *affiliation,
                         mio_affiliation_type_t type);

int mio_subscribe(mio_conn_t * conn, const char *node,
                  mio_response_t * response);
// XMPP functions
void mio_subscription_add(mio_packet_t *pkt, char *subscription, char *sub_id);
int mio_subscriptions_query(mio_conn_t *conn, const char *node,
                            mio_response_t * response);
int mio_acl_affiliation_set(mio_conn_t *conn, const char *node, const char *jid,
                            mio_affiliation_type_t type, mio_response_t * response);
int mio_acl_affiliations_query(mio_conn_t *conn, const char *node,
                               mio_response_t * response);
int mio_unsubscribe(mio_conn_t * conn, const char *node, const char *sub_id,
                    mio_response_t * response);
#endif /* defined(____mio_affiliations__) */
