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
#include "mio_affiliations.h"
#include "mio_connection.h"
#include "mio_error.h"
#include "mio_handlers.h"
#include "mio_node.h"
#include <mio_user.h>
#include <mio_pubsub.h>

extern mio_log_level_t _mio_log_level;
void XMLCALL mio_XMLstart_acl_affiliations_query(void *data,
        const char *element_name, const char **attr) ;

static void XMLCALL mio_XMLstart_subscriptions_query(void *data,
        const char *element_name, const char **attr);
static void XMLCALL mio_XMLstart_subscribe(void *data, const char *element_name,
        const char **attr);
int mio_handler_subscribe(mio_conn_t * const conn, mio_stanza_t * const stanza,
                          mio_response_t *response, void *userdata);
int mio_handler_publisher_add(mio_conn_t* const, mio_stanza_t* const,
                              mio_data_t* const);

int mio_handler_publisher_remove(mio_conn_t* const, mio_stanza_t* const,
                                 mio_data_t* const);

int mio_handler_unsubscribe(mio_conn_t* const, mio_stanza_t* const,
                            mio_data_t* const);

int mio_handler_acl_node(mio_conn_t* const, mio_stanza_t* const,
                         mio_data_t* const);
int mio_handler_subscriptions_query(mio_conn_t * const conn,
                                    mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
int mio_handler_subscriptions_query_node_subscribe(mio_conn_t * const conn,
        mio_stanza_t * const stanza, mio_data_t * const userdata);

int mio_handler_acl_affiliations_query(mio_conn_t * const conn,
                                       mio_stanza_t * const stanza, mio_response_t *response, void *userdata);




/**
 * @ingroup PubSub
 * Internal function to allocate and initialize a new mio subscription struct.
 *
 * @returns A pointer to a newly allocated and initialized mio subscription struct.
 */
mio_subscription_t *mio_subscription_new() {
    mio_subscription_t *sub = malloc(sizeof(mio_subscription_t));
    memset(sub,0,sizeof(mio_subscription_t));
    return sub;
}

/**
 * @ingroup PubSub
 * Frees an allocated mio subscription struct.
 *
 * @param data A pointer to the allocated mio data struct to be freed.
 */
void mio_subscription_free(mio_subscription_t *subscription) {
    if (subscription->sub_id) free(subscription->sub_id);
    if (subscription->subscription) free(subscription->subscription);
    free(subscription);
}

/**
 * @ingroup PubSub
 * Returns the tail of a mio subscription linked list
 *
 * @param subscription A pointer to an element in a mio subscription linked list.
 * @returns A pointer to the tail of a mio subscription linked list.
 */
mio_subscription_t *mio_subscription_tail_get(mio_subscription_t *subscription) {
    mio_subscription_t *tail = NULL;

    while (subscription != NULL ) {
        tail = subscription;
        subscription = subscription->next;
    }
    return tail;
}

/**
 * @ingroup PubSub
 * Create a mio subscription sctruct and add it to a mio packet.
 *
 * @param pkt A pointer the mio packet to which the subscription should be added to.
 * @param subscription A string containing the JID of the node that is subscribed.
 * @param sub_id A string containing the subscription ID of subscription.
 */
void mio_subscription_add(mio_packet_t *pkt, char *subscription, char *sub_id) {

    mio_subscription_t *sub;
    sub = mio_subscription_tail_get((mio_subscription_t*) pkt->payload);

    if (sub == NULL ) {
        sub = mio_subscription_new();
        mio_packet_payload_add(pkt, sub, MIO_PACKET_SUBSCRIPTIONS);
    } else {
        sub->next = mio_subscription_new();
        sub = sub->next;
    }

    sub->subscription = malloc((strlen(subscription) + 1) * sizeof(char));
    strcpy(sub->subscription, subscription);

// If no subscription ID is specified, fill it with a blank.
    if (sub_id != NULL ) {
        sub->sub_id = malloc((strlen(sub_id) + 1) * sizeof(char));
        strcpy(sub->sub_id, sub_id);
    } else {
        sub->sub_id = malloc(sizeof(char));
        strcpy(sub->sub_id, "");
    }

    pkt->num_payloads++;
}

/**
 * @ingroup PubSub
 * Internal function to allocate and initialize a new mio affiliation struct.
 *
 * @returns A pointer to a newly allocated and initialized mio affiliation struct.
 */
mio_affiliation_t *mio_affiliation_new() {
    mio_affiliation_t *aff = malloc(sizeof(mio_affiliation_t));
    memset(aff,0,sizeof(mio_affiliation_t));
    aff->type = MIO_AFFILIATION_NONE;
    return aff;
}

/**
 * @ingroup PubSub
 * Frees an allocated mio affiliation struct.
 *
 * @param affiliation A pointer to the allocated mio affiliation struct to be freed.
 */
void mio_affiliation_free(mio_affiliation_t *affiliation) {
    if (affiliation->affiliation) free(affiliation->affiliation);
    free(affiliation);
}

/**
 * @ingroup PubSub
 * Returns the tail of a mio affiliation linked list
 *
 * @param affiliation A pointer to an element in a mio affiliation linked list.
 * @returns A pointer to the tail of a mio affiliation linked list.
 */
mio_affiliation_t *_mio_affiliation_tail_get(mio_affiliation_t *affiliation) {
    mio_affiliation_t *tail = NULL;

    while (affiliation != NULL ) {
        tail = affiliation;
        affiliation = affiliation->next;
    }
    return tail;
}

/**
 * @ingroup PubSub
 * Adds a mio_subscription to a mio packet.
 *
 * @param pkt A pointer the mio packet to which the affiliation should be added to.
 * @param affiliation A string containing the JID of the node or user that is affiliated.
 * @param type The type of affiliation to be added.
 */
void _mio_affiliation_add(mio_packet_t *pkt, char *affiliation,
                          mio_affiliation_type_t type) {
    mio_affiliation_t *aff;
    aff = _mio_affiliation_tail_get((mio_affiliation_t*) pkt->payload);
    if (aff == NULL ) {
        aff = mio_affiliation_new();
        mio_packet_payload_add(pkt, aff, MIO_PACKET_AFFILIATIONS);
    } else {
        aff->next = mio_affiliation_new();
        aff = aff->next;
    }

    aff->affiliation = malloc((strlen(affiliation) + 1) * sizeof(char));
    strcpy(aff->affiliation, affiliation);

    aff->type = type;

    pkt->num_payloads++;
}


int mio_subscribe(mio_conn_t * conn, const char *node,
                  mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *subscribe = NULL;
    int err;
    mio_packet_t *pkt;
    mio_subscription_t *sub;
    mio_response_t *subscriptions = mio_response_new();

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process subscribe request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    err = mio_subscriptions_query(conn, NULL, subscriptions);
    if (subscriptions->response_type != MIO_RESPONSE_ERROR) {
        pkt = (mio_packet_t*) subscriptions->response;
        sub = (mio_subscription_t*) pkt->payload;
        while (sub != NULL ) {
            if (strcmp(sub->subscription, node) == 0) {
                mio_error(
                    "Already subscribed to node %s, aborting subscription request",
                    node);
                mio_response_free(subscriptions);
                return MIO_ERROR_ALREADY_SUBSCRIBED;
            } else
                sub = sub->next;
        }
    } else if (subscriptions->response_type == MIO_RESPONSE_ERROR) {
        mio_response_print(subscriptions);
        mio_response_free(subscriptions);
        return err;
    } else {
        mio_response_free(subscriptions);
        return err;
    }

    iq = mio_pubsub_set_stanza_new(conn, node);

// Create a new subscribe stanza
    subscribe = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(subscribe, "subscribe");
    xmpp_stanza_set_attribute(subscribe, "node", node);
    xmpp_stanza_set_attribute(subscribe, "jid", conn->xmpp_conn->jid);

// Build xmpp message
    xmpp_stanza_add_child(iq->xmpp_stanza->children, subscribe);

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_subscribe,
                            response);

// Release the stanzas
    xmpp_stanza_release(subscribe);
    mio_stanza_free(iq);

    mio_response_free(subscriptions);
    return err;
}

/** Queries for the subscribers of the event node. Subject to the access
 *      rights of the logged in user.
 *
 * @param conn Active MIO connection.
 * @param node The event node uuid to query for subscription information.
 * @param response The response from the XMPP server. Contains subscription
 *      information
 * */
int mio_subscriptions_query(mio_conn_t *conn, const char *node,
                            mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *subscriptions = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process subscriptions query since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_get_stanza_new(conn, node);

// Create a new subscriptions stanza
    subscriptions = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(subscriptions, "subscriptions");

// If we are querying the affiliations of a node, add the node attribute to the subscriptions stanza and change the namespace of the pubsub stanza
    if (node != NULL ) {
        xmpp_stanza_set_attribute(subscriptions, "node", node);
        xmpp_stanza_set_ns(iq->xmpp_stanza->children,
                           "http://jabber.org/protocol/pubsub#owner");
    }

// Build xmpp message
    xmpp_stanza_add_child(iq->xmpp_stanza->children, subscriptions);

// Send out the stanza
    err = mio_send_blocking(conn, iq,
                            (mio_handler) mio_handler_subscriptions_query, response);

// Release unneeded stanzas
    xmpp_stanza_release(subscriptions);
    mio_stanza_free(iq);

    return err;
}

int mio_acl_affiliations_query(mio_conn_t *conn, const char *node,
                               mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *affiliations = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process affiliations query since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_get_stanza_new(conn, node);
    xmpp_stanza_set_ns(iq->xmpp_stanza->children,
                       "http://jabber.org/protocol/pubsub");

// Create a new affiliations stanza
    affiliations = xmpp_stanza_new(conn->xmpp_conn->ctx);
    if ((err = xmpp_stanza_set_name(affiliations, "affiliations")) < 0)
        return err;

// If node input is NULL, query affiliations of the jid, else query affiliations of the node
    if (node != NULL ) {
        if ((err = xmpp_stanza_set_attribute(affiliations, "node", node)) < 0)
            return err;
        xmpp_stanza_set_ns(iq->xmpp_stanza->children,
                           "http://jabber.org/protocol/pubsub#owner");
    }

// Build xmpp message
    xmpp_stanza_add_child(iq->xmpp_stanza->children, affiliations);

// Send out the stanza
    err = mio_send_blocking(conn, iq,
                            (mio_handler) mio_handler_acl_affiliations_query, response);

// Release unneeded stanzas
    xmpp_stanza_release(affiliations);
    mio_stanza_free(iq);

    return err;
}

/** Sets an event node's affiliation with a specific jid.
 *
 * @param conn Active MIO connection.
 * @param node Event node's UUID which will have its affiliation modified.
 * @param jid The user's jid whose affiliation with node is to change.
 * @param type The type of affiliation the jid should have with the event node
 * @param response [out] Where the response to the affiliation set request is stored.
 *
 * @returns SOX_OK on success, MIO_ERROR_DISCONNECTED on disconnection.
 * */
int mio_acl_affiliation_set(mio_conn_t *conn, const char *node, const char *jid,
                            mio_affiliation_type_t type, mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *affiliations = NULL, *affiliation = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process affiliation set request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_set_stanza_new(conn, node);
    xmpp_stanza_set_ns(iq->xmpp_stanza->children,
                       "http://jabber.org/protocol/pubsub#owner");

// Create a new affiliations stanza
    affiliations = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(affiliations, "affiliations");
    xmpp_stanza_set_attribute(affiliations, "node", node);

// Create a new affiliation stanza
    affiliation = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(affiliation, "affiliation");
    xmpp_stanza_set_attribute(affiliation, "jid", jid);

    switch (type) {
    case MIO_AFFILIATION_NONE:
        xmpp_stanza_set_attribute(affiliation, "affiliation", "none");
        break;
    case MIO_AFFILIATION_OWNER:
        xmpp_stanza_set_attribute(affiliation, "affiliation", "owner");
        break;
    case MIO_AFFILIATION_MEMBER:
        xmpp_stanza_set_attribute(affiliation, "affiliation", "member");
        break;
    case MIO_AFFILIATION_PUBLISHER:
        xmpp_stanza_set_attribute(affiliation, "affiliation", "publisher");
        break;
    case MIO_AFFILIATION_PUBLISH_ONLY:
        xmpp_stanza_set_attribute(affiliation, "affiliation", "publish-only");
        break;
    case MIO_AFFILIATION_OUTCAST:
        xmpp_stanza_set_attribute(affiliation, "affiliation", "outcast");
        break;
    default:
        mio_error("Unknown or missing affiliation type");
        return MIO_ERROR_UNKNOWN_AFFILIATION_TYPE;
        break;
    }

// Build xmpp message
    xmpp_stanza_add_child(affiliations, affiliation);
    xmpp_stanza_add_child(iq->xmpp_stanza->children, affiliations);

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
                            response);

// Release unneeded stanzas
    xmpp_stanza_release(affiliations);
    xmpp_stanza_release(affiliation);
    mio_stanza_free(iq);

    return err;
}

/** Unsubscribes a user from a event node. Can usnusbscribe
 *
 * @param conn Active MIO connection.
 * @param node Event node id to unsubscribe from.
 * @param sub_id Optional subscription id.
 * @param response Response to unsubscribe request.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED when conn disconnected.
 * */
int mio_unsubscribe(mio_conn_t * conn, const char *node, const char *sub_id,
                    mio_response_t * response) {

    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *unsubscribe = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process unsubscribe request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_set_stanza_new(conn, node);

// Create unsubscribe stanza
    unsubscribe = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(unsubscribe, "unsubscribe");
    xmpp_stanza_set_attribute(unsubscribe, "node", node);
    xmpp_stanza_set_attribute(unsubscribe, "jid", conn->xmpp_conn->jid);

// Add subid if we got one
    if (sub_id != NULL ) {
        if ((err = xmpp_stanza_set_attribute(unsubscribe, "subid", sub_id)) < 0)
            return err;
    }

// Build xmpp message
    xmpp_stanza_add_child(iq->xmpp_stanza->children, unsubscribe);

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
                            response);

// Release the stanzas
    xmpp_stanza_release(unsubscribe);
    mio_stanza_free(iq);

    return err;
}
int mio_handler_subscribe(mio_conn_t * const conn, mio_stanza_t * const stanza,
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

    int err = mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_subscribe,
                            NULL );

    if (err == MIO_OK)
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

    return err;
}

void XMLCALL mio_XMLstart_subscriptions_query(void *data,
        const char *element_name, const char **attr) {

    char *curr_jid_node = NULL, *curr_sub_id = NULL;
    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "subscriptions") == 0) {
        response->response_type = MIO_RESPONSE_PACKET;
        packet->type = MIO_PACKET_SUBSCRIPTIONS;
    }

    else if (strcmp(element_name, "subscription") == 0) {
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "jid") == 0) {
                if (curr_jid_node != NULL )
                    free(curr_jid_node);
                curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
                strcpy(curr_jid_node, attr[i + 1]);
            } else if (strcmp(attr_name, "node") == 0) {
                if (curr_jid_node != NULL )
                    free(curr_jid_node);
                curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
                strcpy(curr_jid_node, attr[i + 1]);
            } else if (strcmp(attr_name, "subid") == 0) {
                curr_sub_id = malloc(strlen(attr[i + 1]) + 1);
                strcpy(curr_sub_id, attr[i + 1]);
            }
        }
        mio_subscription_add(packet, curr_jid_node, curr_sub_id);
        if (curr_jid_node != NULL )
            free(curr_jid_node);
        if (curr_sub_id != NULL )
            free(curr_sub_id);

    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}

void XMLCALL mio_XMLstart_subscribe(void *data, const char *element_name,
                                    const char **attr) {

    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "subscription") == 0) {
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "subscription") == 0) {
                if (strcmp(attr[i + 1], "subscribed") == 0)
                    response->response_type = MIO_RESPONSE_OK;
            }
        }
    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}

int mio_handler_subscriptions_query(mio_conn_t * const conn,
                                    mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
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

    mio_packet_t *packet = mio_packet_new();
    response->response = packet;
    int err = mio_xml_parse(conn, stanza, xml_data,
                            mio_XMLstart_subscriptions_query, NULL );
    if (err == MIO_OK)
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

    return err;
}

int mio_handler_acl_affiliations_query(mio_conn_t * const conn,
                                       mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
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

    mio_packet_t *packet = mio_packet_new();
    response->response = packet;
    int err = mio_xml_parse(conn, stanza, xml_data,
                            mio_XMLstart_acl_affiliations_query, NULL );

    if (err == MIO_OK)
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

    return err;
}


void XMLCALL mio_XMLstart_acl_affiliations_query(void *data,
        const char *element_name, const char **attr) {

    char *curr_jid_node = NULL;
    mio_affiliation_type_t curr_aff = MIO_AFFILIATION_UNKNOWN;
    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "affiliations") == 0) {
        response->response_type = MIO_RESPONSE_PACKET;
        packet->type = MIO_PACKET_AFFILIATIONS;
    }

    else if (strcmp(element_name, "affiliation") == 0) {
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "jid") == 0) {
                if (curr_jid_node != NULL )
                    free(curr_jid_node);
                curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
                strcpy(curr_jid_node, attr[i + 1]);
            } else if (strcmp(attr_name, "node") == 0) {
                if (curr_jid_node != NULL )
                    free(curr_jid_node);
                curr_jid_node = malloc(strlen(attr[i + 1]) + 1);
                strcpy(curr_jid_node, attr[i + 1]);
            } else if (strcmp(attr_name, "affiliation") == 0) {
                if (strcmp(attr[i + 1], "none") == 0)
                    curr_aff = MIO_AFFILIATION_NONE;
                else if (strcmp(attr[i + 1], "owner") == 0)
                    curr_aff = MIO_AFFILIATION_OWNER;
                else if (strcmp(attr[i + 1], "member") == 0)
                    curr_aff = MIO_AFFILIATION_MEMBER;
                else if (strcmp(attr[i + 1], "publisher") == 0)
                    curr_aff = MIO_AFFILIATION_PUBLISHER;
                else if (strcmp(attr[i + 1], "publish-only") == 0)
                    curr_aff = MIO_AFFILIATION_PUBLISH_ONLY;
                else if (strcmp(attr[i + 1], "outcast") == 0)
                    curr_aff = MIO_AFFILIATION_OUTCAST;
            }
        }
        _mio_affiliation_add(packet, curr_jid_node, curr_aff);
        if (curr_jid_node != NULL )
            free(curr_jid_node);

    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}


