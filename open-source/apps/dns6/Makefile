#TOPDIR         := $(shell /bin/pwd)
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump
MAKEFILES       = $(TOPDIR)/.config
GENKSYMS        = $(TOOLDIR)genksyms
DEPMOD          = $(TOOLDIR)depmod
MODFLAGS        = -DMODULE
CFLAGS_KERNEL   =
PERL            = perl
CFLAGS		+=-g

export  VERSION PATCHLEVEL SUBLEVEL EXTRAVERSION KERNELRELEASE ARCH \
        CONFIG_SHELL TOPDIR HPATH HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC \
        CPP AR NM STRIP OBJCOPY OBJDUMP MAKE MAKEFILES GENKSYMS MODFLAGS PERL

module_name=dns6
module_version=0.1
module_activex_version=1,0,9,0
module_release=$(shell date +%y%m%d%H%M)
module_pkg=$(module_name)-$(ARCH)-bin-$(SSL)-$(TYPE)-$(module_version)-$(module_release).tar
module_srcpkg=$(module_name)-src-$(module_version)-$(module_release).tgz
module_allsource=$(wildcard *.c)
module_include=$(wildcard *.h)
module_config=$(wildcard *.cfg) $(wildcard *.conf)
module_runpkg=bin conf ssoconf otpconf sslcert cacerts ocspcert cab portal cache crl filesync
module_other=Makefile* *.a *.cab $(module_runpkg) sslengine.crt sslengine.key sslengine.p12

module_exclude=voscli.c vosengine.c
module_source=$(filter-out $(module_exclude), $(module_allsource))
module_objects=$(patsubst %.c,%.o,$(module_source))

module_source_cli=voscli.c
module_objects_cli=$(patsubst %.c,%.o,$(module_source_cli))

LIB+=-lpthread -lutil

.PHONY: version all
all:$(module_name)

$(module_name):depends $(module_objects)
        $(CC) $(CFLAGS) $(module_objects) -o $@ $(LIB)

depends:$(module_source)
        $(CC) -MM $(module_source) > $@
include depends

bf:$(module_allsource) $(module_include) $(module_config) $(module_other)
        tar cvfz $(module_srcpkg) $+

sz:$(module_allsource) $(module_include) $(module_config) $(module_other)
        tar cvfz $(module_srcpkg) $+
        @sz $(module_srcpkg)
        @echo $(module_srcpkg) ok!

