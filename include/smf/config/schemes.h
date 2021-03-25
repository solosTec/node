/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_SCHEMES_H
#define SMF_CONFIG_SCHEMES_H

#include <cyng/store/meta.h>


namespace smf {
	namespace config {


		/**
		 * device/TDevice
		 */
		cyng::meta_store get_store_device();
		cyng::meta_sql get_table_device();

		/**
		 * meter/TMeter
		 */
		cyng::meta_store get_store_meter();
		cyng::meta_sql get_table_meter();

		/**
		 * meterIEC/TMeterIEC
		 */
		cyng::meta_store get_store_meterIEC();
		cyng::meta_sql get_table_meterIEC();

		/**
		 * meterwMBus/TMeterwMBus
		 */
		cyng::meta_store get_store_meterwMBus();
		cyng::meta_sql get_table_meterwMBus();


		/**
		 * gateway/TGateway
		 */
		cyng::meta_store get_store_gateway();
		cyng::meta_sql get_table_gateway();

		/**
		 * loRaDevice/TLoRaDevice
		 */
		cyng::meta_store get_store_lora();
		cyng::meta_sql get_table_lora();

		/**
		 * guiUser/TGuiUser
		 */
		cyng::meta_store get_store_gui_user();
		cyng::meta_sql get_table_gui_user();

		/**
		 * location/TLocation
		 */
		cyng::meta_store get_store_location();
		cyng::meta_sql get_table_location();


		//
		//	store only schemes
		// -----------------------------------------------------------------+
		//

		/**
		 * sessions
		 */
		cyng::meta_store get_store_session();

		/**
		 * push targets
		 */
		cyng::meta_store get_store_target();

		/**
		 * cluster/TCluster
		 */
		cyng::meta_store get_store_cluster();

		/**
		 * super simple config table
		 */
		cyng::meta_store get_config();

		//
		//	uplinks
		//
		cyng::meta_store get_store_uplink_lora();
		cyng::meta_store get_store_uplink_iec();
		cyng::meta_store get_store_uplink_wmbus();


		/**
		 * container to store meta data of in-memory tables
		 */
		using store_map = std::map<std::string, cyng::meta_store>;

		/**
		 * container to store meta data of SQL tables
		 */
		using sql_map = std::map<std::string, cyng::meta_sql>;
	}
}

#endif
