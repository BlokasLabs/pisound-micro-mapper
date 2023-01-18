PREFIX?=/usr/local

BINDIR?=$(PREFIX)/bin

CC?=cc
AR?=ar

CFLAGS?=-O3

INSTALL?=install
INSTALL_PROGRAM?=$(INSTALL)
INSTALL_DATA?=$(INSTALL) -m 644

ifeq ($(DEBUG),yes)
	CFLAGS += -DDEBUG -g -O0
endif

CXXFLAGS=$(CFLAGS) -fno-rtti

all: pisound-micro-mapper

%.o: %.c %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

pisound-micro-mapper: src/control-manager.o src/alsa-control-server.o src/main.o src/upisnd-control-server.o src/dtors.o
	$(CXX) $(CFLAGS) $^ -lpthread -lasound -lpisoundmicro -o $@

install: pisound-micro-mapper
	mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) pisound-micro-mapper $(DESTDIR)$(BINDIR)/

clean:
	rm -f *.o *.a src/*.o pisound-micro-mapper

.PHONY: clean
