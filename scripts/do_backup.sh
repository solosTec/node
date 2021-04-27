#!/bin/sh

echo " =========== DO BACKUP" ===============

DATA_PATH="/usr/local/etc/smf"

JSON_V08="segw_v0.8.json"
JSON_V09="segw_v0.9.json"
DATABASE="segw.database"

BACKUP_FROM_v08=0
BACKUP_FROM_v09=0


if [ -f ${DATA_PATH}/${JSON_V08}  ]; then
	echo "${PROJECT_NAME} backup previous configuration of segw_v0.8"
   mv ${DATA_PATH}/${JSON_V08} ${DATA_PATH}/${JSON_V08}-backup
   BACKUP_FROM_v08=1
fi

if [ -f ${DATA_PATH}/${JSON_V09}  ]; then
	echo "${PROJECT_NAME} backup previous configuration of segw_v0.9"
   mv ${DATA_PATH}/${JSON_V09} ${DATA_PATH}/${JSON_V09}-backup
   BACKUP_FROM_v09=1
fi

if [ -f ${DATA_PATH}/segw.database  ]; then
	echo "${PROJECT_NAME} backup database of segw"
   mv ${DATA_PATH}/${DATABASE} ${DATA_PATH}/${DATABASE}-backup
fi




