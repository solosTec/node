/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_GPIO_H
#define SMF_SEGW_CONFIG_GPIO_H

#include <cfg.h>
#include <vector>
#include <filesystem>

 namespace smf {

	 class cfg_gpio
	 {
	 public:
		 cfg_gpio(cfg&);
		 cfg_gpio(cfg_gpio&) = default;

		 bool is_enabled() const;

		 /**
		  * @return a list of all GPIO IDs
		  */
		 std::vector<std::uint32_t> get_pins() const;

		 std::filesystem::path get_path() const;
		 std::filesystem::path get_path(std::uint32_t pin) const;

		 static std::string get_name(std::uint32_t pin);

	 private:
		 cfg& cfg_;
	 };

}

#endif