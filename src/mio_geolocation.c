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

#include "mio_geolocation.h"

#include "mio_connection.h"
#include "mio_error.h"
#include "mio_handlers.h"
#include "mio_node.h"
#include <strophe.h>
#include <common.h>


void _mio_geoloc_element_add_double(mio_conn_t *conn,
                                    xmpp_stanza_t *geoloc_stanza, char* element_name, double* element_value);
/**
 * @ingroup Geolocation
 * Allocates and initializes a new mio geoloc
 *
 * @returns A pointer to the newly allocated and initialized mio geoloc.
 */
mio_geoloc_t *mio_geoloc_new() {
    mio_geoloc_t *geoloc = malloc(sizeof(mio_geoloc_t));
    memset(geoloc, 0, sizeof(mio_geoloc_t));
    return geoloc;
}

/**
 * @ingroup Geolocation
 * Frees a mio geoloc.
 *
 * @param A pointer to the allocated mio geoloc to be freed.
 */
void mio_geoloc_free(mio_geoloc_t *geoloc) {
    if (geoloc->accuracy != NULL )
        free(geoloc->accuracy);
    if (geoloc->alt != NULL )
        free(geoloc->alt);
    if (geoloc->area != NULL )
        free(geoloc->area);
    if (geoloc->bearing != NULL )
        free(geoloc->bearing);
    if (geoloc->building != NULL )
        free(geoloc->building);
    if (geoloc->country != NULL )
        free(geoloc->country);
    if (geoloc->country_code != NULL )
        free(geoloc->country_code);
    if (geoloc->datum != NULL )
        free(geoloc->datum);
    if (geoloc->description != NULL )
        free(geoloc->description);
    if (geoloc->floor != NULL )
        free(geoloc->floor);
    if (geoloc->lat != NULL )
        free(geoloc->lat);
    if (geoloc->locality != NULL )
        free(geoloc->locality);
    if (geoloc->lon != NULL )
        free(geoloc->lon);
    if (geoloc->postal_code != NULL )
        free(geoloc->postal_code);
    if (geoloc->region != NULL )
        free(geoloc->region);
    if (geoloc->room != NULL )
        free(geoloc->room);
    if (geoloc->speed != NULL )
        free(geoloc->speed);
    if (geoloc->street != NULL )
        free(geoloc->street);
    if (geoloc->text != NULL )
        free(geoloc->text);
    if (geoloc->timestamp != NULL )
        free(geoloc->timestamp);
    if (geoloc->tzo != NULL )
        free(geoloc->tzo);
    if (geoloc->uri != NULL )
        free(geoloc->uri);
    free(geoloc);
}

/**
 * @ingroup Core
 * Prints the data contained in a mio geoloc struct to stdout.
 *
 * @param geoloc A pointer to the mio_geoloc struct to be printed.
 * @param tabs A string containing the formatting to be prefixed to each line being printed e.g. "\t\t"
 */
void mio_geoloc_print(mio_geoloc_t *geoloc, char* tabs) {
    fprintf(stdout, "%sGeoloc:\n", tabs);
    if (geoloc->accuracy != NULL )
        fprintf(stdout, "%s\tAccuracy: %lf\n", tabs, *geoloc->accuracy);
    if (geoloc->alt != NULL )
        fprintf(stdout, "%s\tAltitude: %lf\n", tabs, *geoloc->alt);
    if (geoloc->area != NULL )
        fprintf(stdout, "%s\tArea: %s\n", tabs, geoloc->area);
    if (geoloc->bearing != NULL )
        fprintf(stdout, "%s\tBearing: %lf\n", tabs, *geoloc->bearing);
    if (geoloc->building != NULL )
        fprintf(stdout, "%s\tBuilding: %s\n", tabs, geoloc->building);
    if (geoloc->country != NULL )
        fprintf(stdout, "%s\tCountry: %s\n", tabs, geoloc->country);
    if (geoloc->country_code != NULL )
        fprintf(stdout, "%s\tCountry Code: %s\n", tabs, geoloc->country_code);
    if (geoloc->datum != NULL )
        fprintf(stdout, "%s\tDatum: %s\n", tabs, geoloc->datum);
    if (geoloc->description != NULL )
        fprintf(stdout, "%s\tDescription: %s\n", tabs, geoloc->description);
    if (geoloc->floor != NULL )
        fprintf(stdout, "%s\tFloor: %s\n", tabs, geoloc->floor);
    if (geoloc->lat != NULL )
        fprintf(stdout, "%s\tLatitude: %lf\n", tabs, *geoloc->lat);
    if (geoloc->lon != NULL )
        fprintf(stdout, "%s\tLongitude: %lf\n", tabs, *geoloc->lon);
    if (geoloc->locality != NULL )
        fprintf(stdout, "%s\tLocality: %s\n", tabs, geoloc->locality);
    if (geoloc->postal_code != NULL )
        fprintf(stdout, "%s\tPostal Code: %s\n", tabs, geoloc->postal_code);
    if (geoloc->region != NULL )
        fprintf(stdout, "%s\tRegion: %s\n", tabs, geoloc->region);
    if (geoloc->room != NULL )
        fprintf(stdout, "%s\tRoom: %s\n", tabs, geoloc->room);
    if (geoloc->speed != NULL )
        fprintf(stdout, "%s\tSpeed: %lf\n", tabs, *geoloc->speed);
    if (geoloc->street != NULL )
        fprintf(stdout, "%s\tStreet: %s\n", tabs, geoloc->street);
    if (geoloc->text != NULL )
        fprintf(stdout, "%s\tText: %s\n", tabs, geoloc->text);
    if (geoloc->timestamp != NULL )
        fprintf(stdout, "%s\tTimestamp: %s\n", tabs, geoloc->timestamp);
    if (geoloc->uri != NULL )
        fprintf(stdout, "%s\tURI: %s\n", tabs, geoloc->uri);
}


void _mio_geoloc_element_add_string(mio_conn_t *conn,
                                    xmpp_stanza_t *geoloc_stanza, char* element_name, char* element_text) {
    xmpp_stanza_t *element, *element_text_stanza;
    element = xmpp_stanza_new(conn->xmpp_conn->ctx);
    element_text_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(element, element_name);
    xmpp_stanza_set_text(element_text_stanza, element_text);
    xmpp_stanza_add_child(element, element_text_stanza);
    xmpp_stanza_add_child(geoloc_stanza, element);
    xmpp_stanza_release(element_text_stanza);
    xmpp_stanza_release(element);
}

void _mio_geoloc_element_add_double(mio_conn_t *conn,
                                    xmpp_stanza_t *geoloc_stanza, char* element_name, double* element_value) {
    xmpp_stanza_t *element, *element_text_stanza;
    char text[128];
    element = xmpp_stanza_new(conn->xmpp_conn->ctx);
    element_text_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
    snprintf(text, 128, "%lf", *element_value);
    xmpp_stanza_set_name(element, element_name);
    xmpp_stanza_set_text(element_text_stanza, text);
    xmpp_stanza_add_child(element, element_text_stanza);
    xmpp_stanza_add_child(geoloc_stanza, element);
    xmpp_stanza_release(element_text_stanza);
    xmpp_stanza_release(element);
}

void mio_geoloc_merge(mio_geoloc_t *geoloc_to_update, mio_geoloc_t *geoloc) {

    if (geoloc->accuracy != NULL ) {
        if (geoloc_to_update->accuracy != NULL )
            free(geoloc_to_update->accuracy);
        geoloc_to_update->accuracy = malloc(sizeof(double));
        *geoloc_to_update->accuracy = *geoloc->accuracy;
    }
    if (geoloc->alt != NULL ) {
        if (geoloc_to_update->alt != NULL )
            free(geoloc_to_update->alt);
        geoloc_to_update->alt = malloc(sizeof(double));
        *geoloc_to_update->alt = *geoloc->alt;
    }
    if (geoloc->area != NULL ) {
        if (geoloc_to_update->area != NULL )
            free(geoloc_to_update->area);
        geoloc_to_update->area = strdup(geoloc->area);
    }
    if (geoloc->bearing != NULL ) {
        if (geoloc_to_update->bearing != NULL )
            free(geoloc_to_update->bearing);
        geoloc_to_update->bearing = malloc(sizeof(double));
        *geoloc_to_update->bearing = *geoloc->bearing;
    }
    if (geoloc->building != NULL ) {
        if (geoloc_to_update->building != NULL )
            free(geoloc_to_update->building);
        geoloc_to_update->building = strdup(geoloc->building);
    }
    if (geoloc->country != NULL ) {
        if (geoloc_to_update->country != NULL )
            free(geoloc_to_update->country);
        geoloc_to_update->country = strdup(geoloc->country);
    }
    if (geoloc->country_code != NULL ) {
        if (geoloc_to_update->country_code != NULL )
            free(geoloc_to_update->country_code);
        geoloc_to_update->country_code = strdup(geoloc->country_code);
    }
    if (geoloc->datum != NULL ) {
        if (geoloc_to_update->datum != NULL )
            free(geoloc_to_update->datum);
        geoloc_to_update->datum = strdup(geoloc->datum);
    }
    if (geoloc->description != NULL ) {
        if (geoloc_to_update->description != NULL )
            free(geoloc_to_update->description);
        geoloc_to_update->description = strdup(geoloc->description);
    }
    if (geoloc->floor != NULL ) {
        if (geoloc_to_update->floor != NULL )
            free(geoloc_to_update->floor);
        geoloc_to_update->floor = strdup(geoloc->floor);
    }
    if (geoloc->lat != NULL ) {
        if (geoloc_to_update->lat != NULL )
            free(geoloc_to_update->lat);
        geoloc_to_update->lat = malloc(sizeof(double));
        *geoloc_to_update->lat = *geoloc->lat;
    }
    if (geoloc->locality != NULL ) {
        if (geoloc_to_update->locality != NULL )
            free(geoloc_to_update->locality);
        geoloc_to_update->locality = strdup(geoloc->locality);
    }
    if (geoloc->lon != NULL ) {
        if (geoloc_to_update->lon != NULL )
            free(geoloc_to_update->lon);
        geoloc_to_update->lon = malloc(sizeof(double));
        *geoloc_to_update->lon = *geoloc->lon;
    }
    if (geoloc->postal_code != NULL ) {
        if (geoloc_to_update->postal_code != NULL )
            free(geoloc_to_update->postal_code);
        geoloc_to_update->postal_code = strdup(geoloc->postal_code);
    }
    if (geoloc->region != NULL ) {
        if (geoloc_to_update->region != NULL )
            free(geoloc_to_update->region);
        geoloc_to_update->region = strdup(geoloc->region);
    }
    if (geoloc->room != NULL ) {
        if (geoloc_to_update->room != NULL )
            free(geoloc_to_update->room);
        geoloc_to_update->room = strdup(geoloc->room);
    }
    if (geoloc->speed != NULL ) {
        if (geoloc_to_update->speed != NULL )
            free(geoloc_to_update->speed);
        geoloc_to_update->speed = malloc(sizeof(double));
        *geoloc_to_update->speed = *geoloc->speed;
    }
    if (geoloc->street != NULL ) {
        if (geoloc_to_update->street != NULL )
            free(geoloc_to_update->street);
        geoloc_to_update->street = strdup(geoloc->street);
    }
    if (geoloc->text != NULL ) {
        if (geoloc_to_update->text != NULL )
            free(geoloc_to_update->text);
        geoloc_to_update->text = strdup(geoloc->text);
    }
    if (geoloc->tzo != NULL ) {
        if (geoloc_to_update->tzo != NULL )
            free(geoloc_to_update->tzo);
        geoloc_to_update->tzo = strdup(geoloc->tzo);
    }
    if (geoloc->timestamp != NULL ) {
        if (geoloc_to_update->timestamp != NULL )
            free(geoloc_to_update->timestamp);
        geoloc_to_update->timestamp = strdup(geoloc->timestamp);
    }
    if (geoloc->uri != NULL ) {
        if (geoloc_to_update->uri != NULL )
            free(geoloc_to_update->uri);
        geoloc_to_update->uri = strdup(geoloc->uri);
    }
}

/** Converts a mio_geloc_t struct into an XMPP stanza.
 *
 * @param conn Active MIO connection
 * @param geoloc Initialized geolocation structure.
 *
 * @returns Stanza with geolocation information.
 * */
mio_stanza_t *mio_geoloc_to_stanza(mio_conn_t* conn, mio_geoloc_t *geoloc) {
    xmpp_stanza_t *geoloc_stanza;
    mio_stanza_t *ret = mio_stanza_new(conn);

    geoloc_stanza = xmpp_stanza_new(conn->xmpp_conn->ctx);
    xmpp_stanza_set_name(geoloc_stanza, "geoloc");
    xmpp_stanza_set_ns(geoloc_stanza, "http://jabber.org/protocol/geoloc");
    xmpp_stanza_set_attribute(geoloc_stanza, "xml:lang", "en");

    if (geoloc->accuracy != NULL )
        _mio_geoloc_element_add_double(conn, geoloc_stanza, "accuracy",
                                       geoloc->accuracy);
    if (geoloc->alt != NULL )
        _mio_geoloc_element_add_double(conn, geoloc_stanza, "alt",
                                       geoloc->accuracy);
    if (geoloc->area != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "area",
                                       geoloc->area);
    if (geoloc->bearing != NULL )
        _mio_geoloc_element_add_double(conn, geoloc_stanza, "bearing",
                                       geoloc->bearing);
    if (geoloc->building != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "building",
                                       geoloc->building);
    if (geoloc->country != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "country",
                                       geoloc->country);
    if (geoloc->country_code != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "countrycode",
                                       geoloc->country_code);
    if (geoloc->datum != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "datum",
                                       geoloc->datum);
    if (geoloc->description != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "description",
                                       geoloc->description);
    if (geoloc->floor != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "floor",
                                       geoloc->floor);
    if (geoloc->lat != NULL )
        _mio_geoloc_element_add_double(conn, geoloc_stanza, "lat", geoloc->lat);
    if (geoloc->locality != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "locality",
                                       geoloc->locality);
    if (geoloc->lon != NULL )
        _mio_geoloc_element_add_double(conn, geoloc_stanza, "lon", geoloc->lon);
    if (geoloc->postal_code != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "postalcode",
                                       geoloc->postal_code);
    if (geoloc->region != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "region",
                                       geoloc->region);
    if (geoloc->room != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "room",
                                       geoloc->room);
    if (geoloc->speed != NULL )
        _mio_geoloc_element_add_double(conn, geoloc_stanza, "speed",
                                       geoloc->speed);
    if (geoloc->street != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "street",
                                       geoloc->street);
    if (geoloc->text != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "text",
                                       geoloc->text);
    if (geoloc->timestamp != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "timestamp",
                                       geoloc->timestamp);
    if (geoloc->tzo != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "tzo", geoloc->tzo);
    if (geoloc->uri != NULL )
        _mio_geoloc_element_add_string(conn, geoloc_stanza, "uri", geoloc->uri);

    ret->xmpp_stanza = geoloc_stanza;
    return ret;
}

void XMLCALL mio_XMLString_geoloc(void *data, const XML_Char *s, int len) {
    mio_xml_parser_data_t *xml_data = (mio_xml_parser_data_t*) data;
    mio_geoloc_t *geoloc = (mio_geoloc_t*) xml_data->payload;
    char format[32];

    if (strcmp(xml_data->curr_element_name, "accuracy")
            == 0&& geoloc->accuracy == NULL) {
        geoloc->accuracy = malloc(sizeof(double));
        snprintf(format, 32, "%%%ulf", len);
        sscanf(s, format, geoloc->accuracy);
    }
    if (strcmp(xml_data->curr_element_name, "alt") == 0 && geoloc->alt == NULL ) {
        geoloc->alt = malloc(sizeof(double));
        snprintf(format, 32, "%%%ulf", len);
        sscanf(s, format, geoloc->alt);
    }
    if (strcmp(xml_data->curr_element_name, "area")
            == 0&& geoloc->area == NULL) {
        geoloc->area = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->area, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "bearing")
            == 0&& geoloc->bearing == NULL) {
        geoloc->bearing = malloc(sizeof(double));
        snprintf(format, 32, "%%%ulf", len);
        sscanf(s, format, geoloc->bearing);
    }
    if (strcmp(xml_data->curr_element_name, "building")
            == 0&& geoloc->building == NULL) {
        geoloc->building = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->building, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "country")
            == 0&& geoloc->country == NULL) {
        geoloc->country = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->country, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "countrycode")
            == 0&& geoloc->country_code == NULL) {
        geoloc->country_code = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->country_code, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "datum")
            == 0&& geoloc->datum == NULL) {
        geoloc->datum = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->datum, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "description")
            == 0&& geoloc->description == NULL) {
        geoloc->description = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->description, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "floor")
            == 0&& geoloc->floor == NULL) {
        geoloc->floor = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->floor, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "lat") == 0 && geoloc->lat == NULL ) {
        geoloc->lat = malloc(sizeof(double));
        snprintf(format, 32, "%%%ulf", len);
        sscanf(s, format, geoloc->lat);
    }
    if (strcmp(xml_data->curr_element_name, "locality")
            == 0&& geoloc->locality == NULL) {
        geoloc->locality = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->locality, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "lon") == 0 && geoloc->lon == NULL ) {
        geoloc->lon = malloc(sizeof(double));
        snprintf(format, 32, "%%%ulf", len);
        sscanf(s, format, geoloc->lon);
    }
    if (strcmp(xml_data->curr_element_name, "postalcode")
            == 0&& geoloc->postal_code == NULL) {
        geoloc->postal_code = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->postal_code, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "region")
            == 0&& geoloc->region == NULL) {
        geoloc->region = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->region, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "room")
            == 0&& geoloc->room == NULL) {
        geoloc->room = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->room, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "speed")
            == 0&& geoloc->speed == NULL) {
        geoloc->speed = malloc(sizeof(double));
        snprintf(format, 32, "%%%ulf", len);
        sscanf(s, format, geoloc->speed);
    }
    if (strcmp(xml_data->curr_element_name, "street")
            == 0&& geoloc->street == NULL) {
        geoloc->street = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->street, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "text")
            == 0&& geoloc->text == NULL) {
        geoloc->text = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->text, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "timestamp")
            == 0&& geoloc->timestamp == NULL) {
        geoloc->timestamp = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->timestamp, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "tzo") == 0 && geoloc->tzo == NULL ) {
        geoloc->tzo = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->tzo, len + 1, "%s", s);
    }
    if (strcmp(xml_data->curr_element_name, "uri") == 0 && geoloc->uri == NULL ) {
        geoloc->uri = malloc(sizeof(char) * len + 1);
        snprintf(geoloc->uri, len + 1, "%s", s);
    }
}


