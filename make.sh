#!/bin/sh
touch package/softwinner/qtlib/Makefile 
make -j8
source scripts/setenv.sh 
make_ota_image 

