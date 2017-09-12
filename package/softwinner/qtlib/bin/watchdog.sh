#!/bin/sh
now=`date '+%Y-%m-%d %H:%M:%S'`

grepFlag='BranQt4'
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
	ret=`ps aux | grep "$grepFlag" | grep -v grep | wc -l`

	if [ $ret -eq 0 ];then
		cd $baseDir
		echo "$now process not exists,restart process now..." > "$thisLog"
		./BranQt4 -qws -font &
		echo "$now restart done ......" > "$thisLog"
		cd $curDir
	else
		echo "$now process exists, sleep $sleepTime seconds" > "$thisLog"
	fi
	sleep $sleepTime
done


