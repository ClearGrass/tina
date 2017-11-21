#!/bin/sh
now=`date '+%Y-%m-%d %H:%M:%S'`

BRAN_OS='BranQt4'
MIIO_CLIENT='miio_client -D'
MIIO_SHELL='miio_client_helper_nomqtt.sh'
thisLog='/usr/bin/qtapp/watchdog.log'
baseDir="/usr/bin/qtapp"
sleepTime=5

if [ ! -f "$baseDir/BranQt4" ];then
	echo "$baseDir/BranQt4 missing, check agin" > "$thisLog"
	exit
fi

user="root"
if [ "$user" != "root" ];then
	echo "this tool must run as root"
	exit
fi

while [ 0 -lt 1 ]
do
	now=`date '+%Y-%m-%d %H:%M:%S'`
	ret=`ps aux | grep "$BRAN_OS" | grep -v grep | wc -l`
	if [ $ret -eq 0 ];then
		cd $baseDir
		echo "$now BranQt4 not exists,restart process now..." > "$thisLog"
		./BranQt4 -qws -font &
	fi
	ret=`ps aux | grep "$MIIO_CLIENT" | grep -v grep | wc -l`
	if [ $ret -eq 0 ];then
		cd $baseDir
		echo "$now miio_client not exists,restart process now..." > "$thisLog"
		miio/miio_client -D
	fi
	ret=`ps aux | grep "$MIIO_SHELL" | grep -v grep | wc -l`	
	if [ $ret -eq 0 ];then
		cd $baseDir
		echo "$now miio_client not exists,restart process now..." > "$thisLog"
		miio/miio_client_helper_nomqtt.sh &
	fi
	sleep $sleepTime
done


