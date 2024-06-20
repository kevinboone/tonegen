NAME    := tonegen
VERSION := 0.1a
EXTRA_CFLAGS ?=
EXTRA_LDFLAGS ?=
CC      :=  gcc 
LIBS    := -lm -lasound ${EXTRA_LIBS} 
TARGET	:= $(NAME)
SOURCES := $(shell find src/ -type f -name *.c)
OBJECTS := $(patsubst src/%,build/%,$(SOURCES:.c=.o))
DEPS	:= $(OBJECTS:.o=.deps)
DESTDIR ?= /
PREFIX  := /usr
SHARE   := $(PREFIX)/share
MANDIR  := $(SHARE)/man
BINDIR  := $(PREFIX)/bin
CFLAGS  := -Wall -O3 -Wno-unused-result -DNAME=\"$(NAME)\" -DVERSION=\"$(VERSION)\" -DSHARE=\"$(SHARE)\" -DPREFIX=\"$(PREFIX)\" -I include ${EXTRA_CFLAGS}
LDFLAGS := -s ${EXTRA_LDFLAGS}

all: $(TARGET)
debug: CFLAGS += -g
debug: $(TARGET) 

$(TARGET): $(OBJECTS) 
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS) 

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) -g $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

clean:
	@echo "  Cleaning..."; $(RM) -r build/ $(TARGET) 

install: $(TARGET)
	mkdir -p $(DESTDIR)/$(PREFIX) $(DESTDIR)/$(BINDIR) $(DESTDIR)/$(MANDIR)/man1
	install -m 755 $(TARGET) $(DESTDIR)/${BINDIR}
	install -m 644 man1/* $(DESTDIR)/${MANDIR}/man1/

-include $(DEPS)

.PHONY: clean

