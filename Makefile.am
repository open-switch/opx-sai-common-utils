ACLOCAL_AMFLAGS=-I m4
AUTOMAKE_OPTIONS=subdir-objects
SUBDIRS = inc

lib_LTLIBRARIES = libopx_sai_common_utils.la

libopx_sai_common_utils_la_SOURCES = \
src/sai_gen_utils.c src/sai_map_utl.cpp \
src/acl/sai_acl_utils.c \
src/port/sai_port_attributes.c src/port/sai_port_debug.c \
src/port/sai_port_utils.c \
src/qos/sai_qos_buffer_util.c src/qos/sai_qos_util.c \
src/routing/sai_l3_debug.c  \
src/routing/sai_l3_init.c src/routing/sai_l3_util.c \
src/switchinfra/sai_switch_debug.c \
src/switchinfra/sai_switch_utils.c \
src/switching/sai_fdb_debug.c src/switching/sai_fdb_utils.c \
src/switching/sai_lag_utils.c src/switching/sai_vlan_utils.c  \
src/tunnel/sai_tunnel_utils.c


libopx_sai_common_utils_la_CFLAGS= -I$(top_srcdir)/inc/opx -I$(includedir)/opx
libopx_sai_common_utils_la_LDFLAGS= -shared -version-info 1:1:0
libopx_sai_common_utils_la_CPPFLAGS= -I$(top_srcdir)/inc/opx -I$(includedir)/opx -std=c++11
libopx_sai_common_utils_la_LIBADD= -lopx_common -lopx_logging -lpthread


