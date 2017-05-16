if [ $1 == "SystemUpdate" ]
	then
	echo "do System Update"

	echo "need do ota"

	pkill BranQt4
	
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

	#sync
	sleep 5
	reboot

	elif [ $1 == "BranUpdate" ]
  	then
  	echo "Bran Update"
	else
  	echo "Please make sure the positon variable is SystemUpdate or BranUpdate."
fi


