###############################################
#
# Makefile
#
#

CC := gcc

PROGRAM = bthwtun

SOURCES = bthwtun.c

CFLAGS += -DNATIVE -I . -I ../include -Wall -g
LDFLAGS +=  -lpthread

# skip blz until kernel issue resolved
CFLAGS += -DENABLE_BLZ
LDFLAGS +=  -lbluetooth

#CFLAGS  += `pkg-config glib --cflags`
#`pkg-config glib --libs`

all: $(PROGRAM)

$(PROGRAM): $(patsubst %.c, %.o, $(SOURCES))
	$(CC) -o $@ $? $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f $(PROGRAM) *.o *~ *.bak

