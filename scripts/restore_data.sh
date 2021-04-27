#!/bin/sh

DATA_PATH="/usr/local/etc/smf"
DATABASE_TEXTDUMP="/usr/local/etc/smf/databaseoutput/database_dump_v0.8.txt"

#use this in local tests:
#DATABASE_TEXTDUMP="${HOME}/workspace/samrd/node/scripts/db_example_db_list_v0.8.txt"



getvalbykey()
{
   key="${1}"
   val=$(cat "${DATABASE_TEXTDUMP}" | grep "${key}" | awk '{print $2}')
   echo "${val}"

}


add_val_to_db()
{
   key=${1}
   value=${2}
   datatype=${3}

   /usr/local/etc/segw --set-val ${key} ${value} ${datatype}
}




fetch_values_from_old_db()
{
   # ================BROKER 01 ========================================================
   broker_01_login=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000001:BROKER_LOGIN.....:" | awk '{print $2}')
   echo "broker_01_login: $broker_01_login"

   broker_01_address=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000001:900000000201.....:" | awk '{print $2}')
   echo "broker_01_address: $broker_01_address"

   broker_01_port=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000001:900000000301.....:" | awk '{print $2}')
   echo "broker_01_port: $broker_01_port"

   broker_01_reconnect=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000001:900000000601.....:" | awk '{print $2}')
   echo "broker_01_reconnect: $broker_01_reconnect"

   broker_01_enabled=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000001:enabled..........:" | awk '{print $2}')
   echo "broker_01_enabled: $broker_01_enabled"

   broker_01_account=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000001:900000000401.....:" | awk '{print $2}')
   echo "broker_01_account: $broker_01_account"

   broker_01_password=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000001:900000000501....." | awk '{print $2}')
   echo "broker_01_password: $broker_01_password"


   # ================BROKER 02 ========================================================
   broker_02_login=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000002:BROKER_LOGIN.....:" | awk '{print $2}')
   echo "broker_02_login: $broker_02_login"

   broker_02_address=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000002:900000000201.....:" | awk '{print $2}')
   echo "broker_02_address: $broker_02_address"

   broker_02_port=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000002:900000000301.....:" | awk '{print $2}')
   echo "broker_02_port: $broker_02_port"

   broker_02_reconnect=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000002:900000000601.....:" | awk '{print $2}')
   echo "broker_02_reconnect: $broker_02_reconnect"

   broker_02_enabled=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000002:enabled..........:" | awk '{print $2}')
   echo "broker_02_enabled: $broker_02_enabled"
   
   broker_02_account=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000002:900000000401.....:" | awk '{print $2}')
   echo "broker_02_account: $broker_02_account"

   broker_02_password=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_BROKER:900000000002:900000000501....." | awk '{print $2}')
   echo "broker_02_password: $broker_02_password"


   # ================ SERIAL 1 ========================================================
   serial_01_name=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000001:SERIAL_NAME......:" | awk '{print $2}')
   echo "serial_01_name: $serial_01_name"

   serial_01_databits=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000001:910000000201.....:" | awk '{print $2}')
   echo "serial_01_databits: $serial_01_databits"

   serial_01_parity=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000001:910000000301.....:" | awk '{print $2}')
   echo "serial_01_parity: $serial_01_parity"

   serial_01_flowcontrol=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000001:910000000401.....:" | awk '{print $2}')
   echo "serial_01_flowcontrol: $serial_01_flowcontrol"

   serial_01_stopbits=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000001:910000000501.....:" | awk '{print $2}')
   echo "serial_01_stopbits: $serial_01_stopbits"

   serial_01_baudrate=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000001:910000000601.....:" | awk '{print $2}')
   echo "serial_01_baudrate: $serial_01_baudrate"

   # ================ SERIAL 2 ========================================================
   serial_02_name=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000002:SERIAL_NAME......:" | awk '{print $2}')
   echo "serial_02_name: $serial_02_name"

   serial_02_databits=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000002:910000000202.....:" | awk '{print $2}')
   echo "serial_02_databits: $serial_02_databits"

   serial_02_parity=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000002:910000000302.....:" | awk '{print $2}')
   echo "serial_02_parity: $serial_02_parity"

   serial_02_flowcontrol=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000002:910000000402.....:" | awk '{print $2}')
   echo "serial_02_flowcontrol: $serial_02_flowcontrol"

   serial_02_stopbits=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000002:910000000502.....:" | awk '{print $2}')
   echo "serial_02_stopbits: $serial_02_stopbits"

   serial_02_baudrate=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_SERIAL:910000000002:910000000602.....:" | awk '{print $2}')
   echo "serial_02_baudrate: $serial_02_baudrate"

   # ================ NMS ==========================================================
   nms_address=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_NMS:NMS_ADDRESS......................:" | awk '{print $2}')
   echo "nms_address: $nms_address"

   nms_port=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_NMS:NMS_PORT.........................: " | awk '{print $2}')
   echo "nms_port: $nms_port"

   nms_user=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_NMS:NMS_USER.........................: " | awk '{print $2}')
   echo "nms_user: $nms_user"

   nms_pwd=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_NMS:NMS_PWD..........................: " | awk '{print $2}')
   echo "nms_pwd: $nms_pwd"

   nms_enabled=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_NMS:NMS_ENABLED......................: " | awk '{print $2}')
   echo "nms_enabled: $nms_enabled"

   nms_scriptpath=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_NMS:script-path......................: " | awk '{print $2}')
   echo "nms_scriptpath: $nms_scriptpath"

   # ================ wired ==========================================================
   root_director_login=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_REDIRECTOR:930000000002:REDIRECTOR_LOGIN:" | awk '{print $2}')
   echo "root_director_login: $root_director_login"

   root_director_adress=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_REDIRECTOR:930000000002:930000000202.:" | awk '{print $2}')
   echo "root_director_adress: $root_director_adress"

   root_director_port=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_REDIRECTOR:930000000002:930000000302.: " | awk '{print $2}')
   echo "root_director_port: $root_director_port"

   root_director_enabled=$(cat "${DATABASE_TEXTDUMP}" | grep "ROOT_REDIRECTOR:930000000002:enabled......:" | awk '{print $2}')
   echo "root_director_enabled: $root_director_enabled"

   # ================ LMN ==========================================================
   rs485enabled=$(cat "${DATABASE_TEXTDUMP}" | grep "rs485:enabled.............................:" | awk '{print $2}')
   echo "rs485enabled: $rs485enabled"

   rs485protocol=$(cat "${DATABASE_TEXTDUMP}" | grep "rs485:protocol............................:" | awk '{print $2}')
   echo "rs485protocol: $rs485protocol"


   # ================ wmbus ==========================================================
   wmbusenabled=$(cat "${DATABASE_TEXTDUMP}" | grep "IF_wMBUS:enabled..........................:" | awk '{print $2}')
   echo "wmbusenabled: $wmbusenabled"

   # ================ blocklist ==========================================================
   blocklist_enabled=$(cat "${DATABASE_TEXTDUMP}" | grep "IF_wMBUS:blocklist:enabled................:" | awk '{print $2}')
   echo "blocklis_enabled: $blocklist_enabled"

   blocklist_mode=$(cat "${DATABASE_TEXTDUMP}" | grep "IF_wMBUS:blocklist:mode...................: " | awk '{print $2}')
   echo "blocklist_mode: $blocklist_mode"

   blocklist_period=$(cat "${DATABASE_TEXTDUMP}" | grep "IF_wMBUS:blocklist:period.................:" | awk '{print $2}')
   echo "blocklist_period: $blocklist_period"

}





#todo: use function instead of calling commands directly:
#blocklist_period=$(getvalbykey "IF_wMBUS:blocklist:period.................:")
#echo "blocklist_period with function: $blocklist_period"


echo "===== RESTORE DATA ====="



JSON_V08="segw_v0.8.json"
JSON_V09="segw_v0.9.json"
DATABASE="segw.database"

BACKUP_FROM_v08=0
BACKUP_FROM_v09=0

if [ -f ${DATA_PATH}/${JSON_V08}-backup  ]; then
	echo "${PROJECT_NAME}restore previous configuration of segw_v0.8"
   cp ${DATA_PATH}/${JSON_V08}-backup ${DATA_PATH}/${JSON_V08}|| true
   BACKUP_FROM_v08=1
fi

if [ -f ${DATA_PATH}/${JSON_V09}-backup  ]; then
	echo "${PROJECT_NAME} restore previous configuration of segw_v0.9"
   cp ${DATA_PATH}/${JSON_V09}-backup ${DATA_PATH}/${JSON_V09}|| true
   BACKUP_FROM_v09=1
fi


if [ ${BACKUP_FROM_v08} = "0" ] && [ ${BACKUP_FROM_v09} = "0"  ]; then
   echo "no older version of json found, nothing to restore." 
elif [ ${BACKUP_FROM_v08} = "1" ] && [ ${BACKUP_FROM_v09} = "1"  ]; then
   echo "json from version 0.8 and version 0.9 were found. going to ignore the version 0.8" 
   cp ${DATA_PATH}/${DATABASE}-backup ${DATA_PATH}/${DATABASE}
elif [ ${BACKUP_FROM_v08} = "0" ] && [ ${BACKUP_FROM_v09} = "1"  ]; then
   echo "json from version 0.9 found." 
   cp ${DATA_PATH}/${DATABASE}-backup ${DATA_PATH}/${DATABASE}
else
   echo "json from version 0.8 found. Do a conversion from old to new format"
   fetch_values_from_old_db
   echo "fetching values from database done"

   echo "enter values from old databse into new database"
   #/usr/local/sbin/segw --set-val blocklist/0/period ${blocklist_period} chrono:sec

 
   
   add_val_to_db "broker/0/0/account"  ${broker_01_account}    "s"
   add_val_to_db "broker/0/0/address"  ${broker_01_address}    "s"
   add_val_to_db "broker/0/0/port"     ${broker_01_port}       "u16"
   add_val_to_db "broker/0/0/pwd"      ${broker_01_password}   "s"

   add_val_to_db "broker/1/0/account"  ${broker_02_account}    "s"
   add_val_to_db "broker/1/0/address"  ${broker_02_address}    "s"
   add_val_to_db "broker/1/0/port"     ${broker_02_port}       "u16"
   add_val_to_db "broker/1/0/pwd"      ${broker_01_password}   "s"

   add_val_to_db "lmn/0/broker-enabled"  ${broker_01_enabled}  "bool"
   add_val_to_db "lmn/0/broker-login"    ${broker_01_login}   "bool"
   add_val_to_db "lmn/0/databits"        ${serial_01_databits}  "u8" 
   add_val_to_db "lmn/0/enabled"         ${wmbusenabled}     "bool"
   add_val_to_db "lmn/0/flow-control"     ${serial_01_flowcontrol}   "s"
   add_val_to_db "lmn/0/parity"         ${serial_01_parity}  "s"
   add_val_to_db "lmn/0/port"             ${serial_02_name}   "s"
   add_val_to_db "lmn/0/protocol"            "s"
   add_val_to_db "lmn/0/speed"          ${serial_01_baudrate}    "u32"
   add_val_to_db "lmn/0/stopbits"       ${serial_01_stopbits}     "s"
   add_val_to_db "lmn/0/type"       "s"

   add_val_to_db "lmn/1/broker-enabled" ${broker_02_enabled}  "bool"
   add_val_to_db "lmn/1/broker-login"   ${broker_02_login}  "bool"
   add_val_to_db "lmn/1/databits"     ${serial_02_databits} "u8"
   add_val_to_db "lmn/1/enabled"      ${rs485enabled}           "bool"
   add_val_to_db "lmn/1/flow-control"  ${serial_02_flowcontrol}   "s"
   add_val_to_db "lmn/1/parity"      ${serial_02_parity}  "s"
   add_val_to_db "lmn/1/port"      ${serial_02_name} "s"
   add_val_to_db "lmn/1/protocol"   ${rs485protocol}      "s"
   add_val_to_db "lmn/1/speed"           ${serial_02_baudrate} "s"
   add_val_to_db "lmn/1/stopbits"        "${serial_02_stopbits}"   "u32"
   add_val_to_db "lmn/1/type"           "s"

   add_val_to_db "nms/account"      ${nms_user}   "s"
   add_val_to_db "nms/address"    ${nms_address}     "s"
   add_val_to_db "nms/debug"        "bool"
   add_val_to_db "nms/enabled"      ${nms_enabled} "bool"
   add_val_to_db "nms/port"        ${nms_port} "u16"
   add_val_to_db "nms/pwd"          ${nms_pwd}   "s"
   add_val_to_db "nms/script-path"   ${nms_scriptpath} "s"
  
   add_val_to_db "blocklist/0/enabled"  ${blocklist_enabled}   "bool"
   add_val_to_db "blocklist/0/mode"    ${blocklist_mode}       "s"
   add_val_to_db "blocklist/0/period" ${blocklist_period} "chrono:sec"

   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   add_val_to_db
   


   
   
   
   
   
   
   
   
   
   
   
   
   #todo:  other values

fi



