#!/bin/bash

set -xue


#FILE=tuo_@VERSION@_${TUO_PLATFORM}.exe

#cp build-dir/tuo.exe $TUO_FILE

export TUO_FILE_ZIP="tuo.${TRAVIS_TAG}_32+64_bit.zip" 

#ls -la

zip -r "$TUO_FILE_ZIP" data/ SimpleTUOptimizeStarter.ahk SimpleTUOptimizeStarter.exe "$TUO_FILE_32" "$TUO_FILE_64" "$TUO_FILE_32_DEBUG" "$TUO_FILE_64_DEBUG" 





set +xue
