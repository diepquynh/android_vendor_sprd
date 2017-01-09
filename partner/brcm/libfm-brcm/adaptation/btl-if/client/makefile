###############################################
#
# Makefile
#
#

DEPDIR = .deps

CC := gcc

df = $(DEPDIR)/$(*F)

PROGRAM = btlifc
SOURCES = btl_ifc.c btl_ifc_wrapper.c blz_wrapper.c
CFLAGS += -I . -I ../include -I usr/include/ -I ../../dtun/include -Wall -DBTL_IF_CLIENT_TEST_ENABLE -g -DHAVE_READLINE

#CFLAGS  += `pkg-config glib --cflags`

LDFLAGS +=  -lpthread -lreadline #`pkg-config glib --libs`


#%.o : %.c
#	@$(MAKEDEPEND); \
#	cp $(df).d $(df).P; \
#		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
#		-e '/^$$/ d' -e 's/$$/ :/' < $(df).d >> $(df).P; \
#		rm -f $(df).d
#	$(COMPILE.c) -o $@ $< $(CFLAGS)

#-include $(SRCS:%.c=$(DEPDIR)/%.P)

all: $(PROGRAM)

$(PROGRAM): $(patsubst %.c, %.o, $(SOURCES))
	$(CC) -o $@ $? $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)


clean:
	rm -f $(PROGRAM) *.o *~ *.bak

