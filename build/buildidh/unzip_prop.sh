#! /bin/bash

if [ ! -d "out/IDH" ]; then
  mkdir -p out/IDH;
fi

if [ -f proprietories-${TARGET_PRODUCT}-${TARGET_BUILD_VARIANT}.zip ]; then

  unzip -o proprietories-${TARGET_PRODUCT}-${TARGET_BUILD_VARIANT}.zip -d out/IDH;
  mv out/IDH/out/target/product/*/* out/target/product/$1/
fi
