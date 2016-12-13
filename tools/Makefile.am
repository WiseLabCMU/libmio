miodir = $(includedir)
bin_PROGRAMS = mio_acl mio_actuate mio_authenticate mio_collection mio_item_query mio_meta mio_node mio_password_change mio_publish_data mio_reference mio_subscriptions  
LDADD = ../src/libmio.a ../libs/libstrophe/libstrophe.a

mio_acl_SOURCES = mio_acl.c
mio_acl_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src/ -Wall
mio_acl_LDFLAGS = -L../src/ -L../libs/libstrophe -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio 

mio_acl_ARFLAGS = rcs
#
mio_actuate_SOURCES = mio_actuate.c
mio_actuate_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_actuate_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_actuate_ARFLAGS = rcs
#
mio_authenticate_SOURCES = mio_authenticate.c
mio_authenticate_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_authenticate_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_authenticate_ARFLAGS = rcs
#
mio_collection_SOURCES = mio_collection.c
mio_collection_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_collection_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_collection_ARFLAGS = rcs
#
mio_item_query_SOURCES = mio_item_query.c
mio_item_query_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_item_query_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_item_query_ARFLAGS = rcs
#
mio_meta_SOURCES = mio_meta.c
mio_meta_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_meta_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_meta_ARFLAGS = rcs
#
mio_node_SOURCES = mio_node.c
mio_node_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_node_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_node_ARFLAGS = rcs
#
mio_password_change_SOURCES = mio_password_change.c
mio_password_change_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_password_change_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_password_change_ARFLAGS = rcs
#
mio_publish_data_SOURCES = mio_publish_data.c
mio_publish_data_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_publish_data_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_publish_data_ARFLAGS = rcs
#
mio_reference_SOURCES = mio_reference.c
mio_reference_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_reference_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_reference_ARFLAGS = rcs
#
mio_subscriptions_SOURCES = mio_subscriptions.c
mio_subscriptions_CPPFLAGS = -I../libs/libstrophe/ -I../libs/libstrophe/src/ -I../src -Wall -g3
mio_subscriptions_LDFLAGS = -L ../src/ -lstrophe -lexpat -lssl -lpthread -luuid -lresolv -lmio
mio_subscriptions_ARFLAGS = rcs
