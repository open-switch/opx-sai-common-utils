LIBRARIES=sai-common-utils

SWITCHINFRA_SRCS:=$(wildcard switchinfra/*.c)
PORT_SRCS:= $(wildcard port/*.c)
ROUTING_SRCS:=$(wildcard routing/*.c)
SWITCHING_SRCS:=$(wildcard switching/*.c)
QOS_SRCS:=$(wildcard qos/*.c)
ACL_SRCS:=$(wildcard acl/*.c)
TUNNEL_SRCS:=$(wildcard tunnel/*.c)

sai-common-utils_SRCS= sai_map_utl.cpp sai_gen_utils.c ${SWITCHINFRA_SRCS} ${PORT_SRCS} ${ROUTING_SRCS} ${SWITCHING_SRCS} ${ACL_SRCS}
sai-common-utils_SRCS+= ${QOS_SRCS}
sai-common-utils_SRCS+= ${TUNNEL_SRCS}

sai-common-utils_LDFLAGS+= -ldn_common -levent_log
CXXFLAGS+=-std=c++11

include ${MAKE_INC}/workspace.mak
