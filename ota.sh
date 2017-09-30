#!/bin/sh

hwclock -w
echo 0 > /dev/Boost_En
now=`date '+%Y-%m-%d'`
LOG_PATH="/usr/bin/qtapp/etc/log/$now.log"
sync

echo "ota shell log>>>" >> $LOG_PATH
echo "start system update" >> $LOG_PATH

proc=`pgrep BranQt4`
echo "kill BranQt4,pid:$proc" >> $LOG_PATH
kill -9 `ps -ef | grep "BranQt4" | grep -v \"grep\" | awk '{print $2}'`
proc=`pgrep BranQt4`
echo "current BranQt4 pid:$proc" >> $LOG_PATH

echo "write_misc -s fail -v test1.0" >> $LOG_PATH
write_misc -s fail -v test1.0
command=`read_misc | grep command: | tail -1 | cut -d ":" -f 2 | sed s/[[:space:]]//g`
status=`read_misc | grep status: | tail -1 | cut -d ":" -f 2 | sed s/[[:space:]]//g`
version=`read_misc | grep version: | tail -1 | cut -d ":" -f 2 | sed s/[[:space:]]//g`

echo "command:$command" >> $LOG_PATH
echo "status:$status" >> $LOG_PATH
echo "version:$version" >> $LOG_PATH

# 删除原有目录etc 并备份
echo "rm all files except etc" >> $LOG_PATH
for file in /overlay/usr/bin/qtapp/*
do  
	if [ -d "$file" ]  
	then  
		if [ $file != "/overlay/usr/bin/qtapp/etc" ]
		then
  			rm -rf $file
		fi		
	elif [ -f "$file" ] 
	then  
  		rm $file
	fi 
done

# 升级命令
echo "aw_upgrade_process.sh -f -l /mnt/UDISK" >> $LOG_PATH
aw_upgrade_process.sh -f -l /mnt/UDISK

echo "rm /mnt/UDISK/*" >> $LOG_PATH
rm /mnt/UDISK/*

echo "read_misc" >> $LOG_PATH
echo "read_misc | grep command: | tail -1 | cut -d ":" -f 2 | sed s/[[:space:]]//g" >> $LOG_PATH
command=`read_misc | grep command: | tail -1 | cut -d ":" -f 2 | sed s/[[:space:]]//g`
status=`read_misc | grep status: | tail -1 | cut -d ":" -f 2 | sed s/[[:space:]]//g`
version=`read_misc | grep version: | tail -1 | cut -d ":" -f 2 | sed s/[[:space:]]//g`

echo "command:$command" >> $LOG_PATH
echo "status:$status" >> $LOG_PATH
echo "version:$version" >> $LOG_PATH

if [[ $command = "upgrade_end" ]] && [[ $status = "ok" ]];then
	echo "ota success!!!" >> $LOG_PATH
	echo "reboot" >> $LOG_PATH
	reboot
else
	echo "ota error!!!" >> $LOG_PATH
fi

echo "">>$LOG_PATH
echo "">>$LOG_PATH


