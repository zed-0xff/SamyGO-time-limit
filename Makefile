ROOT   := /opt/samygo

CROSS  := $(ROOT)/bin/
CC     := $(CROSS)gcc
STRIP  := $(CROSS)strip
LD     := $(CROSS)ld
AR     := $(CROSS)ar

CFLAGS += -Wall -O2
CFLAGS += -I$(ROOT)/include
CFLAGS += -I$(ROOT)/include/SDL
LDFLAGS += -L$(ROOT)/lib
LIB += -lSDL_ttf -lfreetype

PLUGNAME=time_limit
#APP_DIR=/dtv/usb/sda1/time_limit
APP_DIR=/mtd_tlib/GGame/time_limit/
FILES=$(PLUGNAME).so loader.so
SRC=include *.c Makefile

all: loader.so $(PLUGNAME).so

loader.so: loader.o
	$(LD) $(LDFLAGS) -shared -soname,$@ -o $@ $^
	$(STRIP) $@

$(PLUGNAME).so: $(PLUGNAME).o
	$(LD) $(LDFLAGS) -shared -soname,$@ -o $@ $^ $(LIB)
	$(STRIP) $@

clean:
	rm -rf *.so *.o *.a *~

install:
	ncftpput -u root -p "" tv $(APP_DIR) $(FILES)

targz:
	mkdir -p ./$(PLUGNAME)
	cp $(FILES) ./$(PLUGNAME)
	mkdir -p ./src
	cp -R $(SRC) ./src
	tar cvfz samygo-${PLUGNAME}-plugin.tar.gz  $(PLUGNAME)/* src/*

