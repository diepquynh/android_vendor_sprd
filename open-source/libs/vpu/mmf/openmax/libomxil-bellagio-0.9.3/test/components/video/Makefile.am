check_PROGRAMS = omxvideodectest omxvideoenctest omxparsertest

bellagio_LDADD = $(top_builddir)/src/libomxil-bellagio.la
bellagio_CFLAGS = -I$(top_srcdir)/include
common_LDADD = ../common/libtestcommon.la
common_CFLAGS = -I$(srcdir)/../common

omxvideodectest_SOURCES = omxvideodectest.c omxvideodectest.h
omxvideodectest_LDADD   = $(bellagio_LDADD) $(common_LDADD) -lpthread
omxvideodectest_CFLAGS  = $(bellagio_CFLAGS) $(common_CFLAGS)


omxvideoenctest_SOURCES = omxvideoenctest.c omxvideoenctest.h
omxvideoenctest_LDADD   = $(bellagio_LDADD) $(common_LDADD) -lpthread
omxvideoenctest_CFLAGS  = $(bellagio_CFLAGS) $(common_CFLAGS)

omxvideocapturetest_SOURCES = omxparsertest.c omxparsertest.h
omxvideocapturetest_LDADD   = $(bellagio_LDADD) $(common_LDADD) -lpthread 
omxvideocapturetest_CFLAGS  = $(bellagio_CFLAGS) $(common_CFLAGS)

