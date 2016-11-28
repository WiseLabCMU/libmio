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

#include "strophe.h"
#include <common.h>
#include <mio.h>
#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

extern mio_log_level_t _mio_log_level ;

/**
 * @ingroup Meta
 * Allocates and initializes a new mio meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio meta struct.
 */
mio_meta_t *mio_meta_new() {
    mio_meta_t *meta = malloc(sizeof(mio_meta_t));
    memset(meta, 0, sizeof(mio_meta_t));
    return meta;
}

/**
 * @ingroup Meta
 * Frees a mio meta struct.
 *
 * @param A pointer to the allocated mio meta struct to be freed.
 */
void mio_meta_free(mio_meta_t * meta) {
    mio_transducer_meta_t *t_curr, *t_next;
    mio_property_meta_t *p_curr, *p_next;
    for (t_curr = meta->transducers; t_curr != NULL ; t_curr = t_next) {
        t_next = t_curr->next;
        mio_transducer_meta_free(t_curr);
    }
    for (p_curr = meta->properties; p_curr != NULL ; p_curr = p_next) {
        p_next = p_curr->next;
        mio_property_meta_free(p_curr);
    }
    if (meta->geoloc != NULL )
        mio_geoloc_free(meta->geoloc);
    if (meta->name != NULL )
        free(meta->name);
    if (meta->info != NULL )
        free(meta->info);
    if (meta->timestamp != NULL )
        free(meta->timestamp);
    free(meta);
}

/**
 * @ingroup Meta
 * Allocates and initializes a new mio transducer meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio transducer meta struct.
 */
mio_transducer_meta_t * mio_transducer_meta_new() {
    mio_transducer_meta_t *t_meta = malloc(sizeof(mio_transducer_meta_t));
    memset(t_meta, 0, sizeof(mio_transducer_meta_t));
    return t_meta;
}

/**
 * @ingroup Meta
 * Frees a mio transducer meta struct. Does not free any successive members in the linked list.
 *
 * @param A pointer to the allocated mio transducer meta struct to be freed.
 */
void mio_transducer_meta_free(mio_transducer_meta_t *t_meta) {
    mio_enum_map_meta_t *e_meta, *e_meta_free;
    mio_property_meta_t *p_meta, *p_meta_free;

    if (t_meta->accuracy != NULL )
        free(t_meta->accuracy);
    if (t_meta->max_value != NULL )
        free(t_meta->max_value);
    if (t_meta->min_value != NULL )
        free(t_meta->min_value);
    if (t_meta->precision != NULL )
        free(t_meta->precision);
    if (t_meta->resolution != NULL )
        free(t_meta->resolution);
    if (t_meta->type != NULL )
        free(t_meta->type);
    if (t_meta->name != NULL )
        free(t_meta->name);
    if (t_meta->unit != NULL )
        free(t_meta->unit);
    if (t_meta->manufacturer != NULL )
        free(t_meta->manufacturer);
    if (t_meta->serial != NULL )
        free(t_meta->serial);
    if (t_meta->info != NULL )
        free(t_meta->info);
    if (t_meta->geoloc != NULL )
        mio_geoloc_free(t_meta->geoloc);
    e_meta = t_meta->enumeration;
    while (e_meta != NULL ) {
        e_meta_free = e_meta;
        e_meta = e_meta->next;
        mio_enum_map_meta_free(e_meta_free);
    }
    p_meta = t_meta->properties;
    while (p_meta != NULL ) {
        p_meta_free = p_meta;
        p_meta = p_meta->next;
        mio_property_meta_free(p_meta_free);
    }
    free(t_meta);
}

/**
 * @ingroup Meta
 * Allocates and initializes a new mio enum map meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio enum map meta struct.
 */
mio_enum_map_meta_t * mio_enum_map_meta_new() {
    mio_enum_map_meta_t *e_meta = malloc(sizeof(mio_enum_map_meta_t));
    memset(e_meta, 0, sizeof(mio_enum_map_meta_t));
    return e_meta;
}

/**
 * @ingroup Meta
 * Frees a mio enum map meta meta struct. Does not free any successive members in the linked list.
 *
 * @param A pointer to the allocated mio enum map meta struct to be freed.
 */
void mio_enum_map_meta_free(mio_enum_map_meta_t *e_meta) {
    if (e_meta->name != NULL )
        free(e_meta->name);
    if (e_meta->value != NULL )
        free(e_meta->value);
    free(e_meta);
}

/**
 * @ingroup Meta
 * Returns the tail of a mio enum map meta linked list.
 *
 * @param e_meta A pointer to an element in a mio enum map meta linked list.
 * @returns A pointer to the tail of a mio enum map meta linked list.
 */
mio_enum_map_meta_t *mio_enum_map_meta_tail_get(mio_enum_map_meta_t *e_meta) {
    mio_enum_map_meta_t *tail = NULL;

    while (e_meta != NULL ) {
        tail = e_meta;
        e_meta = e_meta->next;
    }
    return tail;
}

/**
 * @ingroup Meta
 * Performs a deep copy of a mio enum map meta linked list.
 *
 * @param e_meta A pointer to the mio enum map meta linked list to be cloned
 * @returns A pointer to the first element of the copy of the enum map meta linked list.
 */
mio_enum_map_meta_t *mio_enum_map_meta_clone(mio_enum_map_meta_t *e_meta) {
    mio_enum_map_meta_t *copy, *curr;
    copy = mio_enum_map_meta_new();
    curr = copy;
    while (e_meta != NULL ) {
        if (e_meta->name != NULL ) {
            curr->name = malloc(sizeof(char) * strlen(e_meta->name) + 1);
            strcpy(curr->name, e_meta->name);
        } else {
            mio_enum_map_meta_free(copy);
            return NULL ;
        }
        if (e_meta->value != NULL ) {
            curr->value = malloc(sizeof(char) * strlen(e_meta->value) + 1);
            strcpy(curr->value, e_meta->value);
        } else {
            mio_enum_map_meta_free(copy);
            return NULL ;
        }
        e_meta = e_meta->next;
        if (e_meta != NULL ) {
            curr->next = mio_enum_map_meta_new();
            curr = curr->next;
        }
    }
    return copy;
}

/**
 * @ingroup Meta
 * Allocates and initializes a new mio propery meta struct.
 *
 * @returns A pointer to the newly allocated and initialized mio property meta struct.
 */
mio_property_meta_t *mio_property_meta_new() {
    mio_property_meta_t *p_meta = malloc(sizeof(mio_property_meta_t));
    memset(p_meta, 0, sizeof(mio_property_meta_t));
    return p_meta;
}

/**
 * @ingroup Meta
 * Frees a mio property meta struct. Does not free any successive members in the linked list.
 *
 * @param p_meta A pointer to the allocated mio property meta struct to be freed.
 */
void mio_property_meta_free(mio_property_meta_t *p_meta) {
    if (p_meta->name != NULL )
        free(p_meta->name);
    if (p_meta->value != NULL )
        free(p_meta->value);
    free(p_meta);
}

/**
 * @ingroup Meta
 * Adds a mio property to a mio meta struct
 *
 * @param meta A pointer to the mio meta struct to which the property should be added to.
 * @param p_meta A pointer to the mio property meta struct to be added to the meta struct.
 */
void mio_property_meta_add(mio_meta_t *meta, mio_property_meta_t *p_meta) {
    mio_property_meta_t *curr = mio_property_meta_tail_get(meta->properties);
    if (curr == NULL )
        meta->properties = p_meta;
    else
        curr->next = p_meta;
}

/**
 * @ingroup Meta
 * Returns the tail of a mio property meta linked list
 *
 * @param A pointer to an element in a mio property meta linked list.
 * @returns A pointer to the tail of a mio property meta linked list.
 */
mio_property_meta_t *mio_property_meta_tail_get(mio_property_meta_t *p_meta) {
    mio_property_meta_t *tail = NULL;

    while (p_meta != NULL ) {
        tail = p_meta;
        p_meta = p_meta->next;
    }
    return tail;
}

/**
 * @ingroup Meta
 * Returns the tail of a mio transducer meta linked list
 *
 * @param A pointer to an element in a mio transducer meta linked list.
 * @returns A pointer to the tail of a mio transducer meta linked list.
 */
mio_transducer_meta_t *mio_transducer_meta_tail_get(
    mio_transducer_meta_t *t_meta) {
    mio_transducer_meta_t *tail = NULL;

    while (t_meta != NULL ) {
        tail = t_meta;
        t_meta = t_meta->next;
    }
    return tail;
}

/**
 * @ingroup Meta
 * Adds a mio transducer meta struct to a mio meta struct. If the mio meta struct already contains transducer meta structs, the mio transducer meta struct being added will be appended to the end of the linked list of mio transducer meta structs.
 *
 * @param meta A pointer to the mio meta struct to which the mio transducer meta struct should be added.
 * @param t_meta A pointer to the mio transducer meta struct to be added.
 */
void mio_transducer_meta_add(mio_meta_t *meta, mio_transducer_meta_t *t_meta) {
    mio_transducer_meta_t *curr = mio_transducer_meta_tail_get(
                                      meta->transducers);
    if (curr == NULL )
        meta->transducers = t_meta;
    else
        curr->next = t_meta;
}


/**
 * @ingroup Meta
 * Adds a mio enum map meta struct to a mio meta struct. If the mio meta struct already contains enum map meta structs, the mio enum map meta struct being added will be appended to the end of the linked list of mio enum map meta structs.
 *
 * @param meta A pointer to the mio meta struct to which the mio transducer meta struct should be added.
 * @param t_meta A pointer to the mio transducer meta struct to be added.
 */
void mio_enum_map_meta_add(mio_transducer_meta_t *t_meta,
                           mio_enum_map_meta_t *e_meta) {
    mio_enum_map_meta_t *curr = mio_enum_map_meta_tail_get(t_meta->enumeration);
    if (curr == NULL )
        t_meta->enumeration = e_meta;
    else
        curr->next = e_meta;
}

mio_meta_t* copy_meta(mio_meta_t* meta) {
    return NULL;
}

void mio_transducers_merge(mio_meta_t *meta_to_update, mio_meta_t *meta) {

}

void mio_properties_merge(mio_meta_t *meta_to_update, mio_meta_t *meta) {
}

/**
 * @ingroup Meta
 * Merge a mio meta struct with an existing one. Any members contained in meta will be used to overwrite the corresponding members in meta_to_update.
 *
 * @param meta_to_update A pointer to the mio meta struct to be updated
 * @param event A pointer to the mio meta struct containing the up to date data.
 */
void mio_meta_merge(mio_meta_t *meta_to_update, mio_meta_t *meta) {
    if (meta_to_update == NULL) {
        return;
    }
    if (meta == NULL) {
        return;
    }

    meta_to_update->meta_type = meta->meta_type;
    if (meta->name != NULL ) {
        if (meta_to_update->name != NULL )
            free(meta_to_update->name);
        meta_to_update->name = strdup(meta->name);
    }
    if (meta->info != NULL ) {
        if (meta_to_update->info != NULL )
            free(meta_to_update->info);
        meta_to_update->info = strdup(meta->info);
    }
    if (meta->geoloc != NULL ) {
        if (meta_to_update->geoloc == NULL )
            meta_to_update->geoloc = mio_geoloc_new();
        mio_geoloc_merge(meta_to_update->geoloc, meta->geoloc);
    }
    if (meta->properties != NULL ) {
        if (meta_to_update->properties == NULL )
            meta_to_update->properties = mio_property_meta_new();
        mio_meta_property_merge(meta_to_update->properties, meta->properties);
    }
    if (meta_to_update->timestamp != NULL ) {
        free(meta_to_update->timestamp);
    }
    meta_to_update->timestamp = strdup(meta->timestamp);
    //mio_meta_transducer_merge(query_meta->transducers, meta->transducers);
    //mio_meta_properties_merge(query_meta->properties, meta->properties);
}

/**
 * @ingroup Meta
 * Merge a mio transducer meta struct with an existing one. Any members contained in transducer will be used to overwrite the corresponding members in transducer_to_update. Any mio properties contained in the mio transducer meta structs will be merged by name.
 *
 * @param transducer_to_update A pointer to the mio transducer meta struct to be updated
 * @param transdcuer A pointer to the mio transducer meta struct containing the up to date data.
 */
void mio_meta_transducer_merge(mio_transducer_meta_t *transducer_to_update,
                               mio_transducer_meta_t *transducer) {
    mio_property_meta_t *property, *property_to_update;
    int updated_flag = 0;

    if (transducer->accuracy != NULL ) {
        if (transducer_to_update->accuracy != NULL )
            free(transducer_to_update->accuracy);
        transducer_to_update->accuracy = strdup(transducer->accuracy);
    }
    if (transducer->geoloc != NULL ) {
        if (transducer_to_update->geoloc == NULL )
            transducer_to_update->geoloc = mio_geoloc_new();
        mio_geoloc_merge(transducer_to_update->geoloc, transducer->geoloc);
    }
    if (transducer->interface != NULL ) {
        if (transducer_to_update->interface != NULL )
            free(transducer_to_update->interface);
        transducer_to_update->interface = strdup(transducer->interface);
    }
    if (transducer->manufacturer != NULL ) {
        if (transducer_to_update->manufacturer != NULL )
            free(transducer_to_update->manufacturer);
        transducer_to_update->manufacturer = strdup(transducer->manufacturer);
    }
    if (transducer->max_value != NULL ) {
        if (transducer_to_update->max_value != NULL )
            free(transducer_to_update->max_value);
        transducer_to_update->max_value = strdup(transducer->max_value);
    }
    if (transducer->min_value != NULL ) {
        if (transducer_to_update->min_value != NULL )
            free(transducer_to_update->min_value);
        transducer_to_update->min_value = strdup(transducer->min_value);
    }
    if (transducer->precision != NULL ) {
        if (transducer_to_update->precision != NULL )
            free(transducer_to_update->precision);
        transducer_to_update->precision = strdup(transducer->precision);
    }
    if (transducer->resolution != NULL ) {
        if (transducer_to_update->resolution != NULL )
            free(transducer_to_update->resolution);
        transducer_to_update->resolution = strdup(transducer->resolution);
    }
    if (transducer->serial != NULL ) {
        if (transducer_to_update->serial != NULL )
            free(transducer_to_update->serial);
        transducer_to_update->serial = strdup(transducer->serial);
    }
    if (transducer->type != NULL ) {
        if (transducer_to_update->type != NULL )
            free(transducer_to_update->type);
        transducer_to_update->type = strdup(transducer->type);
    }
    if (transducer->unit != NULL ) {
        if (transducer_to_update->unit != NULL )
            free(transducer_to_update->unit);
        transducer_to_update->unit = strdup(transducer->unit);
    }
    if (transducer->name != NULL ) {
        if (transducer_to_update->name != NULL )
            free(transducer_to_update->name);
        transducer_to_update->name = strdup(transducer->name);
    }
    if (transducer->info != NULL ) {
        if (transducer_to_update->info != NULL )
            free(transducer_to_update->info);
        transducer_to_update->info = strdup(transducer->info);
    }
    if (transducer->accuracy != NULL ) {
        if (transducer_to_update->accuracy != NULL )
            free(transducer_to_update->accuracy);
        transducer_to_update->accuracy = strdup(transducer->accuracy);
    }
//Overwrite (don't merge) enumeration map if it exists
    if (transducer->enumeration != NULL ) {
        if (transducer_to_update->enumeration != NULL )
            mio_enum_map_meta_free(transducer_to_update->enumeration);
        transducer_to_update->enumeration = mio_enum_map_meta_clone(
                                                transducer->enumeration);
    }
    // Merge properties of transducers
    if (transducer->properties != NULL ) {
        for (property = transducer->properties; property != NULL ; property =
                    property->next) {
            for (property_to_update = transducer_to_update->properties;
                    property_to_update != NULL ; property_to_update =
                        property_to_update->next) {
                if (strcmp(property_to_update->name, property->name) == 0) {
                    mio_meta_property_merge(property_to_update, property);
                    updated_flag = 1;
                }
            }
            // If we didn't find a property with the same name to update, add it to the end of the linked list of properties
            if (!updated_flag) {
                property_to_update = mio_property_meta_tail_get(
                                         transducer_to_update->properties);
                if (property_to_update == NULL ) {
                    transducer_to_update->properties = mio_property_meta_new();
                    property_to_update = transducer_to_update->properties;
                } else {
                    property_to_update->next = mio_property_meta_new();
                    property_to_update = property_to_update->next;
                }
                mio_meta_property_merge(property_to_update, property);
            } else
                updated_flag = 0;
        }

    }
}

/**
 * @ingroup Meta
 * Merge a mio property meta struct with an existing one.
 *
 * @param property_to_update A pointer to the mio property meta struct to be updated
 * @param property A pointer to the mio property meta struct containing the up to date data.
 */
void mio_meta_property_merge(mio_property_meta_t *property_to_update,
                             mio_property_meta_t *property) {
    if (property->name != NULL ) {
        if (property_to_update->name != NULL )
            free(property_to_update->name);
        property_to_update->name = strdup(property->name);
    }
    if (property->value != NULL ) {
        if (property_to_update->value != NULL )
            free(property_to_update->value);
        property_to_update->value = strdup(property->value);
    }
}



/**
 * @ingroup Meta
 * Merge the meta data currently contained in a node with data from mio meta, mio transducer meta and/or mio property meta structs.
 *
 * @param conn A pointer to an active mio connection.
 * @param node A string containing the JID of the node to update.
 * @param meta A pointer to the mio meta struct with new data to be merged.
 * @param transducers A pointer to the mio transducer meta struct with new data to be merged.
 * @param properties A pointer to the mio property meta with new data to be merged.
 * @param response  A pointer to an allocated mio response struct which will be populated with the server's response.
 * @returns MIO_OK on success, otherwise an error.
 */
int mio_meta_merge_publish(mio_conn_t *conn, char *node, mio_meta_t *meta,
                           mio_transducer_meta_t *transducers, mio_property_meta_t *properties,
                           mio_response_t *response) {
    mio_meta_t *query_meta;
    mio_packet_t *packet;
    mio_stanza_t *item;
    int err;
    int update_references = 0;
    mio_response_t *query_response = mio_response_new();
    mio_reference_t *references, *reference;

// Get the current meta information from the node
    mio_meta_query(conn, node, query_response);

    if (query_response->response_type != MIO_RESPONSE_PACKET)
        return MIO_ERROR_UNEXPECTED_RESPONSE;
    packet = (mio_packet_t*) query_response->response;
    if (packet->type != MIO_PACKET_META)
        return MIO_ERROR_UNEXPECTED_RESPONSE;
    query_meta = (mio_meta_t*) packet->payload;


    if (query_meta == NULL) {
        query_meta = mio_meta_new();
    }
    if (strcmp(query_meta->name,meta->name) != 0 ||
            query_meta->meta_type != meta->meta_type) {
        update_references = 1;
    }
    mio_meta_merge(query_meta, meta);

// Convert updated query_meta struct to a stanza and publish it
    item = mio_meta_to_item(conn, query_meta);
    err = mio_item_publish(conn, item, node, response);
    mio_response_free(query_response);
    if (err != MIO_OK) {
        return err;
    }
    mio_response_free(response);
    if (update_references) {
        response = mio_response_new();
        err = mio_references_query(conn,node,response);
        if (err != MIO_OK) {
            return err;
        }
        if (response->response == NULL) {
            return err;
        }
        packet = (mio_packet_t*) response->response;
        references = packet->payload;
        reference = references;
        while (reference != NULL) {
            //mio_references_update(reference->id);
            reference = reference->next;
        }
        mio_response_free(response);
    }
    return err;
}

mio_transducer_meta_t *mio_meta_transducer_remove(
    mio_transducer_meta_t *transducers, char *transducer_name) {
    mio_transducer_meta_t *prev = NULL, *curr;

    curr = transducers;
    while (curr != NULL ) {
        if (strcmp(curr->name, transducer_name) == 0) {
            if (prev == NULL ) {
                curr = curr->next;
                transducers->next = NULL;
                mio_transducer_meta_free(transducers);
                return curr;
            }
            prev->next = curr->next;
            curr->next = NULL;
            mio_transducer_meta_free(curr);
            return transducers;
        }
        prev = curr;
        curr = curr->next;
    }
    return transducers;
}

mio_property_meta_t *mio_meta_property_remove(mio_property_meta_t *properties,
        char *property_name) {
    mio_property_meta_t *prev = NULL, *curr;

    curr = properties;
    while (curr != NULL ) {
        if (strcmp(curr->name, property_name) == 0) {
            if (prev == NULL ) {
                curr = curr->next;
                properties->next = NULL;
                mio_property_meta_free(properties);
                return curr;
            }
            prev->next = curr->next;
            curr->next = NULL;
            mio_property_meta_free(curr);
            return properties;
        }
        prev = curr;
        curr = curr->next;
    }
    return properties;
}

void mio_meta_geoloc_remove(mio_meta_t *meta,
                            mio_transducer_meta_t *transducers, char *transducer_name) {
    mio_transducer_meta_t *curr;

    if (meta != NULL ) {
        mio_geoloc_free(meta->geoloc);
        meta->geoloc = NULL;
    }
    if (transducers != NULL && transducer_name != NULL ) {
        curr = transducers;
        while (curr != NULL ) {
            if (strcmp(curr->name, transducer_name) == 0) {
                mio_geoloc_free(curr->geoloc);
                curr->geoloc = NULL;
                break;
            }
            curr = curr->next;
        }
    }
}



int mio_meta_geoloc_remove_publish(mio_conn_t *conn, char *node,
                                   char *transducer_name, mio_response_t *response) {
    mio_packet_t *query_packet;
    mio_meta_t *query_meta;
    mio_response_t *query_response = mio_response_new();
    mio_stanza_t *item;
    int err;

    mio_meta_query(conn, node, query_response);
    if (query_response->response_type != MIO_RESPONSE_PACKET)
        return MIO_ERROR_UNEXPECTED_RESPONSE;

    query_packet = (mio_packet_t*) query_response->response;
    if (query_packet->type != MIO_PACKET_META)
        return MIO_ERROR_UNEXPECTED_PAYLOAD;

    query_meta = (mio_meta_t*) query_packet->payload;

// If no transducer is specified, remove the geoloc from the device registered at the node
    if (transducer_name == NULL )
        mio_meta_geoloc_remove(query_meta, NULL, NULL );
// Otherwise remove the geoloc from the specified transducer
    else
        mio_meta_geoloc_remove(NULL, query_meta->transducers, transducer_name);

//Publish the updated meta information
    item = mio_meta_to_item(conn, query_meta);
    err = mio_item_publish(conn, item, node, response);

//Cleanup
    mio_response_free(query_response);

    return err;
}

int mio_meta_remove_publish(mio_conn_t *conn, char *node, char *meta_name,
                            char **transducer_names, int num_transducer_names,
                            char **property_names, int num_property_names, mio_response_t *response) {
    mio_packet_t *query_packet;
    mio_meta_t *query_meta;
    mio_response_t *query_response = mio_response_new();
    mio_response_t *query_refs, *pub_refs;
    mio_packet_t *packet;
    mio_reference_t *ref, *curr_ref;
    mio_stanza_t *item, *iq;
    xmpp_stanza_t *remove_item = NULL, *retract = NULL;
    int err, i;

    mio_meta_query(conn, node, query_response);
    if (query_response->response_type != MIO_RESPONSE_PACKET)
        return MIO_ERROR_UNEXPECTED_RESPONSE;

    query_packet = (mio_packet_t*) query_response->response;
    if (query_packet->type != MIO_PACKET_META)
        return MIO_ERROR_UNEXPECTED_PAYLOAD;

    query_meta = (mio_meta_t*) query_packet->payload;

// If we want to remove the entire device, remove the entire meta item
    if (meta_name != NULL ) {
        iq = mio_pubsub_set_stanza_new(conn, node);
        retract = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(retract, "retract");
        xmpp_stanza_set_attribute(retract, "node", node);
        remove_item = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(remove_item, "item");
        xmpp_stanza_set_attribute(remove_item, "id", "meta");
        xmpp_stanza_add_child(retract, remove_item);
        xmpp_stanza_add_child(iq->xmpp_stanza->children, retract);
        err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_error,
                                response);
        mio_response_free(query_response);
        mio_stanza_free(iq);

        // Update the references of referenced nodes to meta type unknown
        query_refs = mio_response_new();
        mio_references_query(conn, node, query_refs);
        if (query_refs->response_type == MIO_RESPONSE_PACKET) {
            packet = (mio_packet_t*) query_refs->response;
            if (packet->type == MIO_PACKET_REFERENCES) {
                ref = (mio_reference_t*) packet->payload;
                // Loop over all references of node, updating each reference meta type
                for (curr_ref = ref; curr_ref != NULL ;
                        curr_ref = curr_ref->next) {
                    pub_refs = mio_response_new();
                    if (curr_ref->type == MIO_REFERENCE_CHILD)
                        err = _mio_reference_meta_type_overwrite_publish(conn,
                                curr_ref->node, node, MIO_REFERENCE_PARENT,
                                MIO_META_TYPE_UKNOWN, pub_refs);
                    else if (curr_ref->type == MIO_REFERENCE_PARENT)
                        err = _mio_reference_meta_type_overwrite_publish(conn,
                                curr_ref->node, node, MIO_REFERENCE_CHILD,
                                MIO_META_TYPE_UKNOWN, pub_refs);
                    if (err != MIO_OK
                            && pub_refs->response_type != MIO_RESPONSE_OK)
                        mio_error(
                            "Unable to update meta_type reference of referenced node %s\n",
                            curr_ref->node);
                    mio_response_free(pub_refs);
                }
            } else
                mio_warn(
                    "Query references of node %s failed, check permissions",
                    node);
        } else
            mio_warn(
                "Query references of node %s failed, check if node exists permissions",
                node);
        mio_response_free(query_refs);
        return err;
    }

// Remove transducers
    if (transducer_names != NULL ) {
        for (i = 0; i < num_transducer_names; i++)
            query_meta->transducers = mio_meta_transducer_remove(
                                          query_meta->transducers, transducer_names[i]);
    }
// Remove properties
    if (property_names != NULL ) {
        for (i = 0; i < num_property_names; i++)
            query_meta->properties = mio_meta_property_remove(
                                         query_meta->properties, property_names[i]);
    }
// Convert meta data struct to stanza and publish
    item = mio_meta_to_item(conn, query_meta);
    err = mio_item_publish(conn, item, node, response);
// Cleanup
    mio_stanza_free(item);
    mio_response_free(query_response);
    return err;
}

/** Converts a mio_meta_t struct into a mio stanza.
 *
 * @param conn Active MIO connection.
 * @param meta An initialized meta struct to be parsed into a stanza.
 *
 * @returns Stanza with meta information contained within.
 * */
mio_stanza_t *mio_meta_to_item(mio_conn_t* conn, mio_meta_t *meta) {
    xmpp_stanza_t *meta_stanza = NULL, *transducer_stanza = NULL,
                   *property_stanza = NULL, *t_property_stanza = NULL, *enum_stanza =
                                          NULL;
    mio_transducer_meta_t *transducer;
    mio_enum_map_meta_t *enum_map;
    mio_property_meta_t *property, *t_property;
    mio_stanza_t *geoloc, *item;

    item = mio_pubsub_item_new(conn, "meta");

    meta_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(meta_stanza, "meta");
    xmpp_stanza_set_ns(meta_stanza, "http://jabber.org/protocol/mio");

    switch (meta->meta_type) {
    case MIO_META_TYPE_DEVICE:
        xmpp_stanza_set_attribute(meta_stanza, "type", "device");
        break;
    case MIO_META_TYPE_LOCATION:
        xmpp_stanza_set_attribute(meta_stanza, "type", "location");
        break;
    default:
        xmpp_stanza_release(meta_stanza);
        mio_error("Cannot generate item from unknown meta type\n");
        return NULL ;
    }
    if (meta->timestamp != NULL )
        xmpp_stanza_set_attribute(meta_stanza, "timestamp", meta->timestamp);
    if (meta->info != NULL )
        xmpp_stanza_set_attribute(meta_stanza, "info", meta->info);
    if (meta->geoloc != NULL ) {
        geoloc = mio_geoloc_to_stanza(conn, meta->geoloc);
        xmpp_stanza_add_child(meta_stanza, geoloc->xmpp_stanza);
        mio_stanza_free(geoloc);
    }
    if (meta->name != NULL )
        xmpp_stanza_set_attribute(meta_stanza, "name", meta->name);

    for (transducer = meta->transducers; transducer != NULL ; transducer =
                transducer->next) {
        transducer_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
        xmpp_stanza_set_name(transducer_stanza, "transducer");

        if (transducer->accuracy != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "accuracy",
                                      transducer->accuracy);
        if (transducer->geoloc != NULL ) {
            geoloc = mio_geoloc_to_stanza(conn, transducer->geoloc);
            xmpp_stanza_add_child(transducer_stanza, geoloc->xmpp_stanza);
            mio_stanza_free(geoloc);
        }
        if (transducer->interface != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "interface",
                                      transducer->interface);
        if (transducer->manufacturer != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "manufacturer",
                                      transducer->interface);
        if (transducer->manufacturer != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "manufacturer",
                                      transducer->manufacturer);
        if (transducer->max_value != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "maxValue",
                                      transducer->max_value);
        if (transducer->min_value != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "minValue",
                                      transducer->min_value);
        if (transducer->name != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "name",
                                      transducer->name);
        if (transducer->precision != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "precision",
                                      transducer->precision);
        if (transducer->resolution != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "resolution",
                                      transducer->resolution);
        if (transducer->serial != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "serial",
                                      transducer->serial);
        if (transducer->type != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "type",
                                      transducer->type);
        if (transducer->unit != NULL )
            xmpp_stanza_set_attribute(transducer_stanza, "unit",
                                      transducer->unit);
        if (transducer->enumeration != NULL ) {
            for (enum_map = transducer->enumeration; enum_map != NULL ;
                    enum_map = enum_map->next) {
                enum_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
                xmpp_stanza_set_name(enum_stanza, "map");
                if (enum_map->name != NULL )
                    xmpp_stanza_set_attribute(enum_stanza, "name",
                                              enum_map->name);
                if (enum_map->value != NULL )
                    xmpp_stanza_set_attribute(enum_stanza, "value",
                                              enum_map->value);
                xmpp_stanza_add_child(transducer_stanza, enum_stanza);
                xmpp_stanza_release(enum_stanza);
            }
        }
        if (transducer->properties != NULL ) {
            for (t_property = transducer->properties; t_property != NULL ;
                    t_property = t_property->next) {
                t_property_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
                xmpp_stanza_set_name(t_property_stanza, "property");
                if (t_property->name != NULL )
                    xmpp_stanza_set_attribute(t_property_stanza, "name",
                                              t_property->name);
                if (t_property->value != NULL )
                    xmpp_stanza_set_attribute(t_property_stanza, "value",
                                              t_property->value);
                xmpp_stanza_add_child(transducer_stanza, t_property_stanza);
                xmpp_stanza_release(t_property_stanza);
            }
        }

        xmpp_stanza_add_child(meta_stanza, transducer_stanza);
        xmpp_stanza_release(transducer_stanza);
    }

    for (property = meta->properties; property != NULL ;
            property = property->next) {

        if (property->name != NULL && property->value != NULL ) {
            property_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
            xmpp_stanza_set_name(property_stanza, "property");
            xmpp_stanza_set_attribute(property_stanza, "name", property->name);
            xmpp_stanza_set_attribute(property_stanza, "value",
                                      property->value);
            xmpp_stanza_add_child(meta_stanza, property_stanza);
            xmpp_stanza_release(property_stanza);
        }
    }
    xmpp_stanza_add_child(item->xmpp_stanza, meta_stanza);
    return item;
}

/** Gets the meta information of an event node.
 *
 * @param conn Active MIO connection.
 * @param node Node id of node to query for meta.
 * @param response Pointer to response struct to store response in.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNECTED on disconnection.
 * */
int mio_meta_query(mio_conn_t* conn, const char *node, mio_response_t *response) {
    mio_stanza_t *iq = NULL;
    xmpp_stanza_t *items = NULL, *item = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process meta query request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    iq = mio_pubsub_get_stanza_new(conn, node);

// Create items stanza
    items = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(items, "items");
    xmpp_stanza_set_attribute(items, "node", node);

// Create item stanza
    item = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(item, "item");
    xmpp_stanza_set_attribute(item, "id", "meta");

// Build xmpp message
    xmpp_stanza_add_child(items, item);
    xmpp_stanza_add_child(iq->xmpp_stanza->children, items);

// Send out the stanza
    err = mio_send_blocking(conn, iq, (mio_handler) mio_handler_meta_query,
                            response);

// Release the stanzas
    xmpp_stanza_release(items);
    xmpp_stanza_release(item);
    mio_stanza_free(iq);

    return err;
}

/** Publishes meta information to an event node.
 *
 * @param conn Active MIO connection.
 * @param node Event node id to publish meta information to.
 * @param meta The meta information structure for the event node.
 * @param response Pointer to the response received in acknowledgement of the publish request.
 *
 * @returns MIO_OK on success, MIO_ERROR_DISCONNEDTED on disconnection.
 * */
int mio_meta_publish(mio_conn_t* conn, const char *node, mio_meta_t *meta,
                     mio_response_t *response) {

    mio_stanza_t *meta_stanza = NULL;
    int err;

// Check if connection is active
    if (!conn->xmpp_conn->authenticated) {
        mio_error(
            "Cannot process meta publish request since not connected to XMPP server");
        return MIO_ERROR_DISCONNECTED;
    }

    meta_stanza = mio_meta_to_item(conn, meta);
    err = mio_item_publish(conn, meta_stanza, node, response);
    mio_stanza_free(meta_stanza);
    return err;
}

void XMLCALL mio_XMLstart_pubsub_meta_receive(void *data,
        const char *element_name, const char **attr) {

    int i = 0;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    mio_meta_t *mio_meta = (mio_meta_t*) packet->payload;
    mio_transducer_meta_t *t_meta = NULL;
    mio_enum_map_meta_t *e_meta = NULL;
    mio_property_meta_t *p_meta = NULL, *p_tail = NULL;
    mio_geoloc_t *geoloc = NULL;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "items") == 0) {
        mio_meta = mio_meta_new();
        packet->payload = mio_meta;
        mio_meta->meta_type = MIO_META_TYPE_DEVICE;
        packet->type = MIO_PACKET_META;
        packet->num_payloads++;
        response->response_type = MIO_RESPONSE_PACKET;
    }

    if (strcmp(element_name, "meta") == 0) {
        XML_SetCharacterDataHandler(*xml_data->parser, NULL );
        xml_data->payload = mio_meta;
        for (i = 0; attr[i]; i += 2) {
            xml_data->curr_attr_name = attr[i];
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "name") == 0) {
                mio_meta->name = malloc(strlen(attr[i + 1]) + 1);
                strcpy(mio_meta->name, attr[i + 1]);
            } else if (strcmp(attr_name, "timestamp") == 0) {
                mio_meta->timestamp = malloc(strlen(attr[i + 1]) + 1);
                strcpy(mio_meta->timestamp, attr[i + 1]);
            } else if (strcmp(attr_name, "info") == 0) {
                mio_meta->info = malloc(strlen(attr[i + 1]) + 1);
                strcpy(mio_meta->info, attr[i + 1]);
            } else if (strcmp(attr_name, "type") == 0) {
                if (strcmp(attr[i + 1], "location") == 0)
                    mio_meta->meta_type = MIO_META_TYPE_LOCATION;
                if (strcmp(attr[i + 1], "device") == 0)
                    mio_meta->meta_type = MIO_META_TYPE_DEVICE;
            }
        }
    } else if (strcmp(element_name, "transducer") == 0) {
        XML_SetCharacterDataHandler(*xml_data->parser, NULL );
        xml_data->curr_attr_name = attr[i];
        t_meta = mio_transducer_meta_new();
        xml_data->payload = t_meta;
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "name") == 0) {
                t_meta->name = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->name, attr[i + 1]);
            } else if (strcmp(attr_name, "type") == 0) {
                t_meta->type = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->type, attr[i + 1]);
            } else if (strcmp(attr_name, "interface") == 0) {
                t_meta->interface = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->interface, attr[i + 1]);
            } else if (strcmp(attr_name, "unit") == 0) {
                t_meta->unit = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->unit, attr[i + 1]);
            } else if (strcmp(attr_name, "manufacturer") == 0) {
                t_meta->manufacturer = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->manufacturer, attr[i + 1]);
            } else if (strcmp(attr_name, "serial") == 0) {
                t_meta->serial = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->serial, attr[i + 1]);
            } else if (strcmp(attr_name, "info") == 0) {
                t_meta->info = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->info, attr[i + 1]);
            } else if (strcmp(attr_name, "minValue") == 0) {
                t_meta->min_value = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->min_value, attr[i + 1]);
            } else if (strcmp(attr_name, "maxValue") == 0) {
                t_meta->max_value = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->max_value, attr[i + 1]);
            } else if (strcmp(attr_name, "resolution") == 0) {
                t_meta->resolution = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->resolution, attr[i + 1]);
            } else if (strcmp(attr_name, "precision") == 0) {
                t_meta->precision = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->precision, attr[i + 1]);
            } else if (strcmp(attr_name, "accuracy") == 0) {
                t_meta->accuracy = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->accuracy, attr[i + 1]);
            } else if (strcmp(attr_name, "interface") == 0) {
                t_meta->interface = malloc(strlen(attr[i + 1]) + 1);
                strcpy(t_meta->interface, attr[i + 1]);
            }
        }
        if (mio_meta->meta_type != MIO_META_TYPE_UKNOWN)
            mio_transducer_meta_add(mio_meta, t_meta);
    } else if (strcmp(element_name, "map") == 0) {
        XML_SetCharacterDataHandler(*xml_data->parser, NULL );
        xml_data->curr_attr_name = attr[i];
        t_meta = mio_transducer_meta_tail_get(mio_meta->transducers);
        if (t_meta->enumeration == NULL ) {
            t_meta->enumeration = mio_enum_map_meta_new();
            e_meta = t_meta->enumeration;
        } else {
            e_meta = mio_enum_map_meta_tail_get(t_meta->enumeration);
            e_meta->next = mio_enum_map_meta_new();
            e_meta = e_meta->next;
        }
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "name") == 0) {
                e_meta->name = malloc(strlen(attr[i + 1]) + 1);
                strcpy(e_meta->name, attr[i + 1]);
            } else if (strcmp(attr_name, "value") == 0) {
                e_meta->value = malloc(strlen(attr[i + 1]) + 1);
                strcpy(e_meta->value, attr[i + 1]);
            }
        }
    } else if (strcmp(element_name, "property") == 0) {
        XML_SetCharacterDataHandler(*xml_data->parser, NULL );
        p_meta = mio_property_meta_new();
        if (xml_data->parent != NULL ) {
            if (strcmp(xml_data->parent->parent, "transducer") == 0) {
                t_meta = (mio_transducer_meta_t*) xml_data->payload;
                p_tail = mio_property_meta_tail_get(t_meta->properties);
                if (p_tail == NULL )
                    t_meta->properties = p_meta;
                else
                    p_tail->next = p_meta;
            } else {
                p_tail = mio_property_meta_tail_get(mio_meta->properties);
                if (p_tail == NULL )
                    mio_meta->properties = p_meta;
                else
                    p_tail->next = p_meta;
                //mio_meta->properties = p_meta;
            }
        }
        xml_data->curr_attr_name = attr[i];
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "name") == 0) {
                p_meta->name = malloc(strlen(attr[i + 1]) + 1);
                strcpy(p_meta->name, attr[i + 1]);
            } else if (strcmp(attr_name, "value") == 0) {
                p_meta->value = malloc(strlen(attr[i + 1]) + 1);
                strcpy(p_meta->value, attr[i + 1]);
            }
        }
        //	if (p_meta != NULL )
        //	mio_property_meta_add(mio_meta, p_meta);
    } else if (strcmp(element_name, "geoloc") == 0) {
        geoloc = mio_geoloc_new();
        if (xml_data->prev_element_name != NULL ) {
            if (strcmp(xml_data->prev_element_name, "transducer") == 0) {
                t_meta = (mio_transducer_meta_t*) xml_data->payload;
                t_meta->geoloc = geoloc;
            } else {
                mio_meta = (mio_meta_t*) xml_data->payload;
                mio_meta->geoloc = geoloc;
            }
        }
        xml_data->payload = geoloc;
        XML_SetCharacterDataHandler(*xml_data->parser, mio_XMLString_geoloc);
    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);

}

static void XMLCALL mio_XMLstart_node_type_query(void *data,
        const char *element_name, const char **attr) {

    int i;
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_response_t *response = (mio_response_t*) xml_data->response;
    mio_packet_t *packet = (mio_packet_t*) response->response;
    mio_node_type_t *type;
    _mio_xml_data_update_start_element(xml_data, element_name);

    if (strcmp(element_name, "identity") == 0) {
        for (i = 0; attr[i]; i += 2) {
            const char* attr_name = attr[i];
            if (strcmp(attr_name, "type") == 0) {
                response->response_type = MIO_RESPONSE_PACKET;
                packet->type = MIO_PACKET_NODE_TYPE;
                packet->num_payloads = 1;
                packet->payload = malloc(sizeof(mio_node_type_t));
                type = (mio_node_type_t*) packet->payload;
                if (strcmp(attr[i + 1], "collection") == 0)
                    *type = MIO_NODE_TYPE_COLLECTION;
                else if (strcmp(attr[i + 1], "leaf") == 0)
                    *type = MIO_NODE_TYPE_LEAF;
                else
                    *type = MIO_NODE_TYPE_UNKNOWN;
            }
        }
    } else if (strcmp(element_name, "error") == 0)
        mio_XML_error_handler(element_name, attr, response);
}

int mio_handler_meta_query(mio_conn_t * const conn, mio_stanza_t * const stanza,
                           mio_response_t *response, void *userdata) {
    mio_stanza_t *stanza_copy;
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
    int err = mio_xml_parse(conn, stanza, xml_data,
                            mio_XMLstart_pubsub_meta_receive, mio_XMLString_geoloc);

    if (err == MIO_OK) {
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);
    }

    return err;
}

int mio_handler_node_type_query(mio_conn_t * const conn,
                                mio_stanza_t * const stanza, mio_response_t *response, void *userdata) {
    mio_stanza_t *stanza_copy;
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
    int err = mio_xml_parse(conn, stanza, xml_data,
                            mio_XMLstart_node_type_query, NULL );

    if (err == MIO_OK)
        mio_cond_signal(&request->cond, &request->mutex, &request->predicate);

    return err;
}


