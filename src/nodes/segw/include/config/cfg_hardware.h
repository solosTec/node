/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_HARDWARE_H
#define SMF_SEGW_CONFIG_HARDWARE_H

#include <cfg.h>

 namespace smf {

	 class cfg_hardware
	 {
	 public:
		 cfg_hardware(cfg&);
		 cfg_hardware(cfg_hardware&) = default;

		 std::string get_model() const;
		 std::uint32_t get_serial_number() const;
		 std::string get_manufacturer() const;

		 constexpr static char root[] = "hw";
	 private:
		 cfg& cfg_;
	 };

}

#endif