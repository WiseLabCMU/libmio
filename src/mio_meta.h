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

#ifndef _MIO_META_
#define _MIO_META_

#include <mio.h>
typedef enum {
    MIO_META_TYPE_UKNOWN, MIO_META_TYPE_DEVICE, MIO_META_TYPE_LOCATION, MIO_META_TYPE_GATEWAY,
    MIO_META_TYPE_ADAPTER, MIO_META_TYPE_AGENT
} mio_meta_type_t;

typedef struct mio_geoloc {
    double *accuracy;
    double *alt;
    char *area;
    double *bearing;
    char *building;
    char *country;
    char *country_code;
    char *datum;
    char *description;
    char *floor;
    double *lat;
    char *locality;
    double *lon;
    char *postal_code;
    char *region;
    char *room;
    double *speed;
    char *street;
    char *text;
    char *timestamp;
    char *tzo;
    char *uri;
} mio_geoloc_t;

typedef struct mio_enum_map_meta mio_enum_map_meta_t;

struct mio_enum_map_meta {
    char *name;
    char *value;
    mio_enum_map_meta_t *next;
};

typedef struct mio_property_meta mio_property_meta_t;

struct mio_property_meta {
    char* name;
    char* value;
    mio_property_meta_t *next;
};

typedef struct mio_transducer_meta mio_transducer_meta_t;

struct mio_transducer_meta {
    char *name;
    char *type;
    char *interface;
    char *manufacturer;
    char *serial;
    char *info;
    char *unit;
    char *min_value;
    char *max_value;
    char *resolution;
    char *precision;
    char *accuracy;
    mio_enum_map_meta_t *enumeration;
    mio_geoloc_t *geoloc;
    mio_property_meta_t *properties;
    mio_transducer_meta_t *next;
};

typedef enum {
    MIO_DEVICE_TYPE_INDOOR_WEATHER,
    MIO_DEVICE_TYPE_OUTDOOR_WEATHER,
    MIO_DEVICE_TYPE_HVAC,
    MIO_DEVICE_TYPE_OCCUPANCY,
    MIO_DEVICE_TYPE_MULTIMEDIA_INPUT,
    MIO_DEVICE_TYPE_MULTIMEDIA_OUTPUT,
    MIO_DEVICE_TYPE_SCALE,
    MIO_DEVICE_TYPE_VEHICLE,
    MIO_DEVICE_TYPE_RESOURCE_CONSUMPTION,
    MIO_DEVICE_TYPE_RESOURCE_GENERATION,
    MIO_DEVICE_TYPE_OTHER
} mio_device_type_t;

typedef enum {
    MIO_UNIT_METER,
    MIO_UNIT_GRAM,
    MIO_UNIT_SECOND,
    MIO_UNIT_AMPERE,
    MIO_UNIT_KELVIN,
    MIO_UNIT_MOLE,
    MIO_UNIT_CANDELA,
    MIO_UNIT_RADIAN,
    MIO_UNIT_STERADIAN,
    MIO_UNIT_HERTZ,
    MIO_UNIT_NEWTON,
    MIO_UNIT_PASCAL,
    MIO_UNIT_JOULE,
    MIO_UNIT_WATT,
    MIO_UNIT_COULOMB,
    MIO_UNIT_VOLT,
    MIO_UNIT_FARAD,
    MIO_UNIT_OHM,
    MIO_UNIT_SIEMENS,
    MIO_UNIT_WEBER,
    MIO_UNIT_TESLA,
    MIO_UNIT_HENRY,
    MIO_UNIT_LUMEN,
    MIO_UNIT_LUX,
    MIO_UNIT_BECQUEREL,
    MIO_UNIT_GRAY,
    MIO_UNIT_SIEVERT,
    MIO_UNIT_KATAL,
    MIO_UNIT_LITER,
    MIO_UNIT_SQUARE_METER,
    MIO_UNIT_CUBIC_METER,
    MIO_UNIT_METERS_PER_SECOND,
    MIO_UNIT_METERS_PER_SECOND_SQUARED,
    MIO_UNIT_RECIPROCAL_METER,
    MIO_UNIT_KILOGRAM_PER_CUBIC_METER,
    MIO_UNIT_CUBIC_METER_PER_KILOGRAM,
    MIO_UNIT_AMPERE_PER_SQUARE_METER,
    MIO_UNIT_AMPERE_PER_METER,
    MIO_UNIT_MOLE_PER_CUBIC_METER,
    MIO_UNIT_CANDELA_PER_SQUARE_METER,
    MIO_UNIT_KILOGRAM_PER_KILOGRAM,
    MIO_UNIT_VOLT_AMPERE_REACTIVE,
    MIO_UNIT_VOLT_AMPERE,
    MIO_UNIT_WATT_SECOND,
    MIO_UNIT_PERCENT,
    MIO_UNIT_ENUM,
    MIO_UNIT_LAT,
    MIO_UNIT_LON
} mio_unit_t;

typedef struct mio_meta {
    char *name;
    char *timestamp;
    char *info;
    mio_meta_type_t meta_type;
    mio_geoloc_t *geoloc;
    mio_transducer_meta_t *transducers;
    mio_property_meta_t *properties;
} mio_meta_t;

//transducer meta functions
mio_transducer_meta_t * mio_transducer_meta_new();
void mio_transducer_meta_free(mio_transducer_meta_t *t_meta);
mio_transducer_meta_t *mio_transducer_meta_tail_get(
    mio_transducer_meta_t *t_meta);
void mio_transducer_meta_add(mio_meta_t *meta, mio_transducer_meta_t *t_meta);

// property functions
mio_property_meta_t *mio_property_meta_tail_get(mio_property_meta_t *p_meta);

// enum functions
void mio_enum_map_meta_free(mio_enum_map_meta_t *e_meta);
mio_enum_map_meta_t * mio_enum_map_meta_new();
void mio_enum_map_meta_add(mio_transducer_meta_t *t_meta,
                           mio_enum_map_meta_t *e_meta);
mio_enum_map_meta_t *mio_enum_map_meta_tail_get(mio_enum_map_meta_t *e_meta);
mio_enum_map_meta_t *mio_enum_map_meta_clone(mio_enum_map_meta_t *e_meta);
mio_property_meta_t *mio_meta_property_remove(mio_property_meta_t *properties,
        char *property_name);
void mio_meta_transducer_merge(mio_transducer_meta_t *transducer_to_update,
                               mio_transducer_meta_t *transducer);
void mio_meta_property_merge(mio_property_meta_t *property_to_update,
                             mio_property_meta_t *property);

mio_transducer_meta_t *mio_meta_transducer_remove(
    mio_transducer_meta_t *transducers, char *transducer_name);
int mio_meta_publish(mio_conn_t* conn, const char *node, mio_meta_t *meta,
                     mio_response_t *response);
int mio_meta_merge_publish(mio_conn_t *conn, char *node, mio_meta_t *meta,
                           mio_transducer_meta_t *transducers, mio_property_meta_t *properties,
                           mio_response_t *response);
int mio_meta_remove_publish(mio_conn_t *conn, char *node, char *device_name,
                            char **transducer_names, int num_transducer_names,
                            char **property_names, int num_property_names, mio_response_t *response);

void XMLCALL mio_XMLstart_pubsub_meta_receive(void *data,
        const char *element_name, const char **attr);


// mio meta functions
mio_meta_t *mio_meta_new();
void mio_meta_merge(mio_meta_t *meta_to_update, mio_meta_t *meta);
void mio_meta_free(mio_meta_t * meta);
mio_stanza_t *mio_meta_to_item(mio_conn_t* conn, mio_meta_t *meta);
mio_property_meta_t *mio_property_meta_new();
void mio_property_meta_free(mio_property_meta_t *p_meta);
void mio_property_meta_add(mio_meta_t *meta, mio_property_meta_t *p_meta);
mio_property_meta_t *mio_property_meta_tail_get(mio_property_meta_t *p_meta);
int mio_meta_query(mio_conn_t* conn, const char *node, mio_response_t *response);
int mio_handler_node_type_query(mio_conn_t * const conn,
                                mio_stanza_t * const stanza, mio_response_t *response, void *userdata);
int mio_handler_meta_query(mio_conn_t * const conn, mio_stanza_t * const stanza,
                           mio_response_t *response, void *userdata);
int mio_handler_collection_parents_query(mio_conn_t * const conn,
        mio_stanza_t * const stanza, mio_response_t *response, void *userdata);


#endif
