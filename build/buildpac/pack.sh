#! /bin/bash

PACK_DIR="vendor/sprd/build/buildpac"
CURRENT_BASEPAC_VERSION="$PACK_DIR/basePac/basepac_version.txt"
basepacCount=`ls $PACK_DIR/basePac/$TARGET_PRODUCT-$TARGET_BUILD_VARIANT/*.pac | wc -l`
echo "===================== beginning make sprdpac now ====================="

function dellastpac()
{
  if [ -d "$ANDROID_PRODUCT_OUT/pac" ]; then
    echo "maybe pac have been created which u make sprdpac last time, so remove this directory now..."
    rm -rf $ANDROID_PRODUCT_OUT/pac
    echo "----remove $ANDROID_PRODUCT_OUT/pac done----"
  else
    echo "---prepare to repack now---"
  fi

  if [ -d "$PACK_DIR/tmpUnpackPac" ]; then
    echo "maybe unpackedimgs have been created which u make sprdpac last time, so remove this directory now..."
    rm -rf $PACK_DIR/tmpUnpackPac
    echo "----remove $PACK_DIR/tmpUnpackPac done----"
  else
    echo "---prepare to repack now---"
  fi
}

dellastpac

function unpack()
{
  echo -e "Current basepac is from: \c"
  cat $CURRENT_BASEPAC_VERSION
  mkdir -p $PACK_DIR/tmpUnpackPac

  if [ $basepacCount = 1 ]; then
    perl $PACK_DIR/tools/unpac_perl/unpac.pl $PACK_DIR/basePac/$TARGET_PRODUCT-$TARGET_BUILD_VARIANT/*.pac $PACK_DIR/tmpUnpackPac
  else
    echo "Sorry, your $TARGET_PRODUCT-$TARGET_BUILD_VARIANT maybe null, pls make sure only one right basePac in it."
    exit 1
  fi
}

unpack

function replace()
{
  echo "Replace bins from replace_bin"
  find $PACK_DIR/basePac/$TARGET_PRODUCT-$TARGET_BUILD_VARIANT/replace_bin -type f -print0 | xargs -0 cp --target-directory=$PACK_DIR/tmpUnpackPac
}

replace

function pack()
{
  mkdir -p $ANDROID_PRODUCT_OUT/pac
  . $PACK_DIR/tools/pac_perl/repack.sh
}

function findImgExistAndRepack()
{
  if [ -n "${ANDROID_PRODUCT_OUT}" ]; then

        if [ -f "$ANDROID_PRODUCT_OUT/system.img" ] && \
           [ -f "$ANDROID_PRODUCT_OUT/boot.img" ] && \
           [ -f "$ANDROID_PRODUCT_OUT/recovery.img" ] && \
           [ -f "$ANDROID_PRODUCT_OUT/cache.img" ] && \
           [ -f "$ANDROID_PRODUCT_OUT/dt.img" ] && \
           [ -f "$ANDROID_PRODUCT_OUT/userdata.img" ] && \
           [ -f "$ANDROID_PRODUCT_OUT/fdl2.bin" ] && \
           [ -f "$ANDROID_PRODUCT_OUT/u-boot.bin" ]; then
              cp $ANDROID_PRODUCT_OUT/system.img  $PACK_DIR/tmpUnpackPac
              cp $ANDROID_PRODUCT_OUT/boot.img  $PACK_DIR/tmpUnpackPac
              cp $ANDROID_PRODUCT_OUT/recovery.img  $PACK_DIR/tmpUnpackPac
              cp $ANDROID_PRODUCT_OUT/cache.img  $PACK_DIR/tmpUnpackPac
              cp $ANDROID_PRODUCT_OUT/dt.img  $PACK_DIR/tmpUnpackPac
              cp $ANDROID_PRODUCT_OUT/userdata.img  $PACK_DIR/tmpUnpackPac
              cp $ANDROID_PRODUCT_OUT/fdl2.bin $PACK_DIR/tmpUnpackPac
              cp $ANDROID_PRODUCT_OUT/u-boot.bin $PACK_DIR/tmpUnpackPac
              echo "system&boot&recovery&cache&dt&userdata img and fdl2&u-boot.bin have been replaced at "$(date +%Y-%m-%d-%X) >> $PACK_DIR/imgreplaced_log.txt

              if [ -d "$ANDROID_PRODUCT_OUT/pac" ]; then
                  echo "maybe pac have been created which u make sprdpac last time, so remove this directory now..."
                  rm -rf $ANDROID_PRODUCT_OUT/pac
                  echo "----remove $ANDROID_PRODUCT_OUT/pac done----"
              else
                  echo "---prepare to repack now---"
              fi

              pack

              if [ -n "$PACK_DIR/tmpUnpackPac" ]; then
                  echo "----prepare to remove this tmpUnpackPac directory----"
                  rm -rf $PACK_DIR/tmpUnpackPac
                  echo "----remove tmpUnpackPac directory done----"
                  echo "=====repacked finished successfully====="
              else
                  echo "Hey guy, why ur tmpUnpackPac missed? Maybe unpack basepac failed, pls check it again, thks!"
              fi

         else
              echo "----imgs have not generated now, pls make sure----"
              exit 1
         fi

  else
         echo "----no product generated, nothing can we do now----"
         exit 1
  fi
}
findImgExistAndRepack

echo "================== Ending make sprdpac ===================="
