#
# This is a build script for all daily packages of sprd android
#

if [ $# -ne 1 -o "$1" = "-h" ]; then
	echo "Usage: $0 <outdir>"
	exit 1
fi

if [ ! -f Makefile -o ! -f build/core/main.mk ]; then
	echo "Current directory is not an android folder!"
	exit 1
fi

BINNAME=`readlink -f $0`
BINDIR=`dirname $BINNAME`

if [ ! -f $BINDIR/build_product.sh ]; then
	echo "$BINDIR/build_product.sh not existed!"
	exit 1
fi

if [ ! -f $BINDIR/build_proprietories.sh ]; then
	echo "$BINDIR/build_proprietories.sh not existed!"
	exit 1
fi

OUTDIR=`readlink -f $1`
mkdir -p $OUTDIR
if [ $? -ne 0 ]; then
	echo "Failed to create $OUTDIR"
	exit 1
fi

export BUILD_ID=JRO03C
export BUILD_NUMBER=`date +W%g.%V.%u-%H%M%S`
repo manifest -r -o $OUTDIR/manifest.$BUILD_NUMBER.xml

echo "==== Spreadtrum Android Build Start ===="

N=16

# native version
# vlx version
# maxscend cmmb
export BOARD_CMMB_HW=mxd
$BINDIR/build_product.sh sp8810eabase userdebug vlx_mxd $OUTDIR -j$N
$BINDIR/build_product.sh sp8810eaplus userdebug vlx_mxd $OUTDIR -j$N
unset BOARD_CMMB_HW
$BINDIR/build_product.sh sp8810eabase userdebug native $OUTDIR -j$N

echo "==== Spreadtrum Android Build Done ===="

