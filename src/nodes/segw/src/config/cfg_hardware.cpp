/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_hardware.h>

 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	cfg_hardware::cfg_hardware(cfg& c)
		: cfg_(c)
	{}

	namespace {
		std::string serial_path() {
			return cyng::to_path(cfg::sep, cfg_hardware::root, "serial");
		}
		std::string model_path() {
			return cyng::to_path(cfg::sep, cfg_hardware::root, "model");
		}
		std::string manufacturer_path() {
			return cyng::to_path(cfg::sep, cfg_hardware::root, "manufacturer");
		}
	}

	std::string cfg_hardware::get_model() const {
		return cfg_.get_value(model_path(), "model");
	}

	std::uint32_t cfg_hardware::get_serial_number() const {
		return cfg_.get_value(serial_path(), 10000000u);
	}

	std::string cfg_hardware::get_manufacturer() const {
		return cfg_.get_value(manufacturer_path(), "solosTec");
	}


}