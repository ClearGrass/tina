#!/bin/sh

WIFI_START_SCRIPT="/the_path_to/wifi_start.sh"
MIIO_RECV_LINE="/usr/bin/qtapp/miio/miio_recv_line"
MIIO_SEND_LINE="/usr/bin/qtapp/miio/miio_send_line"
WIFI_MAX_RETRY=5
WIFI_RETRY_INTERVAL=3
WIFI_SSID=



GLIBC_TIMEZONE_DIR="/usr/share/zoneinfo"
UCLIBC_TIMEZONE_DIR="/usr/share/zoneinfo/uclibc"

YOUR_LINK_TIMEZONE_FILE="/mnt/data/TZ"
YOUR_TIMEZONE_DIR=$UCLIBC_TIMEZONE_DIR



# contains(string, substring)
#
# Returns 0 if the specified string contains the specified substring,
# otherwise returns 1.
contains() {
    string="$1"
    substring="$2"
    if test "${string#*$substring}" != "$string"
    then
        return 0    # $substring is in $string
    else
        return 1    # $substring is not in $string
    fi
}

send_helper_ready() {
    ready_msg="{\"method\":\"_internal.helper_ready\"}"
    echo $ready_msg
    $MIIO_SEND_LINE "$ready_msg"
}

req_wifi_conf_status() {
    REQ_WIFI_CONF_STATUS_RESPONSE="{\"method\":\"_internal.res_wifi_conf_status\",\"params\":1}"
}

request_dinfo() {

    dinfo_file="/usr/bin/qtapp/etc/device.conf"
    dinfo_did=`cat $dinfo_file | grep -v ^# | grep did= | tail -1 | cut -d '=' -f 2`
    dinfo_key=`cat $dinfo_file | grep -v ^# | grep key= | tail -1 | cut -d '=' -f 2`
    dinfo_vendor=`cat $dinfo_file | grep -v ^# | grep vendor= | tail -1 | cut -d '=' -f 2`
    dinfo_mac=`cat $dinfo_file | grep -v ^# | grep mac= | tail -1 | cut -d '=' -f 2`
    dinfo_model=`cat $dinfo_file | grep -v ^# | grep model= | tail -1 | cut -d '=' -f 2`
    RESPONSE_DINFO="{\"method\":\"_internal.response_dinfo\",\"params\":{"
    if [ x$dinfo_did != x ]; then
	RESPONSE_DINFO="$RESPONSE_DINFO\"did\":$dinfo_did"
    fi
    if [ x$dinfo_key != x ]; then
	RESPONSE_DINFO="$RESPONSE_DINFO,\"key\":\"$dinfo_key\""
    fi
    if [ x$dinfo_vendor != x ]; then
	RESPONSE_DINFO="$RESPONSE_DINFO,\"vendor\":\"$dinfo_vendor\""
    fi
    if [ x$dinfo_mac != x ]; then
	RESPONSE_DINFO="$RESPONSE_DINFO,\"mac\":\"$dinfo_mac\""
    fi
    if [ x$dinfo_model != x ]; then
	RESPONSE_DINFO="$RESPONSE_DINFO,\"model\":\"$dinfo_model\""
    fi
    RESPONSE_DINFO="$RESPONSE_DINFO}}"
}

request_dtoken() {
    dtoken_string=$1
    dtoken_token=${dtoken_string##*ntoken\":\"}
    dtoken_token=${dtoken_token%%\"*}

    dtoken_file="/usr/bin/qtapp/etc/device.token"

    if [ -e ${dtoken_file} ]; then
	dtoken_token=`cat ${dtoken_file}`
    else
	echo ${dtoken_token} > ${dtoken_file}
    fi
    
    RESPONSE_DTOKEN="{\"method\":\"_internal.response_dtoken\",\"params\":\"${dtoken_token}\"}"
    RESPONSE_DCOUNTRY="{\"method\":\"_internal.response_dcountry\",\"params\":\"\"}"
}

save_wifi_conf() {
    echo "save_wifi_conf"
}

clear_wifi_conf() {
    echo "clear_wifi_conf"
}

save_tz_conf() {
	new_tz=$YOUR_TIMEZONE_DIR/$1
	if [ -f $new_tz ]; then
		unlink $YOUR_LINK_TIMEZONE_FILE
		ln -sf  $new_tz $YOUR_LINK_TIMEZONE_FILE
		echo "timezone set success:$new_tz"
	else
		echo "timezone is not exist:$new_tz"
	fi
}

sanity_check() {
    echo "sanity_check"
}

main() {
    while true; do
	BUF=`$MIIO_RECV_LINE`
	if [ $? -ne 0 ]; then
	    sleep 1;
	    continue
	fi
	if contains "$BUF" "_internal.info"; then

	    STRING=`ifconfig wlan0`

	    ip=${STRING##*inet addr:}
	    ip=`echo ${ip} | cut -d ' ' -f 1`
	    echo "ip: $ip"

	    
	    netmask=${STRING##*Mask:}
	    netmask=`echo ${netmask} | cut -d ' ' -f 1`
	    echo "netmask: $netmask"

	    gw=`route -n|grep 'UG'|tr -s ' ' | cut -f 2 -d ' '`
	    echo "gw: $gw"

	    # get vendor and then version
	    vendor=`grep "vendor" /usr/bin/qtapp/etc/device.conf | cut -f 2 -d '=' | tr a-z A-Z`
	    sw_version=`grep "${vendor}_VERSION" /usr/bin/qtapp/os-release | cut -f 2 -d '='`
	    echo $sw_version
	    if [ -z $sw_version ]; then
		sw_version="unknown"
	    fi

	    msg="{\"method\":\"_internal.info\",\"partner_id\":\"\",\"params\":{\
\"hw_ver\":\"Linux\",\"fw_ver\":\"$sw_version\",\
\"ap\":{\
 \"ssid\":\"\",\"bssid\":\"\"\
},\
\"netif\":{\
 \"localIp\":\"$ip\",\"mask\":\"$netmask\",\"gw\":\"$gw\"\
}}}"

	    echo $msg
	    $MIIO_SEND_LINE "$msg"
	elif contains "$BUF" "_internal.req_wifi_conf_status"; then
	    echo "Got _internal.req_wifi_conf_status"
	    req_wifi_conf_status "$BUF"
	    echo $REQ_WIFI_CONF_STATUS_RESPONSE
	    $MIIO_SEND_LINE "$REQ_WIFI_CONF_STATUS_RESPONSE"
	elif contains "$BUF" "_internal.wifi_start"; then
	    echo "Got _internal.wifi_start"
	elif contains "$BUF" "_internal.request_dinfo"; then
	    echo "Got _internal.request_dinfo"
	    request_dinfo "$BUF"
	    echo $RESPONSE_DINFO
	    $MIIO_SEND_LINE "$RESPONSE_DINFO"
	elif contains "$BUF" "_internal.request_dtoken"; then
	    echo "Got _internal.request_dtoken"
	    request_dtoken "$BUF"
	    echo $RESPONSE_DTOKEN
	    $MIIO_SEND_LINE "$RESPONSE_DTOKEN"
		echo $RESPONSE_DCOUNTRY
	    $MIIO_SEND_LINE "$RESPONSE_DCOUNTRY"
	elif contains "$BUF" "_internal.config_tz"; then
	    echo "Got _internal.config_tz"
	    tz=${BUF##*\",\"tz\":\"}
	    tz=${tz%%\"*}

	    save_tz_conf "$tz"
	else
	    echo "Unknown cmd: $BUF"
	fi
    done
}

sanity_check
send_helper_ready
main
