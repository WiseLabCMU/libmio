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

#include "mio_connection.h"
#include "mio_collection.h"
#include "mio_error.h"
#include "mio_handlers.h"
#include "mio_node.h"
#include <mio_pubsub.h>

extern mio_log_level_t _mio_log_level;
/**
 * @ingroup Collections
 * Internal function to allocate and initialize a new mio collection struct.
 *
 * @returns A pointer to a newly allocated and initialized mio collection struct.
 */
mio_collection_t *mio_collection_new() {
    mio_collection_t *coll = malloc(sizeof(mio_collection_t));
    memset(coll, 0, sizeof(mio_collection_t));
    return coll;
}

/**
 * @ingroup Collections
 * Internal function to free an allocated mio collection struct.
 *
 * @param collection A pointer to the allocated mio collection struct to be freed.
 */
void mio_collection_free(mio_collection_t *collection) {
    if (collection->name != NULL )
        free(collection->name);
    if (collection->node != NULL )
        free(collection->node);
    free(collection);
}

/**
 * @ingroup Collections
 * Returns the tail of a mio affiliation linked list
 *
 * @param collection A pointer to an element in a mio collection linked list.
 * @returns A pointer to the tail of a mio collection linked list.
 */
mio_collection_t *_mio_collection_tail_get(mio_collection_t *collection) {
    mio_collection_t *tail = NULL;

    while (collection != NULL ) {
        tail = collection;
        collection = collection->next;
    }
    return tail;
}

/**
 * @ingroup Collections
 * Creates and adds a mio collection to a mio packet.
 *
 * @param pkt A pointer the mio packet to which the collection should be added to.
 * @param node A string containing the JID of the node that is part of the collection.
 * @param name A string containing the name of the node that is part of the collection.
 */
void _mio_collection_add(mio_packet_t *pkt, char *node, char *name) {
    mio_collection_t *coll;
    coll = _mio_collection_tail_get((mio_collection_t*) pkt->payload);

    if (coll == NULL ) {
        coll = mio_collection_new();
        mio_packet_payload_add(pkt, coll, MIO_PACKET_COLLECTIONS);
    } else {
        coll->next = mio_collection_new();
        coll = coll->next;
    }
    if (name != NULL ) {
        coll->name = malloc((strlen(name) + 1) * sizeof(char));
        strcpy(coll->name, name);
    }
    if (node != NULL ) {
        coll->node = malloc((strlen(node) + 1) * sizeof(char));
        strcpy(coll->node, node);
    }

    pkt->num_payloads++;
}

/** Creates an collection event node.
 *
 * @param conn Active MIO connection.
 * @param node The unique node id of the collection node. Should be UUID.
 * @param name The name of the collection node. Need not be unique.
 *
 * @returns MIO_OK upon success, MIO_ERROR_DISCONNECTED on connection failure.
 * */
int mio_collection_node_create(mio_conn_t * conn, const char *node,
                               const char *title, mio_response_t * response) {

    int err;
    xmpp_stanza_t *create = NULL, *curr = NULL, *field_title = NULL, *value =
            NULL, *value_text = NULL;
    mio_stanza_t *collection_stanza = mio_node_config_new(conn, node,
                                      MIO_NODE_TYPE_COLLECTION);
// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process collection node creation request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    create = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(create, "create");
    xmpp_stanza_set_attribute(create, "node", node);

    if (title != NULL ) {
        field_title = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(field_title, "field");
        xmpp_stanza_set_attribute(field_title, "var", "pubsub#title");
        value = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(value, "value");
        value_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_text(value_text, title);
        xmpp_stanza_add_child(value, value_text);
        xmpp_stanza_add_child(field_title, value);
        xmpp_stanza_add_child(
            collection_stanza->xmpp_stanza->children->children->children,
            field_title);
    }

// Put create stanza ahead of config stanza in collection_stanza
    curr = collection_stanza->xmpp_stanza->children->children;
    curr->prev = create;
    create->next = curr;
    collection_stanza->xmpp_stanza->children->children = create;

// Send out the stanza
    err = mio_send_blocking(conn, collection_stanza,
                            (mio_handler) mio_handler_error, response);

    mio_stanza_free(collection_stanza);

    return err;
}

/** Allows user to configure existing collection node.
 *
 * @param conn Active MIO connection.
 * @param node The node id of the collection node.
 * @param collection_stanza Stanza describing collection node.
 * @param response Pointer to allocated mio response.
 *      Holds response to configure request.
 *
 * @returns MIO_OK on success. MIO_ERROR_DISCONNECTED on connection error.
 * */
int mio_collection_node_configure(mio_conn_t * conn, const char *node,
                                  mio_stanza_t *collection_stanza, mio_response_t * response) {

    xmpp_stanza_t *configure = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process collection node configuration request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

//Update pubsub ns to pubsub#owner
    xmpp_stanza_set_ns(collection_stanza->xmpp_stanza->children,
                       "http://jabber.org/protocol/pubsub#owner");

    configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(configure, "configure");
    xmpp_stanza_set_attribute(configure, "node", node);

    xmpp_stanza_add_child(collection_stanza->xmpp_stanza->children, configure);

// Send out the stanza
    err = mio_send_blocking(conn, collection_stanza,
                            (mio_handler) mio_handler_error, response);

    mio_stanza_free(collection_stanza);

    return err;
}

/** Queries collection node for children.
 *
 * @param conn Active MIO connection.
 * @param node Node id of the collection node to query.
 * @param response The response packet containing the children of the collection node
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on MIO disconnection.
 * */
int mio_collection_children_query(mio_conn_t * conn, const char *node,
                                  mio_response_t * response) {

    int err;
    xmpp_stanza_t *query = NULL;

// Create stanzas
    mio_stanza_t *iq = mio_pubsub_iq_get_stanza_new(conn, node);
    query = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, "http://jabber.org/protocol/disco#items");
    xmpp_stanza_set_attribute(query, "node", node);
    xmpp_stanza_add_child(iq->xmpp_stanza, query);

// Send out the stanza
    err = mio_send_blocking(conn, iq,
                            (mio_handler) mio_handler_collection_children_query, response);

    mio_stanza_free(iq);

    return err;
}

/** Queries collection node for parents.
 *
 * @param conn Active MIO connection.
 * @param node Node id of the collection node to query.
 * @param response The response packet containing the parents of the collection node
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on MIO disconnection.
 * */
int mio_collection_parents_query(mio_conn_t * conn, const char *node,
                                 mio_response_t * response) {

    int err;
    xmpp_stanza_t *configure = NULL;

// Create stanzas
    mio_stanza_t *iq = mio_pubsub_get_stanza_new(conn, node);
    xmpp_stanza_set_ns(iq->xmpp_stanza->children,
                       "http://jabber.org/protocol/pubsub#owner");
// Create configure stanza
    configure = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(configure, "configure");
    xmpp_stanza_set_attribute(configure, "node", node);

// Build message
    xmpp_stanza_add_child(iq->xmpp_stanza->children, configure);

// Send out the stanza
    err = mio_send_blocking(conn, iq,
                            (mio_handler) mio_handler_collection_parents_query, response);

    mio_stanza_free(iq);

    return err;
}

/** Adds a child node to a collection nodes stanza
 *
 * @param conn Active MIO connection.
 * @param child The node id of the child to add to the collection node's stanza
 * @param collection_stanza The collection stanza to add the child node id to.
 *
 * @returns MIO_OK upon success, MIO_ERROR_DISCONNECTED if disconnected.
 * */
int mio_collection_config_child_add(mio_conn_t * conn, const char *child,
                                    mio_stanza_t *collection_stanza) {

    xmpp_stanza_t *value_child = NULL, *value_child_text = NULL, *field = NULL,
                   *curr = NULL, *curr_value;
    char *curr_attribute = NULL;

    xmpp_stanza_set_ns(collection_stanza->xmpp_stanza->children,
                       "http://jabber.org/protocol/pubsub#owner");

// Create value stanza for child
    value_child = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(value_child, "value");
    value_child_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_text(value_child_text, child);
    xmpp_stanza_add_child(value_child, value_child_text);

// Get the field stanzas and find the pubsub#children one
    curr = xmpp_stanza_get_children(
               collection_stanza->xmpp_stanza->children->children->children);
    while (curr != NULL ) {
        curr_attribute = xmpp_stanza_get_attribute(curr, "var");
        if (curr_attribute != NULL ) {
            if (strcmp(curr_attribute, "pubsub#children") == 0)
                break;
        }
        curr = xmpp_stanza_get_next(curr);
    }

// If there is no pubsub#children value stanza, create it and add the child collection node
    if (curr == NULL ) {
        field = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(field, "field");
        xmpp_stanza_set_attribute(field, "var", "pubsub#children");
        xmpp_stanza_add_child(field, value_child);
        xmpp_stanza_add_child(
            collection_stanza->xmpp_stanza->children->children->children,
            field);
    } else {
        curr_value = xmpp_stanza_get_children(curr);
        while (curr_value != NULL ) {
            if (strcmp(xmpp_stanza_get_text(curr_value), child) == 0)
                return MIO_ERROR_DUPLICATE_ENTRY;
            curr_value = xmpp_stanza_get_next(curr_value);
        }
// If no duplicate is found, add parent to list of parent nodes
        xmpp_stanza_add_child(curr, value_child);
    }

    return MIO_OK;
}

/** Adds collection node as parent to the child node.
 *
 * @param conn Active MIO connection.
 * @param parent Event node id of the collection node parent.
 * @param collection_stanza The child's collection stanza.
 *
 * @returns SOX_OK on success, SOX_ERROR_DISCONNECTED when disconected.
 * */
int mio_collection_config_parent_add(mio_conn_t * conn, const char *parent,
                                     mio_stanza_t *collection_stanza) {

    xmpp_stanza_t *value_parent = NULL, *value_parent_text = NULL,
                   *field = NULL, *curr = NULL, *curr_value = NULL;

    char *curr_attribute = NULL;

    xmpp_stanza_set_ns(collection_stanza->xmpp_stanza->children,
                       "http://jabber.org/protocol/pubsub#owner");
// Create value stanza for parent
    value_parent = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(value_parent, "value");
    value_parent_text = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_text(value_parent_text, parent);
    xmpp_stanza_add_child(value_parent, value_parent_text);

// Get the field stanzas and find the pubsub#collection one
    curr = xmpp_stanza_get_children(
               collection_stanza->xmpp_stanza->children->children->children);
    while (curr != NULL ) {
        curr_attribute = xmpp_stanza_get_attribute(curr, "var");
        if (curr_attribute != NULL ) {
            if (strcmp(curr_attribute, "pubsub#collection") == 0)
                break;
        }
        curr = xmpp_stanza_get_next(curr);
    }

// If there is no pubsub#collection value stanza, create it and add the child collection node
    if (curr == NULL ) {
        field = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(field, "field");
        xmpp_stanza_set_attribute(field, "var", "pubsub#collection");
        xmpp_stanza_add_child(field, value_parent);
        xmpp_stanza_add_child(
            collection_stanza->xmpp_stanza->children->children->children,
            field);
// Check if we are trying to add a duplicate parent
    } else {
        curr_value = xmpp_stanza_get_children(curr);
        while (curr_value != NULL ) {
            if (strcmp(xmpp_stanza_get_text(curr_value), parent) == 0)
                return MIO_ERROR_DUPLICATE_ENTRY;
            curr_value = xmpp_stanza_get_next(curr_value);
        }
// If no duplicate is found, add parent to list of parent nodes
        xmpp_stanza_add_child(curr, value_parent);
    }

    return MIO_OK;
}

int mio_collection_child_add(mio_conn_t * conn, const char *child,
                             const char *parent, mio_response_t *response) {

    mio_response_t *temp_response = mio_response_new();
    mio_stanza_t *collection_stanza = mio_node_config_new(conn, parent,
                                      MIO_NODE_TYPE_UNKNOWN);
    mio_collection_children_query(conn, parent, temp_response);
    if (temp_response->response_type == MIO_RESPONSE_PACKET) {
        mio_packet_t *packet = (mio_packet_t*) temp_response->response;
        mio_collection_t *coll = (mio_collection_t*) packet->payload;
        while (coll != NULL ) {
            mio_collection_config_child_add(conn, coll->node,
                                            collection_stanza);
            coll = coll->next;
        }
    }
    mio_collection_config_child_add(conn, child, collection_stanza);
    mio_response_free(temp_response);
    temp_response = mio_response_new();
    mio_send_blocking(conn, collection_stanza, (mio_handler) mio_handler_error,
                      temp_response);

    mio_response_free(temp_response);
    temp_response = mio_response_new();
    mio_collection_parents_query(conn, child, temp_response);
    mio_stanza_free(collection_stanza);
    collection_stanza = mio_node_config_new(conn, child, MIO_NODE_TYPE_UNKNOWN);
    if (temp_response->response_type == MIO_RESPONSE_PACKET) {
        mio_packet_t *packet = (mio_packet_t*) temp_response->response;
        mio_collection_t *coll = (mio_collection_t*) packet->payload;
        while (coll != NULL ) {
            mio_collection_config_parent_add(conn, coll->node,
                                             collection_stanza);
            coll = coll->next;
        }
    }
    mio_response_free(temp_response);
    mio_collection_config_parent_add(conn, parent, collection_stanza);
    mio_send_blocking(conn, collection_stanza, (mio_handler) mio_handler_error,
                      response);
    mio_stanza_free(collection_stanza);

    return MIO_OK;
}

int mio_collection_child_remove(mio_conn_t * conn, const char *child,
                                const char *parent, mio_response_t *response) {

    int remove_flag = 0;
    mio_response_t *response_parent = NULL, *response_child = NULL;
    mio_stanza_t *collection_stanza_parent, *collection_stanza_child;
    mio_packet_t *packet = NULL;
    mio_collection_t *coll = NULL;
    //mio_response_error_t rerr;

// Query parent and child nodes for children and parents respectively
    collection_stanza_parent = mio_stanza_new(conn);
    collection_stanza_child = mio_stanza_new(conn);
    mio_collection_children_query(conn, parent, response_parent);
    mio_collection_parents_query(conn, child, response_child);

// Parse children affiliated with parent and add to new configuration stanza
    collection_stanza_parent = mio_node_config_new(conn, parent,
                               MIO_NODE_TYPE_UNKNOWN);
    if (response_parent->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) response_parent->response;
        coll = (mio_collection_t*) packet->payload;
        while (coll != NULL ) {
            if (strcmp(coll->node, child) != 0)
                mio_collection_config_child_add(conn, coll->node,
                                                collection_stanza_parent);
            else
                remove_flag++;
            coll = coll->next;
        }
    }
    if (remove_flag == 0)
        mio_error(
            "Child %s not affiliated with parent %s, check if node IDs are correct.",
            child, parent);
    else {
// If only one child was affiliated with the parent, add a blank value stanza to indicate that the parent has no children
        if (packet->num_payloads == 1)
            mio_collection_config_child_add(conn, "", collection_stanza_parent);
    }

    mio_response_free(response_parent);
    response_child = mio_response_new();
    mio_collection_parents_query(conn, child, response_child);
    collection_stanza_child = mio_node_config_new(conn, child,
                              MIO_NODE_TYPE_UNKNOWN);
    if (response_child->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) response_child->response;
        coll = (mio_collection_t*) packet->payload;
        while (coll != NULL ) {
            if (strcmp(coll->node, parent) != 0)
                mio_collection_config_parent_add(conn, coll->node,
                                                 collection_stanza_child);
            else
                remove_flag++;
            coll = coll->next;
        }
    }
    if (remove_flag < 2)
        mio_error(
            "Parent %s not affiliated with child %s, check if node IDs are correct",
            parent, child);
    else {
// If only one parent was affiliated with the child, add a blank value stanza to indicate that the parent has no children
        if (packet->num_payloads == 1)
            mio_collection_config_parent_add(conn, "", collection_stanza_child);

// If the child was found at the parent and vice versa, send out the new config stanzas to update the nodes
        mio_send_blocking(conn, collection_stanza_parent,
                          (mio_handler) mio_handler_error, response);
        if (response->response_type == MIO_RESPONSE_OK) {
            mio_response_free(response);
            mio_send_blocking(conn, collection_stanza_child,
                              (mio_handler) mio_handler_error, response);
        } else {
            mio_error("Error removing child from parent node:");
            mio_response_print(response);
        }
    }
    mio_response_free(response_child);
    mio_stanza_free(collection_stanza_parent);
    mio_stanza_free(collection_stanza_child);
    if (remove_flag != 2)
        return MIO_ERROR_NOT_AFFILIATED;

    return MIO_OK;
}

// Mio handlers
static void XMLCALL mio_XMLstart_collection_children_query(void *data,
        const char *element_name, const char **attr) {

    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    char *curr_node = NULL, *curr_name = NULL;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "item") == 0) {
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "node") == 0) {
                curr_node = malloc(strlen(attr[i + 1]) + 1);
                strcpy(curr_node, attr[i + 1]);
            } else if (strcmp(attr_name, "name") == 0) {
                curr_name = malloc(strlen(attr[i + 1]) + 1);
                strcpy(curr_name, attr[i + 1]);
            }
        }
        _mio_collection_add(packet, curr_node, curr_name);
        if (curr_node != NULL )
            free(curr_node);
        if (curr_name != NULL )
            free(curr_name);

    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}

static void XMLCALL mio_XMLString_collection_query(void *data,
        const XML_Char *s, int len) {
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_packet_t *packet = xml_data->payload;
    char *curr_node = NULL;

    if (strcmp(xml_data->curr_element_name, "value") == 0) {
        curr_node = malloc(len + 1);
        snprintf(curr_node, len + 1, "%s", s);
        _mio_collection_add(packet, curr_node, NULL );
    }
}

static void XMLCALL mio_XMLstart_collection_parents_query(void *data,
        const char *element_name, const char **attr) {

    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    _mio_xml_data_update_start_element(xml_data, element_name);

    xml_data->prev_element_name = xml_data->curr_element_name;
    xml_data->curr_element_name = element_name;

    if (strcmp(element_name, "field") == 0) {
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "var") == 0) {
                if (strcmp("pubsub#collection", attr[i + 1]) == 0) {
                    xml_data->payload = (void*) packet;
                    XML_SetCharacterDataHandler(*xml_data->parser,
                                                mio_XMLString_collection_query);
                }
            } else
                XML_SetCharacterDataHandler(*xml_data->parser, NULL );
        }
    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}

int mio_handler_collection_children_query(mio_conn_t * const conn,
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
                            mio_XMLstart_collection_children_query, NULL );

    if (err == MIO_OK) {
        response->response_type = MIO_RESPONSE_PACKET;
        packet->type = MIO_PACKET_COLLECTIONS;
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
    }

    return err;
}

int mio_handler_collection_parents_query(mio_conn_t * const conn,
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
                            mio_XMLstart_collection_parents_query, NULL );

    if (err == MIO_OK) {
        response->response_type = MIO_RESPONSE_PACKET;
        packet->type = MIO_PACKET_COLLECTIONS;
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
    }

    return err;
}

