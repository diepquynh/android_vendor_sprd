omxvideoscheddir = $(plugindir)

omxvideosched_LTLIBRARIES = libomxvideosched.la

libomxvideosched_la_SOURCES = omx_video_scheduler_component.c omx_video_scheduler_component.h \
								library_entry_point.c

libomxvideosched_la_LIBADD = $(top_builddir)/src/libomxil-bellagio.la
libomxvideosched_la_LDFLAGS = 
libomxvideosched_la_CFLAGS = -I$(top_srcdir)/include \
			-I$(top_srcdir)/src \
			-I$(top_srcdir)/src/base
