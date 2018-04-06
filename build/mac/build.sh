#!/bin/bash

set -xue

make rebuild -f Makefile.osx VERSION=$TRAVIS_TAG;

./tuo.osx -version

cp tuo.osx $TUO_FILE_32

cp tuo.osx $TUO_FILE_64

set +xue
