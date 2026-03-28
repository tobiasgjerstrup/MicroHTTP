CC ?= cc
AR ?= ar
RANLIB ?= ranlib

CPPFLAGS ?= -Iinclude
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11
LDFLAGS ?=
LDLIBS ?=

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig

VERSION ?= 0.1.0
SOVERSION ?= 0

LIB_NAME := microhttp
LIB_SRC := src/tcp.c
LIB_OBJ := $(LIB_SRC:.c=.o)

STATIC_LIB := lib$(LIB_NAME).a
REAL_SHARED_LIB := lib$(LIB_NAME).so.$(VERSION)
SONAME_LIB := lib$(LIB_NAME).so.$(SOVERSION)
LINK_SHARED_LIB := lib$(LIB_NAME).so
PKGCONFIG_FILE := $(LIB_NAME).pc

DEMO_TARGET := hello
DEMO_SRC := example/main.c

.PHONY: all clean demo run install uninstall

all: $(STATIC_LIB) $(REAL_SHARED_LIB) $(SONAME_LIB) $(LINK_SHARED_LIB) $(PKGCONFIG_FILE)

$(LIB_OBJ): %.o: %.c include/microhttp.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -c $< -o $@

$(STATIC_LIB): $(LIB_OBJ)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(REAL_SHARED_LIB): $(LIB_OBJ)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,$(SONAME_LIB) -o $@ $^ $(LDLIBS)

$(SONAME_LIB): $(REAL_SHARED_LIB)
	ln -sf $< $@

$(LINK_SHARED_LIB): $(SONAME_LIB)
	ln -sf $< $@

$(PKGCONFIG_FILE): microhttp.pc.in Makefile
	sed \
		-e 's|@PREFIX@|$(PREFIX)|g' \
		-e 's|@LIBDIR@|$(LIBDIR)|g' \
		-e 's|@INCLUDEDIR@|$(INCLUDEDIR)|g' \
		-e 's|@VERSION@|$(VERSION)|g' \
		$< > $@

demo: $(DEMO_TARGET)

$(DEMO_TARGET): $(DEMO_SRC) $(LINK_SHARED_LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -L. -Wl,-rpath,'$$ORIGIN' -l$(LIB_NAME) -o $@

run: demo
	./$(DEMO_TARGET)

install: all
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(PKGCONFIGDIR)
	install -m 644 include/microhttp.h $(DESTDIR)$(INCLUDEDIR)/microhttp.h
	install -m 644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/$(STATIC_LIB)
	install -m 755 $(REAL_SHARED_LIB) $(DESTDIR)$(LIBDIR)/$(REAL_SHARED_LIB)
	install -m 644 $(PKGCONFIG_FILE) $(DESTDIR)$(PKGCONFIGDIR)/$(PKGCONFIG_FILE)
	ln -sf $(REAL_SHARED_LIB) $(DESTDIR)$(LIBDIR)/$(SONAME_LIB)
	ln -sf $(SONAME_LIB) $(DESTDIR)$(LIBDIR)/$(LINK_SHARED_LIB)

uninstall:
	rm -f $(DESTDIR)$(INCLUDEDIR)/microhttp.h
	rm -f $(DESTDIR)$(LIBDIR)/$(STATIC_LIB)
	rm -f $(DESTDIR)$(LIBDIR)/$(LINK_SHARED_LIB)
	rm -f $(DESTDIR)$(LIBDIR)/$(SONAME_LIB)
	rm -f $(DESTDIR)$(LIBDIR)/$(REAL_SHARED_LIB)
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/$(PKGCONFIG_FILE)

clean:
	rm -f $(LIB_OBJ) $(STATIC_LIB) $(LINK_SHARED_LIB) $(SONAME_LIB) $(REAL_SHARED_LIB) $(PKGCONFIG_FILE) $(DEMO_TARGET)