#
# generate configuration files for windows setup
#
#

# http
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/http.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/http_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/http_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/http_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/http_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/http_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/http_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/http_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/http.rc.in"
		"${PROJECT_BINARY_DIR}/http.rc")

# https
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/https.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/https_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/https_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/https_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/https_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/https_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/https_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/https_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/https.rc.in"
		"${PROJECT_BINARY_DIR}/https.rc")

# dash
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dash.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/dash_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dash_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/dash_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dash_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/dash_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dash_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/dash_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dash.rc.in"
		"${PROJECT_BINARY_DIR}/dash.rc")

# dashs
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dashs.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/dashs_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dashs_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/dashs_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dashs_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/dashs_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dashs_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/dashs_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dashs.rc.in"
		"${PROJECT_BINARY_DIR}/dashs.rc")

# e355
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/e350_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/e350_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/e350_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/e350_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.rc.in"
		"${PROJECT_BINARY_DIR}/e350.rc")

# ipt collector 
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/collector_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/collector_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/collector_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/collector_restart_service.cmd")

# ipt emitter 
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/emitter_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/emitter_create_service.cmd")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/emitter_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/emitter_restart_service.cmd")

# ipt gateway
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/gateway_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/gateway_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/gateway_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/gateway_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway.rc.in"
		"${PROJECT_BINARY_DIR}/gateway.rc")

# ipt segw
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/segw_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/segw_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/segw_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/segw_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw.rc.in"
		"${PROJECT_BINARY_DIR}/segw.rc")

# ipt master
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/ipt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/ipt_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/ipt_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/ipt_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.rc.in"
		"${PROJECT_BINARY_DIR}/ipt.rc")

# ipts master
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipts.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/ipts_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipts_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/ipts_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipts_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/ipts_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipts_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/ipts_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipts.rc.in"
		"${PROJECT_BINARY_DIR}/ipts.rc")

# ipt store
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/store_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/store_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/store_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/store_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.rc.in"
		"${PROJECT_BINARY_DIR}/store.rc")

# lora
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/lora_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/lora_create_service.cmd")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/lora_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/lora_restart_service.cmd")

# master
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/master/templates/master.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/master_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/master/templates/master_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/master_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/master/templates/master_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/master_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/master/templates/master_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/master_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/master/templates/master.rc.in"
		"${PROJECT_BINARY_DIR}/master.rc")

# modem
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/modem_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/modem_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/modem_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/modem_restart_service.cmd")

# mqtt
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/mqtt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/mqtt_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/mqtt_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/mqtt_restart_service.cmd")

# setup
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.windows.cgf.in"
		"${PROJECT_BINARY_DIR}/config/setup_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_create_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/setup_create_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_delete_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/setup_delete_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_restart_service.cmd.in"
		"${PROJECT_BINARY_DIR}/config/setup_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.rc.in"
		"${PROJECT_BINARY_DIR}/setup.rc")

    # csv
    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.windows.cgf.in"
        "${PROJECT_BINARY_DIR}/config/csv_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv_create_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/csv_create_service.cmd")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv_delete_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/csv_delete_service.cmd")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv_restart_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/csv_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.rc.in"
		"${PROJECT_BINARY_DIR}/csv.rc")

    # stat
    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/stat/templates/stat.windows.cgf.in"
        "${PROJECT_BINARY_DIR}/config/stat_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/stat/templates/stat_create_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/stat_create_service.cmd")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/stat/templates/stat_delete_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/stat_delete_service.cmd")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/stat/templates/stat_restart_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/stat_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/tasks/stat/templates/stat.rc.in"
		"${PROJECT_BINARY_DIR}/stat.rc")


    # tsdb
    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/tsdb/templates/tsdb.windows.cgf.in"
        "${PROJECT_BINARY_DIR}/config/tsdb_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/tsdb/templates/tsdb_create_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/tsdb_create_service.cmd")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/tsdb/templates/tsdb_delete_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/tsdb_delete_service.cmd")

    configure_file (
        "${PROJECT_SOURCE_DIR}/tasks/tsdb/templates/tsdb_restart_service.cmd.in"
        "${PROJECT_BINARY_DIR}/config/tsdb_restart_service.cmd")

configure_file (
		"${PROJECT_SOURCE_DIR}/tasks/tsdb/templates/tsdb.rc.in"
		"${PROJECT_BINARY_DIR}/tsdb.rc")
