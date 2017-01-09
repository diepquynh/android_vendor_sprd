#!/bin/sh

# First clear the directory 
rm -fr *.[cho]
rm Makefile.am.sample


# Then regenerate the whole structure
make regen

# Build the debug library
make -f Makefile.debug

# build the main library
make clean
make

# build the convertion tool
make conv

# Remove the objetc files...
rm -fr *.o