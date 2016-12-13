#libmio

This repository holds the c libmio library as 
well as command line tools that are a useful demonstrations of 
the library. These tools also expose most Sensor Andrew functionality.
More information about this platfor cand be found on the 
[documentation page](http://dev.mortr.io/projects/mortar-io/wiki/Documentation).

## Installation

This section will guide you through the installation of libmio and mio tools.

### Requirements

The following dependencies exist for 

* gcc
* autotools
* autoconf
* automake
* libexpat
* libuuid
* openssl
* Doxygen (optional)

These are all available for Mac, Windows, and Linux. Installation of each will depend on
the operating system and distribution you use. The following works for Debian based distributions
with aptitude. To configure autotools follow the guide on their [site](https://autotools.io/index.html).

```bash
apt-get install libssl-dev libexpat1-dev autotools-dev automake autoconf uuid-dev  
```

### libstrophe

libmio uses libstrophe to manage XMPP connections and to receive and send stanzas. The libstrophe distribution 
comes with libmio and is found in the `libmio/libs/libstrophe` directory. To build make sure that the aforementioned
dependencies are available on your system. Then use the autotools build process to install. 

```bash
cd libmio/libs/libstrophe
./bootstrap.sh
./configure
make && sudo make install
```

After this you are ready to install libmio and mio-tools.


### libmio and tools

This takes on a similar build process to that of libstrophe.

```bash
cd libmio/
./bootstrap.sh
./configure
make && sudo make install
```

After this process is done, you are ready to test the installation.

### Test installation

A simple way to test whether or not the installation was succesful 
is to run mio_authenticate using a pre-existing account on the XMPP webserver
for more information on how to register users visit the 
[documentation page](http://dev.mortr.io/projects/mortar-io/wiki/Documentation)


## Usage

Here is some basic information about the library and tools.

### libmio

To build detailed documentation first install [doxygen](doxygen.org). Then run 
the following commands.

```bash
cd libmio/doxygen
doxygen
```

### mio-tools

Here is a list of mio tools and their usage.

#### mio_acl

```bash
"./mio_acl" Command line utility to interact with event node affilations.
Usage: ./mio_acl <-j jid> <-p password (optional)> <-c command> <-j jid> <-event event_node> [-verbose] [-stanza]  
	-event event_node = name of node to query
	-j jid = JID (give the full JabberID, i.e. user@domain)
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
	-server = server where event node resides, if not same as jid
	-stanza = only print the affiliations stanza

Commands:
	-r = remove
	-a = add
	-q = query

Options:
	-affil [affiliaiton] = affiliation, publisher, outcast, none, onlypublish, owner, member
	-group [group jid] = group to add to or remove from
	-event [event id] = event node id
	-u [user jid] = user to add or remove from acl
  ```

#### mio_actuate

```bash
Usage: ./mio_actuate <-event event_node> <-value transducer_typed_value> <-u username> <-p password> [-int interface] [-id transducer_id] [-rawvalue transducer_raw_value] [-verbose]
Usage: ./mio_actuate -help
	-event event_node = name of event node to publish to
	-value transducer_value = typed value to actuate
	-j username = JID (give the full JID, i.e. user@domain)
	-p password = JID user password
	-server = server where event node resides, if not same as jid
	-id transducer_id = id (name) of transducer within message
	-help = print this usage and exit
	-verbose = print info
```

#### mio_authenticate

```bash
Usage: mio_authenticate <-j username> <-p password> [-verbose]
Usage: mio_authenticate -help
	-j username = JID to authenticate (give the full JID, i.e. user@domain)
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
```

#### mio_collection

```bash
Usage: mio_collection <-collection collection_node> <-j username> <-p password> [-verbose] -a -r 
Usage: mio_collection -help
	-collection collection_node = id of collection node to query
	-child = id of node to add or remove to collection
	-parent = id of node of parent collection
	-j username = JID (give the full JID, i.e. user@domain)
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
	-title = name of collection node when creating
	-acm = access control model of collection node
	-a = add child node with id child id 
	-c = create collection node with collection id
	-r = remove child node with id child id
	-q = query collection node with collection id
```


#### mio_item_query

```bash
Usage: ./mio_item_query <-event event_node> [-max max_items] [-id item_id] <-u username> <-p password> [-verbose]
Usage: ./mio_item_query -help
	-event event_node = name of node to get data from
	-u username = JID (give the full JID, i.e. user@domain)
	-p password = JID user password
	-max max_items = maximum number of items to retrieve if no item_id is specified
	-id item_id = item id of the item to be retrieved
	-help = print this usage and exit
	-verbose = print info
```


#### mio_meta

```bash
Usage: mio_meta <-event event_node> <-name meta_name> <-type meta_type> [-info meta_info] <-j username> <-p password> [-verbose]
Usage: mio_meta -help
	-event event_node = name of event node to publish to
	-name meta_name = name of meta information to add
	-type meta_type = type of meta information to add: either "device" or "location"
	-info meta_info = description of the meta information to add
	-j username = JID (give the full JID, i.e. user@domain)
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
	-a = adds meta information if not already present.
	-c = change meta information, creates if no meta information present
	-q = query meta information
	-cop = copies meta information from template.
	-template = query meta information
	-unit transducer_unit = units of output of transducer to add
	-enum_names enumeration_unit_names = comma separated names of enum unit values e.g. "red,green,blue"
	-enum_names enumeration_unit_values = comma separated values of enum unit values e.g. "0,1,2"
	-min transducer_min_value = minimum output value of transducer to add
	-min transducer_max_value = maximum output value of transducer to add
	-resolution transducer_resolution = resolution of output of transducer to add
	-precision transducer_precision = precision of output of transducer to add
	-accuracy transducer_precision = accuracy of output of transducer to add
	-serial transducer_serial_number = serial number of the transducer to add
	-manufacturer transducer_manufacturer = manufacturer of the transducer to add
	-info trans_info = description of the transducer to add
	-interface trans_interface = interface of the transducer to add
	-accuracy accuracy = horizontal GPS error in meters (decimal) e.g. "10"
	-alt altitude = altitude in meters above or below sea level (decimal) e.g. "1609"
	-area area = a named area such as a campus or neighborhood e.g. "Central Park"
	-bearing bearing = GPS bearing (direction in which the entity is heading to reach its next waypoint), measured in decimal degrees relative to true north (decimal) e.g. "-07:00"
	-building building = a specific building on a street or in an area e.g. "The Empire State Building"
	-country country = the nation where the device/transducer is located e.g. "United States"
	-country_code country_code = the ISO 3166 two-letter country code e.g. "US"
	-datum datum = GPS datum e.g. "-07:00"
	-description description = a natural-language name for or description of the location e.g. "Bill's house"
	-floor floor = a particular floor in a building e.g. "102"
	-lat latitude = latitude in decimal degrees north (decimal) e.g. "39.75"
	-locality locality = a locality within the administrative region, such as a town or city e.g. "New York City"
	-lon longitude = longitude in decimal degrees East (decimal) e.g. "-104.99"
	-zip zip_code = a code used for postal delivery e.g. "10118"
	-region region = an administrative region of the nation, such as a state or province e.g. "New York"
	-room room = a particular room in a building e.g. "Observatory"
	-speed speed = the speed at which the device/transducer is moving, in meters per second (decimal) e.g. "52.69"
	-street street = a thoroughfare within the locality, or a crossing of two thoroughfares e.g. "350 Fifth Avenue / 34th and Broadway"
	-text text = a catch-all element that captures any other information about the location e.g. "Northwest corner of the lobby"
	-tzo time_zone_offset = the time zone offset from UTC for the current location e.g. "-07:00"
	-uri uri = a URI or URL pointing to a map of the location e.g. "http://www.esbnyc.com/map.jpg"
```

#### mio_node

```bash
Usage: mio_node <-event event_node> [-title node_title] [-access_model access_model_model] <-j username> <-p password> [-verbose]
Usage: mio_node -help
	-event event_node = name of event node to create
	-r = remove event node
	-title node_title = title of event node
	-access access_model = access model of the node, either "open", "whitelist", "presence" or "roster"
	-j username = JID (give the full JID, i.e. user@domain)
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
```

#### mio_password_change

```bash
"mio_password_change" Command line utility to change the password of a JID
Usage: mio_password_change <-np new_password> <-j username> <-p old_password> [-verbose]
Usage: mio_password_change -help
	-np new_password = new password to set
	-j username = JID (give the full JID, i.e. user@domain)
	-p old_password = Current JID user password
	-help = print this usage and exit
	-verbose = print info
  ```

#### mio_publish_data

```bash
Usage: mio_publish_data <-event event_node> <-id transducer_reg_id> <-value transducer_typed_value>  <-u username> <-p password> [-verbose]
Usage: mio_publish_data -help
	-event event_node = name of event node to publish to
	-id transducer_id = id (name) of transducer within message
	-value transducer_typed_value = typed value to publish
	-u username = JID (give the full JID, i.e. user@domain)
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
```


#### mio_reference

```bash
Usage: mio_reference <-parent parent_event_node> <-child child_event_node> <-child_type child_node_type> <-parent_type parent_node_type> [-add_ref_child] <-u username> <-p password> [-verbose]
Usage: mio_reference -help
	-parent parent_event_node = name of parent event node to add to child
	-a = add reference
	-r = remove reference
	-q = query references
	-child child_event_node = name of child event node to add to parent
	-add_ref_child = adds a reference to the parent at the child node if possible
	-u username = JID (give the full JID, i.e. user@domain)
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
```

#### mio_subscriptions

```bash
"./mio_subscriptions" Command line utility to subscribe to an event node
Usage: ./mio_subscriptions <-event event_node> <-j username> <-p password> [-verbose]
Usage: ./mio_subscriptions -help
	-event event_node = name of node to add subscriber to
	-j username = JID (give the full JID, i.e. user@domain)
	-q = query subscriptions
	-a = add subscription
	-r = remove subscription
	-l = listen to subscription messages
	-p password = JID user password
	-help = print this usage and exit
	-verbose = print info
```
