ACLOCAL_AMFLAGS=-I m4
AUTOMAKE_OPTIONS=subdir-objects
SUBDIRS = inc

lib_LTLIBRARIES = libsai-common-utils.la

libsai_common_utils_la_SOURCES = \
src/sai_gen_utils.c src/switchinfra/sai_switch_debug.c \
src/switchinfra/sai_switch_utils.c \
src/port/sai_port_attributes.c src/port/sai_port_debug.c \
src/port/sai_port_utils.c src/routing/sai_l3_debug.c  \
src/routing/sai_l3_init.c src/routing/sai_l3_util.c \
src/switching/sai_fdb_debug.c src/switching/sai_fdb_utils.c \
src/switching/sai_lag_debug.c src/switching/sai_lag_utils.c \
src/switching/sai_vlan_debug.c src/switching/sai_vlan_utils.c  \
src/qos/sai_qos_buffer_util.c src/qos/sai_qos_debug.c \
src/qos/sai_qos_maps_debug.c src/qos/sai_qos_policer_debugs.c \
src/qos/sai_qos_util.c src/qos/sai_qos_wred_debugs.c \
src/acl/sai_acl_utils.c src/tunnel/sai_tunnel_utils.c


libsai_common_utils_la_CFLAGS= -I$(top_srcdir)/inc/opx -I$(includedir)/opx
libsai_common_utils_la_LDFLAGS= -shared -version-info 1:1:0
libsai_common_utils_la_LIBADD= -lopx_common -lopx_logging -lpthread


