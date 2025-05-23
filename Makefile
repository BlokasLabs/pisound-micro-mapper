# SPDX-License-Identifier: GPL-2.0-only
#
# pisound-micro-mapper - a daemon to facilitate Pisound Micro control mappings.
#  Copyright (C) 2023-2025  Vilniaus Blokas UAB, https://blokas.io/
#
# This file is part of pisound-micro-mapper.
#
# pisound-micro-mapper is free software: you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation, version 2.0 of the License.
#
# pisound-micro-mapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
# the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along with pisound-micro-mapper.
# If not, see <https://www.gnu.org/licenses/>.

PREFIX?=/usr/local

VERSION=1.0.0

BINDIR?=$(PREFIX)/bin

CC?=cc
AR?=ar

CFLAGS=-Itps/rapidjson/include
CFLAGS+=-MMD

INSTALL?=install
INSTALL_PROGRAM?=$(INSTALL)
INSTALL_DATA?=$(INSTALL) -m 644

CXXFLAGS=$(CFLAGS) -fno-rtti

ifneq ($(NO_LOCAL_CFG),yes)
	-include Makefile.local
endif

ifeq ($(DEBUG),yes)
	CFLAGS += -DDEBUG -g -O0
else
	CFLAGS += -O3
endif

all: pisound-micro-mapper pisound-micro-schema.json

%.o: %.c %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.c: schema/%.ts
	./gen_schema.sh $^ $@

SCHEMA_FILES = $(wildcard src/schema/*.ts)

pisound-micro-schema.json: $(SCHEMA_FILES)
	npx ts-json-schema-generator --minify --strict-tuples --path $< --no-top-ref --type pisound_micro_root -o $@

.PRECIOUS: %.c

OBJ = src/config_schema.o src/alsa_schema.o src/pisound_micro_schema.o src/midi_schema.o src/control-manager.o \
	src/osc_schema.o src/alsa-control-server.o src/main.o src/upisnd-control-server.o src/dtors.o src/config-loader.o \
	src/logger.o src/alsa-control-server-loader.o src/upisnd-control-server-loader.o src/midi-control-server.o \
	src/midi-control-server-loader.o src/osc-control-server.o src/osc-control-server-loader.o src/utils.o
DEP = $(OBJ:%.o=%.d)

pisound-micro-mapper: $(OBJ)
	$(CXX) $^ $(CFLAGS) -lpthread -lasound -llo $(shell pkg-config --libs libpisoundmicro) -o $@

-include $(DEP)

schema-test: src/schema-test.cpp
	$(CXX) $^ $(CFLAGS) -o $@

schema-check: src/config-schema.json schema-test
	./schema-test src/config-schema.json example.json

install: pisound-micro-mapper
	mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) pisound-micro-mapper $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(OBJ) $(DEP) src/*_schema.json src/*_schema.c pisound-micro-mapper pisound-micro-schema.json schema-test schema-test.d

dist:
	git archive --prefix "pisound-micro-mapper-$(VERSION)/" -o "pisound-micro-mapper-$(VERSION).tar" HEAD
	git submodule foreach --recursive "git archive --prefix=pisound-micro-mapper-$(VERSION)/\$$path/ --output=\$$sha1.tar HEAD && tar --concatenate --file=$(shell pwd)/pisound-micro-mapper-$(VERSION).tar \$$sha1.tar && rm \$$sha1.tar"
	gzip "pisound-micro-mapper-$(VERSION).tar"

.PHONY: clean schema-check
