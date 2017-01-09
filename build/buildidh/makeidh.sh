#! /bin/bash

IDH_BUILD_SYSTEM=vendor/sprd/build/buildidh
IDH_CODE_SCRIPT=$IDH_BUILD_SYSTEM/makeidh.pl

NOTRELEASE_LIST=`repo forall --group=idh,idhdel -c 'echo $REPO_PATH,'`
NOTRELEASES=`echo $NOTRELEASE_LIST | sed 's/[[:space:]]//g'`

#EXCLUDE_RELEASE is a white list
EXCLUDE_RELEASE=$1

#make idh.code.tgz and conf-$plat.tar.gz
if [ ! -f sps.image/idh.code.tgz ]; then
/usr/bin/perl $IDH_CODE_SCRIPT `pwd` "$NOTRELEASES" "$EXCLUDE_RELEASE"
fi
