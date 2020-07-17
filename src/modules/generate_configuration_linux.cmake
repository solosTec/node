#
# generate configuration files for linux setup
#
#

# http
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/http.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/http_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/http.service.in"
		"${PROJECT_BINARY_DIR}/config/node-http.service")

# https
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/https.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/https_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/www/templates/https.service.in"
		"${PROJECT_BINARY_DIR}/config/node-https.service")

# dash
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dash.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/dash_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dash.service.in"
		"${PROJECT_BINARY_DIR}/config/node-dash.service")

# dashs
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dashs.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/dashs_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/admin/templates/dashs.service.in"
		"${PROJECT_BINARY_DIR}/config/node-dashs.service")

# e355
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/e350_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.service.in"
		"${PROJECT_BINARY_DIR}/config/node-e350.service")

# ipt collector 
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/collector_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector.service.in"
		"${PROJECT_BINARY_DIR}/config/node-collector.service")

# ipt emitter 
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/emitter_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter.service.in"
		"${PROJECT_BINARY_DIR}/config/node-emitter.service")
	  
# ipt segw
if (${${PROJECT_NAME}_CROSS_COMPILE})
	configure_file (
			"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw.oecp.cgf.in"
			"${PROJECT_BINARY_DIR}/config/segw_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
			
	configure_file (
			"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/oecp.service.in"
			"${PROJECT_BINARY_DIR}/config/segw.service")
else()
	configure_file (
			"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw.linux.cgf.in"
			"${PROJECT_BINARY_DIR}/config/segw_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
			
	configure_file (
			"${PROJECT_SOURCE_DIR}/nodes/ipt/segw/templates/segw.service.in"
			"${PROJECT_BINARY_DIR}/config/node-segw.service")
			
endif()
	  

# ipt master
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/ipt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.service.in"
		"${PROJECT_BINARY_DIR}/config/node-ipt.service")

# ipts master
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipts.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/ipts_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipts.service.in"
		"${PROJECT_BINARY_DIR}/config/node-ipts.service")

# ipt store
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/store_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.service.in"
		"${PROJECT_BINARY_DIR}/config/node-store.service")

# lora
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/lora_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora.service.in"
		"${PROJECT_BINARY_DIR}/config/node-lora.service")
	  
# master
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/master/templates/master.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/master_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/master/templates/master.service.in"
		"${PROJECT_BINARY_DIR}/config/node-master.service")

# modem
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/modem_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem.service.in"
		"${PROJECT_BINARY_DIR}/config/node-modem.service")

# mqtt
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/mqtt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt.service.in"
		"${PROJECT_BINARY_DIR}/config/node-mqtt.service")


# setup
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/setup_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.service.in"
		"${PROJECT_BINARY_DIR}/config/node-setup.service")
	  
# csv
configure_file (
    "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.linux.cgf.in"
    "${PROJECT_BINARY_DIR}/config/csv_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

configure_file (
    "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.service.in"
    "${PROJECT_BINARY_DIR}/config/task-csv.service")

# iec_62056
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/broker/iec-62056/templates/iec_62056.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/iec_62056_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/broker/iec-62056/templates/iec_62056.service.in"
		"${PROJECT_BINARY_DIR}/config/node-iec_62056.service")

# iec_62056
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/broker/mbus-radio/templates/mbus_radio.linux.cgf.in"
		"${PROJECT_BINARY_DIR}/config/mbus_radio_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
		"${PROJECT_SOURCE_DIR}/nodes/broker/mbus-radio/templates/mbus_radio.service.in"
		"${PROJECT_BINARY_DIR}/config/node-mbus_radio.service")
