/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MBUS_SEGW_CONFIG_H
#define NODE_MBUS_SEGW_CONFIG_H

#include <string>
#include <chrono>
#include <cstdint>

namespace node
{

	class cache;

	/**
	 * wired m-bus configuration
	 */
	class cfg_mbus
	{
	public:
		cfg_mbus(cache&);

		/**
		 * rs485:enabled"
		 * @return "rs485:enabled"
		 */
		bool is_enabled() const;

		/**
		 * @return "OBIS_CLASS_MBUS:OBIS_CLASS_MBUS_RO_INTERVAL"
		 */
		std::chrono::seconds get_readout_interval() const;

		/**
		 * @return "OBIS_CLASS_MBUS:OBIS_CLASS_MBUS_SEARCH_INTERVAL"
		 */
		std::chrono::seconds get_search_interval() const;

		/**
		 * @brief search device now and by restart
		 *
		 * @return "OBIS_CLASS_MBUS:OBIS_CLASS_MBUS_SEARCH_DEVICE"
		 */
		bool get_search_device() const;

		/**
		 * @brief automatic activation of meters 
		 *
		 * @return "OBIS_CLASS_MBUS:OBIS_CLASS_MBUS_AUTO_ACTICATE"
		 */
		bool is_auto_activation() const;

		bool generate_profile() const;

	private:
		cache& cache_;
	};
}
#endif
