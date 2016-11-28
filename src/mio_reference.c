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

#include "mio_reference.h"
#include "mio_pubsub.h"
#include "mio_connection.h"
#include "mio_error.h"
#include "mio_handlers.h"
#include "mio_node.h"


extern mio_log_level_t _mio_log_level;
void XMLCALL mio_XMLstart_references_query(void *data,
        const char *element_name, const char **attr);


/**
 * @ingroup Meta
 * Overwrite an existing reference at a node.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the node to update.
 * @param ref_node A string containing the JID of the node contained in the reference to overwrite.
 * @param ref_type The new mio meta type of the node contained within the reference to update.
 * @param response  A pointer to an allocated mio response struct which will be populated with the server's response.
 * @returns MIO_OK on success, otherwise an error.
 */
int _mio_reference_meta_type_overwrite_publish(mio_conn_t *conn,
        char *node, char *ref_node, mio_reference_type_t ref_type,
        mio_meta_type_t ref_meta_type, mio_response_t *response) {
    mio_response_t *query_response = mio_response_new();
    mio_packet_t *packet;
    mio_reference_t *query_ref;
    mio_stanza_t *item;

    // Query node for its references
    mio_references_query(conn, node, query_response);
    if (query_response->response_type != MIO_RESPONSE_PACKET) {
        mio_response_free(query_response);
        return MIO_ERROR_UNEXPECTED_RESPONSE;
    }
    packet = (mio_packet_t*) query_response->response;
    if (packet->type != MIO_PACKET_REFERENCES) {
        mio_response_free(query_response);
        return MIO_ERROR_UNEXPECTED_PAYLOAD;
    }
    // If query was successful, remove reference that we want to overwrite
    query_ref = (mio_reference_t*) packet->payload;
    query_ref = mio_reference_remove(conn, query_ref, ref_type, ref_node);
    // If reference to remove was not present, don't publish anything
    if (query_ref != NULL ) {
        // Add new reference to list
        mio_reference_add(conn, query_ref, ref_type, ref_meta_type, ref_node);
        // Publish modified list of references
        item = mio_references_to_item(conn, query_ref);
        mio_item_publish(conn, item, node, response);
    }
// Cleanup
    mio_response_free(query_response);
    //mio_stanza_free(item);
    return MIO_OK;
}
mio_reference_t *mio_reference_new() {
    mio_reference_t *ref = malloc(sizeof(mio_reference_t));
    memset(ref, 0, sizeof(mio_reference_t));
    return ref;
}

void mio_reference_free(mio_reference_t *ref) {
    if (ref->node != NULL )
        free(ref->node);
    if (ref->name != NULL )
        free(ref->name);
    free(ref);
}

mio_reference_t *mio_reference_tail_get(mio_reference_t *ref) {
    mio_reference_t *tail = NULL;

    while (ref != NULL ) {
        tail = ref;
        ref = ref->next;
    }
    return tail;
}

int mio_references_query(mio_conn_t *conn, char *node, mio_response_t *response) {
    return mio_item_recent_get(conn, node, response, 1, "references",
                               (mio_handler *) mio_handler_references_query);
}

mio_stanza_t *mio_references_to_item(mio_conn_t* conn, mio_reference_t *ref) {
    mio_reference_t *curr;
    xmpp_stanza_t *ref_stanza, *reference_stanza;
    mio_stanza_t *item = mio_pubsub_item_new(conn, "references");

    reference_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(reference_stanza, "references");
    for (curr = ref; curr != NULL ; curr = curr->next) {
        ref_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(ref_stanza, "reference");

// Populate stanza with attributes from reference
        switch (curr->type) {
        case MIO_REFERENCE_CHILD:
            xmpp_stanza_set_attribute(ref_stanza, "type", "child");
            break;
        case MIO_REFERENCE_PARENT:
            xmpp_stanza_set_attribute(ref_stanza, "type", "parent");
            break;
        default:
            mio_stanza_free(item);
            return NULL ;
            break;
        }

        switch (curr->meta_type) {
        case MIO_META_TYPE_DEVICE:
            xmpp_stanza_set_attribute(ref_stanza, "metaType", "device");
            break;
        case MIO_META_TYPE_LOCATION:
            xmpp_stanza_set_attribute(ref_stanza, "metaType", "location");
            break;
        case MIO_META_TYPE_UKNOWN:
            xmpp_stanza_set_attribute(ref_stanza, "metaType", "unknown");
            break;
        default:
            mio_stanza_free(item);
            return NULL ;
            break;
        }

        if (curr->node != NULL )
            xmpp_stanza_set_attribute(ref_stanza, "node", curr->node);
        if (curr->name != NULL )
            xmpp_stanza_set_attribute(ref_stanza, "name", curr->name);

        xmpp_stanza_add_child(reference_stanza, ref_stanza);
        xmpp_stanza_release(ref_stanza);
    }
    xmpp_stanza_add_child(item->xmpp_stanza, reference_stanza);
    xmpp_stanza_release(reference_stanza);
    return item;
}

void mio_reference_add(mio_conn_t* conn, mio_reference_t *refs,
                       mio_reference_type_t type, mio_meta_type_t meta_type, char *node) {
    mio_reference_t *curr;
    mio_response_t *query_response;
    mio_packet_t *packet;
    mio_meta_t *meta;

    if (refs->type == MIO_REFERENCE_UNKOWN)
        curr = refs;
    else {
        curr = mio_reference_tail_get(refs);
        curr->next = mio_reference_new();
        curr = curr->next;
    }
    curr->type = type;
    curr->meta_type = meta_type;
    if (node != NULL ) {
        curr->node = strdup(node);

        // FIXME: Move this code to tool
        query_response = mio_response_new();
        mio_meta_query(conn, node, query_response);
        if (query_response->response_type != MIO_RESPONSE_PACKET) {
            mio_response_free(query_response);
            return;
        }
        packet = query_response->response;
        if (packet->type != MIO_PACKET_META) {
            mio_response_free(query_response);
            return;
        }
        meta = packet->payload;
        if (meta->name != NULL )
            curr->name = strdup(meta->name);
        mio_response_free(query_response);
    }
}

mio_reference_t *mio_reference_remove(mio_conn_t* conn, mio_reference_t *refs,
                                      mio_reference_type_t type, char *node) {
    mio_reference_t *prev = NULL, *curr;
    curr = refs;
    while (curr != NULL ) {
        if (curr->type == type && strcmp(curr->node, node) == 0) {
            if (prev == NULL ) {
                curr = curr->next;
                return curr;
            }
            prev->next = curr->next;
            curr->next = NULL;
            mio_reference_free(curr);
            return refs;
        }
        prev = curr;
        curr = curr->next;
    }
    return refs;
}

int mio_reference_child_remove(mio_conn_t *conn, char* parent, char *child,
                               mio_response_t *response) {
    mio_response_t *query_parent = mio_response_new();
    mio_response_t *query_child, *pub_child = NULL, *pub_parent = NULL;
    mio_packet_t *packet;
    mio_reference_t *parent_refs, *child_refs;
    mio_stanza_t *child_item = NULL, *parent_item = NULL;
    mio_response_error_t *rerr;
    int err = 0;

// Query child for current references
    query_child = mio_response_new();
    mio_references_query(conn, child, query_child);

    if (query_child->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) query_child->response;

        if (packet->type == MIO_PACKET_REFERENCES) {
            child_refs = packet->payload;

// Convert references to items and send them out

            if (child_refs != NULL ) {
                child_refs = mio_reference_remove(conn, child_refs,
                                                  MIO_REFERENCE_PARENT, parent);
// If we removed all references, update the packet so that the payload isn't freed twice
                if (child_refs == NULL )
                    packet->payload = NULL;
                child_item = mio_references_to_item(conn, child_refs);
		if (child_item == NULL) { 
			child_item = mio_pubsub_item_new(conn,"references");
		}
                pub_child = mio_response_new();
                mio_item_publish(conn, child_item, child, pub_child);
                if (pub_child->response_type == MIO_RESPONSE_ERROR) {
                    mio_error("Error publishing to child node\n");
                    mio_response_print(pub_child);
                }
            }
        }
    }
// Query parent for current references
    mio_references_query(conn, parent, query_parent);

    if (query_parent->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) query_parent->response;

        if (packet->type == MIO_PACKET_REFERENCES) {
            parent_refs = packet->payload;

            if (parent_refs != NULL ) {
                parent_refs = mio_reference_remove(conn, parent_refs,
                                                   MIO_REFERENCE_CHILD, child);
// If we removed all references, update the packet so that the payload isn't freed twice
                if (parent_refs == NULL )
                    packet->payload = NULL;
                parent_item = mio_references_to_item(conn, parent_refs);
                pub_parent = mio_response_new();
                err = mio_item_publish(conn, parent_item, parent, pub_parent);
                if (response->response_type == MIO_RESPONSE_ERROR)
                    mio_error("Error publishing to parent node\n");
            }
        }
    }

    if (pub_parent == NULL && pub_child == NULL ) {
        response->response_type = MIO_RESPONSE_ERROR;
        rerr = _mio_response_error_new();
        rerr->description = strdup("Reference not found at child or parent");
        // FIXME: Add proper error number
        rerr->err_num = 0;
        response->response = rerr;
    } else
        response->response_type = MIO_RESPONSE_OK;

// Cleanup
    mio_response_free(query_child);
    mio_response_free(query_parent);
    if (pub_parent != NULL )
        mio_response_free(pub_parent);
    if (pub_child != NULL )
        mio_response_free(pub_child);
    if (parent_item != NULL )
        mio_stanza_free(parent_item);
    if (child_item != NULL )
        mio_stanza_free(child_item);
    return err;
}

int mio_reference_child_add(mio_conn_t *conn, char* parent, char *child,
                            int add_reference_at_child, mio_response_t *response) {
    mio_reference_t *curr, *parent_refs = NULL, *child_refs = NULL;
    mio_response_t *query_parent = mio_response_new();
    mio_response_t *query_child, *pub_child, *query_child_type,
                   *query_parent_type;
    mio_packet_t *packet;
    mio_stanza_t *child_item, *parent_item;
// Default meta types to unknown types
    mio_meta_type_t parent_type = MIO_META_TYPE_UKNOWN;
    mio_meta_type_t child_type = MIO_META_TYPE_UKNOWN;
    mio_meta_t *meta;
    int err;

// Query node for current references
    mio_references_query(conn, parent, query_parent);

    if (query_parent->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) query_parent->response;

        if (packet->type == MIO_PACKET_REFERENCES && packet->num_payloads > 0) {
            parent_refs = packet->payload;
// Check if child reference already exists or if loop would be created
            for (curr = parent_refs; curr != NULL ; curr = curr->next) {
                if (strcmp(curr->node, child) == 0) {
                    if (curr->type == MIO_REFERENCE_CHILD) {
                        mio_response_free(query_parent);
                        mio_error(
                            "Child node %s already referenced from node %s\n",
                            child, parent);
                        return MIO_ERROR_DUPLICATE_ENTRY;
                    } else if (curr->type == MIO_REFERENCE_PARENT) {
                        mio_response_free(query_parent);
                        mio_error(
                            "Child node %s already referenced as parent from node %s\n",
                            child, parent);
                        return MIO_ERROR_REFERENCE_LOOP;
                    }
                }
            }
        } else
            parent_refs = mio_reference_new();
    } else
        parent_refs = mio_reference_new();

// If we want to add a reference link at the chikd, first check the parent type
    if (add_reference_at_child == MIO_ADD_REFERENCE_AT_CHILD) {
        query_parent_type = mio_response_new();
        mio_meta_query(conn, parent, query_parent_type);
// If we can't check the parent type (permissions issues, etc. leave it as unknown)
        if (query_parent_type->response_type == MIO_RESPONSE_PACKET) {
            packet = (mio_packet_t*) query_parent_type->response;
            if (packet->type == MIO_PACKET_META) {
                meta = (mio_meta_t*) packet->payload;
                if (meta != NULL )
                    parent_type = meta->meta_type;
            }
        }

// Check if node already referenced as parent at node or if loop would be created
        query_child = mio_response_new();
        mio_references_query(conn, child, query_child);

        if (query_child->response_type == MIO_RESPONSE_PACKET) {
            packet = (mio_packet_t*) query_child->response;

            if (packet->type == MIO_PACKET_REFERENCES
                    && packet->num_payloads > 0) {
                child_refs = (mio_reference_t*) packet->payload;

                for (curr = child_refs; curr != NULL ; curr = curr->next) {
                    if (strcmp(curr->node, parent) == 0) {
                        if (curr->type == MIO_REFERENCE_PARENT) {
                            mio_response_free(query_parent);
                            mio_response_free(query_child);
                            mio_error(
                                "Parent node %s already referenced as parent from node %s\n",
                                parent, child);
                            return MIO_ERROR_DUPLICATE_ENTRY;
                        } else if (curr->type == MIO_REFERENCE_CHILD) {
                            mio_response_free(query_parent);
                            mio_response_free(query_child);
                            mio_error(
                                "Node %s already referenced as child from node %s\n",
                                child, parent);
                            return MIO_ERROR_REFERENCE_LOOP;
                        }
                    }
                }
            } else
                child_refs = mio_reference_new();
        } else
            child_refs = mio_reference_new();

        // Add parent references
        mio_reference_add(conn, child_refs, MIO_REFERENCE_PARENT, parent_type,
                          parent);
        // Convert references to items and send them out
        child_item = mio_references_to_item(conn, child_refs);
        // Publish items
        pub_child = mio_response_new();
        err = mio_item_publish(conn, child_item, child, pub_child);
        // If publication failed, return error response
        if (pub_child->response_type == MIO_RESPONSE_ERROR) {
            response->response_type = MIO_RESPONSE_ERROR;
            mio_error("Error adding reference to child node %s\n", child);
        }
        mio_response_free(pub_child);
    }
// Check child type
    query_child_type = mio_response_new();
    mio_meta_query(conn, child, query_child_type);
// If we can't check the child type (permissions issues, etc. leave it as unknown)
    if (query_child_type->response_type == MIO_RESPONSE_PACKET) {
        packet = (mio_packet_t*) query_child_type->response;
        if (packet->type == MIO_PACKET_META) {
            meta = (mio_meta_t*) packet->payload;
            if (meta != NULL )
                child_type = meta->meta_type;
        }
    }
// Add child references
    mio_reference_add(conn, parent_refs, MIO_REFERENCE_CHILD, child_type,
                      child);

// Convert references to items and send them out
    parent_item = mio_references_to_item(conn, parent_refs);

// If publication was successful, publish to parent
    err = mio_item_publish(conn, parent_item, parent, response);
    if (response->response_type == MIO_RESPONSE_ERROR)
        mio_error("Error adding reference to parent node %s\n", parent);
    return err;
}

void XMLCALL mio_XMLstart_references_query(void *data,
        const char *element_name, const char **attr) {

    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    mio_reference_t *ref;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "items") == 0) {
        response->response_type = MIO_RESPONSE_PACKET;
        packet->type = MIO_PACKET_REFERENCES;
    }
    if (strcmp(element_name, "reference") == 0) {
        packet->num_payloads++;
        if (packet->payload == NULL ) {
            packet->payload = mio_reference_new();
            ref = (mio_reference_t*) packet->payload;
        } else {
            ref = mio_reference_tail_get((mio_reference_t*) packet->payload);
            ref->next = mio_reference_new();
            ref = ref->next;
        }
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "type") == 0) {
                if (strcmp(attr[i + 1], "child") == 0)
                    ref->type = MIO_REFERENCE_CHILD;
                else if (strcmp(attr[i + 1], "parent") == 0)
                    ref->type = MIO_REFERENCE_PARENT;
            } else if (strcmp(attr_name, "metaType") == 0) {
                if (strcmp(attr[i + 1], "device") == 0)
                    ref->meta_type = MIO_META_TYPE_DEVICE;
                else if (strcmp(attr[i + 1], "location") == 0)
                    ref->meta_type = MIO_META_TYPE_LOCATION;
            } else if (strcmp(attr_name, "node") == 0) {
                ref->node = strdup(attr[i + 1]);
            } else if (strcmp(attr_name, "name") == 0) {
                ref->name = strdup(attr[i + 1]);
            }
        }
    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}

int mio_handler_references_query(mio_conn_t * const conn,
                                 mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
    mio_stanza_t *stanza_copy;
    int err;
    mio_request_t *request = _mio_request_get(conn, response->id);
    if (request == NULL ) {
        mio_error("Request with id %s not found, aborting handler",
                  response->id);
        return MIO_ERROR_REQUEST_NOT_FOUND;
    }
    mio_packet_t *packet = mio_packet_new();
    mio_xml_parser_data_t *xml_data = mio_xml_parser_data_new();
    xml_data->response = response;
    stanza_copy = mio_stanza_clone(conn, stanza);
    response->stanza = stanza_copy;

    response->response = packet;
    err = mio_xml_parse(conn, stanza, xml_data, mio_XMLstart_references_query,
                        NULL );

    if (err == MIO_OK) {
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
    }

    return err;
}

int mio_reference_update(mio_conn_t *conn, char* event_node_id) {
	return -1;
}

