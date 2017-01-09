man_MANS = omxregister-bellagio.1

EXTRA_DIST = Doxyfile.in footer_SF.html.in

# ensure doxygen-build.stamp is included, or else the documentation will
# not be built or distributed
noinst_DATA = doxygen-build.stamp

if HAVE_DOXYGEN
#doxygen-build.stamp: Doxyfile $(top_srcdir)
doxygen-build.stamp: Doxyfile $(top_srcdir)/include/*.h $(top_srcdir)/src/*.h $(top_srcdir)/src/base/*.h
	doxygen
	touch doxygen-build.stamp
else
doxygen-build.stamp:
	echo "*** Warning: Doxygen not found; documentation will not be built."
	touch doxygen-build.stamp
endif

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)

clean-local:
	if test -d $(PACKAGE); then rm -rf $(PACKAGE); fi
	if test -f doxygen-build.stamp; then rm -f doxygen-build.stamp; fi

