#
# This is a build script for a product package
#

if [ $# -lt 5 -o "$1" = "-h" ]; then
	echo "Usage: $0 <product> <variant> <vlx> <outdir> <-jN>"
	exit 1
fi

if [ ! -f Makefile -o ! -f build/core/main.mk ]; then
	echo "Current directory is not an android folder!"
	exit 1
fi

PROD=$1
VAR=$2
VLX=$3
OUTDIR=$4
JOBS=$5

MOD=$6
if [ -z "$MOD" ]; then
  LOG=$OUTDIR/$PROD-$VAR-$VLX.build.log
  PAK=$OUTDIR/$PROD-$VAR-$VLX.tar.gz
else
 LOG=$OUTDIR/$PROD-$VAR-$VLX-$MOD.build.log
 PAK=$OUTDIR/$PROD-$VAR-$VLX-$MOD.tar.gz
fi

echo "==== $PROD-$VAR-$VLX Start ===="

echo "==== $ANDROID_PRODUCT_OUT/$PROD-$VAR-$VLX ====" > $LOG
date >> $LOG
echo "==== ====" >> $LOG

# setenv & choose product
. build/envsetup.sh >>$LOG 2>&1
choosecombo 1 $PROD $VAR >>$LOG 2>&1
# failure handle
if [ "`cat $LOG|grep 'Not a valid product'`" ];then
	echo "==== ====" >> $LOG
	date >> $LOG
	echo "==== Build Failed ====" >> $LOG

	exit 1
fi

# do clean
uclean >>/dev/null 2>&1
kclean >>/dev/null 2>&1
make clean >>/dev/null 2>&1

# do make
kheader >>$LOG 2>&1
if [ -z "$MOD" ]; then
  make KALLSYMS_EXTRA_PASS=1 $JOBS >>$LOG 2>&1
else
  make KALLSYMS_EXTRA_PASS=1 $JOBS $MOD >>$LOG 2>&1
fi

# failure handle
if [ $? -ne 0 ]; then
	echo "==== ====" >> $LOG
	date >> $LOG
	echo "==== Build Failed ====" >> $LOG

	exit 1
fi

cd $ANDROID_PRODUCT_OUT
cp obj/KERNEL/vmlinux symbols/

find . -maxdepth 1 -name "*.img" -o -name "*.bin" -o -name "*.xml" -o -name "installed-files.txt" -o -name "$MOD.apk" | xargs tar -zcf $PAK 
cd -


echo "==== ====" >> $LOG
date >> $LOG

if [ -f "$ANDROID_PRODUCT_OUT/system.img" ]; then
  info=($(du -skh $ANDROID_PRODUCT_OUT/system.img))
  size=${info[0]}
  echo "system.img:$size" >>$LOG
fi

echo "==== Build Successfully ====" >> $LOG

echo "==== $PROD-$VAR-$VLX Done ===="
