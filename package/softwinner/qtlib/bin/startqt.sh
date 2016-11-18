#!/bin/sh

mkdir -p /tmp/class/esp_boot

export PATH=/usr/bin:/usr/sbin:/bin:/sbin
export LD_LIBRARY_PATH=/lib/qtlib

export TSLIB_TSDEVICE=/dev/input/event5
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_PLUGINDIR=/lib/qtlib/ts
export TSLIB_CALIBFILE=/etc/pointercal
export TSLIB_CONSOLEDEVICE=none
export TSLIB_FBDEVICE=/dev/fb0

export QT_QPA_PLATFORM=eglfs
export QT_QPA_PLATFORM_PLUGIN_PATH=/lib/qtlib
export QT_QPA_FONTDIR=/lib/qtlib/fonts
export QML2_IMPORT_PATH=/lib/qtlib/qml
export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event5

cd /bin/qtapp
./baklight 128

logread -f >> kernel.log &

insmod /lib/qtlib/mali.ko
