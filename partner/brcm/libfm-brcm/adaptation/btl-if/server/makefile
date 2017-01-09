###############################################
#
# Makefile
#

CC := gcc

PROGRAM = btlifs
SOURCES = btl_ifs.c btl_ifs_wrapper.c btl_ifs_dtun.c
CFLAGS += -I . -I ../include -I ../../dtun/include -I ../../dtun/dtuns  -Wall -DBTL_IF_SERVER_TEST_ENABLE -DBTLIF_STANDALONE -g

LDFLAGS +=  -lpthread

all: $(PROGRAM)

$(PROGRAM): $(patsubst %.c, %.o, $(SOURCES))
	$(CC) -o $@ $? $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f $(PROGRAM) *.o *~ *.bak

