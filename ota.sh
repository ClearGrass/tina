#!/bin/sh
echo "do System Update"
echo "need do ota"

# 删除原有目录etc 并备份
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
aw_upgrade_process.sh -f -l /mnt/UDISK
rm /mnt/UDISK/*
sync



