#!/bin/bash

set -xue

sudo apt-get update

echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" \
    | sudo tee /etc/apt/sources.list.d/mxeapt.list
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB

sudo apt-get update


MXE_TARGET=i686-w64-mingw32.static
MXE2_TARGET=$(echo "$MXE_TARGET" | sed 's/_/-/g')
sudo apt-get --yes install \
    mxe-${MXE2_TARGET}-boost


MXE_TARGET=x86-64-w64-mingw32.static

MXE2_TARGET=$(echo "$MXE_TARGET" | sed 's/_/-/g')
sudo apt-get --yes install \
    mxe-${MXE2_TARGET}-boost


set +xue
