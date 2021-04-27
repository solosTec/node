#!/bin/bash
#
# This script is intended to run on a smart bpl router as part of updating it from a v0.8 version to version 0.9
# 
#
#${1} - path to ipk you want to install

if [ "$#" -lt "1" ]; then
	echo "Usage: $0 path to ipk to install"
	echo "Example : $0 /tmp/smf_0.9.0.113_armel.ipk"
	exit 1
fi



PATH_TO_IPK=${1}
OLD_APPLICATION_NAME="node"
FOLDER_FOR_DATAABASE_DUMP="/usr/local/etc/databaseoutput"

echo "create folder for database dump"
mkdir -p ${FOLDER_FOR_DATAABASE_DUMP}

echo "create databse dump of v0.8 application"
/usr/local/sbin/segw -C /usr/local/etc/segw_v0.8.cfg -l > ${FOLDER_FOR_DATAABASE_DUMP}/database_dump_v0.8.txt

echo "remove old application"
opkg remove ${OLD_APPLICATION_NAME}

echo "install new application"
opkg install --force-space "${PATH_TO_IPK}"