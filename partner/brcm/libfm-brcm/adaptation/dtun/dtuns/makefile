###############################################
#
# Makefile
#
#

CC := gcc

PROGRAM = dtuns
SOURCES = dtun_svr.c dtun_test.c
CFLAGS += -I . -I ../include  -Wall

#CFLAGS  += `pkg-config glib --cflags`

LDFLAGS +=  -lpthread #`pkg-config glib --libs`


all: $(PROGRAM)

$(PROGRAM): $(patsubst %.c, %.o, $(SOURCES))
	$(CC) -o $@ $? $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f $(PROGRAM) *.o *~ *.bak

