#!/system/xbin/busybox sh

sd_cache_mounted=0

mount_cache() {
  echo "need to create /storage/sdcard0/cachedisk"
    #create a new virtual partion
  diskfree=$(dfex |busybox grep /storage/sdcard0 |busybox awk '{print $4}')
  fsize=61440
  echo "diskfree size if $diskfree, need size is $fsize"
  if busybox [ $diskfree -gt $fsize ]
  then
    dd if=/dev/zero of=/storage/sdcard0/cachedisk bs=1024 count=$fsize
    #waiting for cachedisk maded
    echo "make sd cache"
    #format cachedisk with ext4
    make_ext4fs -l 62914560 -b 4096 /storage/sdcard0/cachedisk
    umount /cache
    if $(mount -t ext4 -o loop /storage/sdcard0/cachedisk /cache)
    then
      echo "mount sd cache succeed"
            sd_cache_mounted=1
    else
      echo "mount sd cache failed"
      umount /cache
      mount -t ubifs /dev/ubi0_cache /cache
    fi
  else
    #no space for cache virtual partion
    echo "no space for cache virtual partion"
    umount /cache
    mount -t ubifs /dev/ubi0_cache /cache
    fi
}
if busybox [ 1 -ne $# ]
then
  echo "Argument is wrong"
  exit 0
else
  sd_cache_mounted=$(getprop persist.sys.cache_on_sd)
  echo "sd_cache_mounted=$sd_cache_mounted"
if busybox [ 1 = $sd_cache_mounted -a 1 -eq $1 ]
then
    umount /cache
    umount /cache2
    mount_cache_times=0
    while true
    do
        if $(mount -t ubifs /dev/ubi0_cache /cache)
        then
          echo "mount cache succeed"
          sd_cache_mounted=0
          setprop persist.sys.cache_on_sd $sd_cache_mounted
          break
        else
          mount_cache_times=$(busybox expr $mount_cache_times + 1)
          if busybox [ $mount_cache_times -gt 5 ]
          then
            echo "mount times greate than 5, error!"
            break
          fi
        fi
    done
elif busybox [ 0 = $sd_cache_mounted -a 0 -eq $1 ]
then
if ls /storage/sdcard0 >/dev/null
then
#with sdcard
    #while until sdcard mounted
    while   sleep 2
    do
      if  mount | busybox grep "vfat"
      then
        echo "sdcard mounted"
        break
      else
        echo "sdcard not mounted"
        mount -t ubifs /dev/ubi0_cache /cache
        exit 0
      fi
    done

    fsize=0
    #mount /storage/sdcard0/cachedisk to /cache
    if ls /storage/sdcard0/cachedisk >/dev/null
    then
        umount /cache
        if $(mount -t ext4 -o loop /storage/sdcard0/cachedisk /cache)
        then
          echo "mount sd cache succeed"
          sd_cache_mounted=1
        else
          rm /storage/sdcard0/cachedisk
          mount_cache
        fi
    else
        mount_cache
    fi
else
#without sdcard
  echo "without sdcard"
fi

echo "sd_cache_mounted is $sd_cache_mounted"
if busybox [ 1 = $sd_cache_mounted ]
then
  mount_times=0
  mount_res=1
  while busybox [ 0 != $mount_res ]
  do
    echo "mount -t ubifs /dev/ubi0_cache /cache2"
    mount -t ubifs /dev/ubi0_cache /cache2
    mount_res=$?
    echo "mount_res=$mount_res"
    mount_times=$(busybox expr $mount_times + 1)
    if busybox [ $mount_times -gt 5 ]
    then
      echo "mount times greate than 5, error!"
      break
    fi
  done
fi
echo "setprop persist.sys.cache_on_sd $sd_cache_mounted"
setprop persist.sys.cache_on_sd $sd_cache_mounted


#echo "========mount state=========" >>/data/system/mkcached.log
#mount >>/data/system/mkcached.log
#echo "========mount state=========" >>/data/system/mkcached.log

if busybox [ 1 = $sd_cache_mounted ]
then
  chown system.cache /cache
  chmod 0770 /cache
  mkdir /cache/lost+found 0770
  chown root.root /cache/lost+found
  chmod 0770 /cache/lost+found
fi

fi

fi

