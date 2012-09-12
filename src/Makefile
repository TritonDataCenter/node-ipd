#
# Copyright (c) 2012 Joyent, Inc.  All rights reserved.
#

PREFIX_NODE :=	$(shell dirname $$(bash -c 'hash node; hash -t node'))/..
V8PLUS :=       $(shell $(PREFIX_NODE)/bin/node -e 'require("v8plus");')

include $(V8PLUS)/Makefile.v8plus.defs

MODULE =	ipd_binding

SRCS =	\
		node_ipd.c

ERRNO_JSON =	errno.json

CC =		/opt/local/bin/gcc
CXX =		/opt/local/bin/g++

LIBS +=		-lipd

include $(V8PLUS)/Makefile.v8plus.targ
