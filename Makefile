PREFIX?=/usr/local

BINDIR?=$(PREFIX)/bin

CC?=cc
AR?=ar

CFLAGS?=-O3 -Itps/rapidjson/include
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
endif

all: pisound-micro-mapper

%.o: %.c %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.c: schema/%.ts
	./gen_schema.sh $^ $@

.PRECIOUS: %.c

OBJ = src/config_schema.o src/alsa_schema.o src/pisound_micro_schema.o src/control-manager.o src/alsa-control-server.o src/main.o src/upisnd-control-server.o src/dtors.o src/config-loader.o src/logger.o src/alsa-control-server-loader.o src/upisnd-control-server-loader.o
DEP = $(OBJ:%.o=%.d)

pisound-micro-mapper: $(OBJ)
	$(CXX) $^ $(CFLAGS) -lpthread -lasound $(shell pkg-config --libs libpisoundmicro) -o $@

-include $(DEP)

schema-test: src/schema-test.cpp
	$(CXX) $^ $(CFLAGS) -o $@

schema-check: src/config-schema.json schema-test
	./schema-test src/config-schema.json example.json

install: pisound-micro-mapper
	mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) pisound-micro-mapper $(DESTDIR)$(BINDIR)/

clean:
	rm -f *.o *.a src/*.o src/*_schema.json src/*_schema.c pisound-micro-mapper schema-test

.PHONY: clean schema-check
