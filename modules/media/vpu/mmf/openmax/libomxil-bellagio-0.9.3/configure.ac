AC_INIT([OpenMAX Bellagio], [0.9.3], [giulio.urlini@st.com], [libomxil-bellagio])
AM_INIT_AUTOMAKE()

# Prerequisite autoconf version
AC_PREREQ([2.59])

AC_CONFIG_HEADERS([config.h])
CFLAGS="${CFLAGS} -Wall -Werror"

################################################################################
# Set the shared versioning info, according to section 6.3 of the libtool info #
# pages. CURRENT:REVISION:AGE must be updated immediately before each release: #
#                                                                              #
#   * If the library source code has changed at all since the last             #
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').             #
#                                                                              #
#   * If any interfaces have been added, removed, or changed since the         #
#     last update, increment CURRENT, and set REVISION to 0.                   #
#                                                                              #
#   * If any interfaces have been added since the last public release,         #
#     then increment AGE.                                                      #
#                                                                              #
#   * If any interfaces have been removed since the last public release,       #
#     then set AGE to 0.                                                       #
#                                                                              #
################################################################################
SHARED_VERSION_INFO="0:0:0"
AC_SUBST(SHARED_VERSION_INFO)

# Check if the OMX_Core.h file is present
AC_CONFIG_SRCDIR([include/OMX_Core.h])

# Set to 'm4' the directory where the extra autoconf macros are stored
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([
    Makefile
    libomxil-bellagio.pc
    libomxil-bellagio.spec
    doc/Doxyfile
    doc/footer_SF.html
    doc/omxregister-bellagio.1
    doc/Makefile
    include/Makefile
    src/Makefile
    src/base/Makefile
    src/core_extensions/Makefile
    src/components/Makefile
    src/components/audio_effects/Makefile
    src/components/clocksrc/Makefile
    src/components/videoscheduler/Makefile
    src/dynamic_loader/Makefile
    m4/Makefile
    test/Makefile
    test/components/Makefile
    test/components/common/Makefile
    test/components/audio_effects/Makefile
    test/components/resource_manager/Makefile
])
################################################################################
# Define the extra arguments the user can pass to the configure script         #
################################################################################

# Check whether volume/mixer components has been requested
AC_ARG_ENABLE(
    [audioeffects],
    [AC_HELP_STRING(
        [--disable-audioeffects],
        [whether to disable the audio effects components compilation])],
    [with_audioeffects=$enableval],
    [with_audioeffects=yes])

#Check whether clock component has been requested
AC_ARG_ENABLE(
    [clocksrc],
    [AC_HELP_STRING(
        [--disable-clocksrc],
        [whether to disable the clocksrc compilation])],
    [with_clocksrc=$enableval],
    [with_clocksrc=yes]) dnl default

#Check whether xvideosink component has been requested
AC_ARG_ENABLE(
    [videoscheduler],
    [AC_HELP_STRING(
        [--disable-videoscheduler],
        [whether to disable the video scheduler component])],
    [with_videoscheduler=$enableval],
    [with_videoscheduler=yes])

#Check whether to disable all components
AC_ARG_ENABLE(
    [components],
    [AC_HELP_STRING(
        [--disable-components],
        [whether to disable all the components])],
    [with_components=$enableval],
    [with_components=yes])

#Check whether to create documentation 
AC_ARG_ENABLE(
    [doc],
    [AC_HELP_STRING(
        [--disable-doc],
        [whether to enable documentation generation])],
    [with_doc=$enableval],
    [with_doc=yes])

AC_ARG_ENABLE(
    [debug],
    [AC_HELP_STRING(
        [--enable-debug],
        [whether to enable an x debug level])],
    [with_debug_level=$enableval],
    [with_debug_level=no])

AC_ARG_ENABLE(
    [android],
    [AC_HELP_STRING(
        [--enable-android],
        [whether to enable android compilation])],
    [with_android=$enableval],
    [with_android=no])

################################################################################
# Check for programs                                                           #
################################################################################

# Check for a working C compiler
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Check for libtool
AM_PROG_LIBTOOL

# Check for doxygen
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN,($HAVE_DOXYGEN) && (test x$with_doc = xyes))
if (test $HAVE_DOXYGEN = "false"); then
             AC_MSG_WARN([*** doxygen not found, docs will not be built])
fi

################################################################################
# Check for libraries                                                          #
################################################################################

AC_CHECK_LIB([pthread], [pthread_create])

# Check for libdl
AC_SEARCH_LIBS([dlopen], [dl], [], [AC_MSG_ERROR([libdl is required])])

if test "x$with_components" = "xno"; then
	with_audioeffects=no
	with_clocksrc=no
	with_videoscheduler=no
fi

# Define components default ldflags (man ld)
PLUGIN_LDFLAGS="-module -avoid-version -no-undefined -as-needed"
AC_SUBST(PLUGIN_LDFLAGS)

AX_SET_PLUGINDIR

CFG_DEBUG_LEVEL=0
# Define debug messages level
if test "x$with_debug_level" = "xno"; then
 CFG_DEBUG_LEVEL=0
fi
if test "x$with_debug_level" = "xyes"; then
 CFG_DEBUG_LEVEL=255
fi
if test "x$with_debug_level" = "x1"; then
 CFG_DEBUG_LEVEL=2
fi
if test "x$with_debug_level" = "x2"; then
 CFG_DEBUG_LEVEL=4
fi
if test "x$with_debug_level" = "x3"; then
 CFG_DEBUG_LEVEL=8
fi
if test "x$with_debug_level" = "x4"; then
 CFG_DEBUG_LEVEL=16
fi
if test "x$with_debug_level" = "x5"; then
 CFG_DEBUG_LEVEL=32
fi
if test "x$with_debug_level" = "x6"; then
 CFG_DEBUG_LEVEL=255
fi

AC_SUBST(CFG_DEBUG_LEVEL)
CFLAGS="${CFLAGS} -DCONFIG_DEBUG_LEVEL=$CFG_DEBUG_LEVEL"

if test "x$with_android" = "xyes"; then
 CFLAGS="${CFLAGS} -DANDROID_COMPILATION"
fi
################################################################################
# Check for header files                                                       #
################################################################################

AC_CHECK_HEADERS([fcntl.h malloc.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])

################################################################################
# Check for pkg-config modules                                                 #
################################################################################

################################################################################
# Check for types                                                              #
################################################################################

AC_TYPE_SIZE_T

################################################################################
# Check for structures                                                         #
################################################################################

# unused

################################################################################
# Check for compiler characteristics                                           #
################################################################################

# unused

################################################################################
# Check for library functions                                                  #
################################################################################

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gethostbyname gettimeofday memset mkdir socket strdup strerror strndup strrchr])

################################################################################
# Check for system services                                                    #
################################################################################

# unused

################################################################################
# Conditionals and file output                                                 #
################################################################################
AM_CONDITIONAL([WITH_AUDIOEFFECTS], [test x$with_audioeffects = xyes])
AM_CONDITIONAL([WITH_CLOCKSRC], [test x$with_clocksrc = xyes])
AM_CONDITIONAL([WITH_VIDEOSCHEDULER],[test x$with_videoscheduler = xyes])

AC_OUTPUT
