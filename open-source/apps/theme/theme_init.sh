#!/system/bin/sh
# create by spreadst

function create_theme_dir
{
  DIR=$1
  MOD=$2
  USR=$3
  GRP=$4
  mkdir -p $DIR
  if [ -d $DIR ] ; then
    chmod $MOD $DIR
    chown $USR:$GRP $DIR
  fi
}

create_theme_dir /data/theme 0775 theme system
create_theme_dir /data/theme/overlay 0775 theme system
create_theme_dir /data/theme/additional 0775 theme system
create_theme_dir /data/theme/default/ 0775 theme system
create_theme_dir /data/theme/default/preview 0775 theme system
create_theme_dir /data/theme/additional/holo 0775 theme system
create_theme_dir /data/theme/additional/holo/preview 0775 theme system
