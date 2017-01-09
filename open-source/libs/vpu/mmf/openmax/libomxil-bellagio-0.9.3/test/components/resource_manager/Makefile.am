check_PROGRAMS = omxrmtest omxprioritytest

bellagio_LDADD = -lomxil-bellagio
common_CFLAGS = -I$(top_srcdir)/test/components/common -I$(includedir)

omxrmtest_SOURCES = omxrmtest.c omxrmtest.h
omxrmtest_LDADD = $(bellagio_LDADD) -lpthread
omxrmtest_CFLAGS = $(common_CFLAGS)

omxprioritytest_SOURCES = omxprioritytest.c omxprioritytest.h
omxprioritytest_LDADD = $(bellagio_LDADD) -lpthread
omxprioritytest_CFLAGS = $(common_CFLAGS)

