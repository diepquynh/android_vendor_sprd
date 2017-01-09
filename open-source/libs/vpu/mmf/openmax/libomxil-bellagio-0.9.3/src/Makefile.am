# Build order is important
# First base, then core, finally components

SUBDIRS = base core_extensions . components dynamic_loader

bin_PROGRAMS = omxregister-bellagio
omxregister_bellagio_SOURCES = omxregister.c common.c common.h
omxregister_bellagio_CFLAGS = -DOMXILCOMPONENTSPATH=\"$(plugindir)/\" \
			      -I$(top_srcdir)/include
omxregister_bellagio_LDFLAGS = -lomxil-bellagio -L$(builddir)

lib_LTLIBRARIES = libomxil-bellagio.la
libomxil_bellagio_la_SOURCES = component_loader.h \
			       st_static_component_loader.c st_static_component_loader.h \
			       omxcore.c omxcore.h \
			       omx_create_loaders_linux.c omx_create_loaders.h \
			       omx_comp_debug_levels.h \
			       extension_struct.h \
			       tsemaphore.c tsemaphore.h \
			       queue.c queue.h \
			       utils.c utils.h \
			       common.c common.h \
			       content_pipe_inet.c content_pipe_inet.h \
			       content_pipe_file.c content_pipe_file.h \
			       omx_reference_resource_manager.c \
			       omx_reference_resource_manager.h

libomxil_bellagio_la_CFLAGS = -I$(top_srcdir)/include -I$(srcdir)/base -I$(srcdir)/core_extensions \
                              -DINSTALL_PATH_STR=\"$(plugindir)\" -DOMX_LOADERS_DIRNAME=\"$(libdir)/omxloaders\/\"
libomxil_bellagio_la_LIBADD = base/libomxbase.la core_extensions/libomxcoreext.la -lpthread
libomxil_bellagio_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@

include_extradir = $(includedir)/bellagio

include_extra_HEADERS = $(srcdir)/omxcore.h \
			$(srcdir)/queue.h \
			$(srcdir)/utils.h \
			$(srcdir)/component_loader.h \
			$(srcdir)/st_static_component_loader.h \
			$(srcdir)/tsemaphore.h \
			$(srcdir)/omx_comp_debug_levels.h \
			$(srcdir)/common.h \
			$(srcdir)/omx_reference_resource_manager.h \
			$(srcdir)/base/omx_base_component.h \
			$(srcdir)/base/omx_base_port.h \
			$(srcdir)/base/omx_base_audio_port.h \
			$(srcdir)/base/omx_base_video_port.h \
			$(srcdir)/base/omx_base_filter.h \
			$(srcdir)/base/omx_base_sink.h \
			$(srcdir)/base/omx_base_source.h \
			$(srcdir)/base/omx_classmagic.h \
			$(srcdir)/base/omx_base_clock_port.h \
			$(srcdir)/base/omx_base_image_port.h \
			$(srcdir)/extension_struct.h \
			$(srcdir)/base/OMXComponentRMExt.h \
			$(srcdir)/core_extensions/OMXCoreRMExt.h \
			$(top_srcdir)/test/components/common/user_debug_levels.h

