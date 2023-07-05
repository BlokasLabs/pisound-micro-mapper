PREFIX?=/usr/local

BINDIR?=$(PREFIX)/bin

CC?=cc
AR?=ar

CFLAGS?=-O3 -Itps/rapidjson/include

INSTALL?=install
INSTALL_PROGRAM?=$(INSTALL)
INSTALL_DATA?=$(INSTALL) -m 644

CXXFLAGS=$(CFLAGS) -fno-rtti

-include Makefile.local

ifeq ($(DEBUG),yes)
	CFLAGS += -DDEBUG -g -O0
endif

all: pisound-micro-mapper

%.o: %.c %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

src/config-schema.json: src/schema/config-schema.ts
	npx ts-json-schema-generator --path $^ --no-top-ref --type ConfigRoot -o $@

src/config-schema.c: src/config-schema.json
	echo "#include \"config-schema.h\"" > $@
	echo "static const char config_schema[] = {" >> $@
	xxd -i < $^ >> $@
	echo "};" >> $@
	echo "const char *get_config_schema() { return config_schema; }" >> $@
	echo "const size_t get_config_schema_length() { return sizeof(config_schema); }" >> $@

pisound-micro-mapper: src/config-schema.o src/control-manager.o src/alsa-control-server.o src/main.o src/upisnd-control-server.o src/dtors.o src/config-loader.o src/logger.o src/alsa-control-server-loader.o src/upisnd-control-server-loader.o
	$(CXX) $^ $(CFLAGS) -lpthread -lasound $(shell pkg-config --libs libpisoundmicro) -o $@

schema-test: src/schema-test.cpp
	$(CXX) $^ $(CFLAGS) -o $@

schema-check: src/config-schema.json schema-test
	./schema-test src/config-schema.json example.json

install: pisound-micro-mapper
	mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) pisound-micro-mapper $(DESTDIR)$(BINDIR)/

clean:
	rm -f *.o *.a src/*.o pisound-micro-mapper schema-test src/config-schema.c src/config-schema.json

.PHONY: clean schema-check
