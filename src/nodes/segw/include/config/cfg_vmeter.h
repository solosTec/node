/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_VMETER_H
#define SMF_SEGW_CONFIG_VMETER_H

#include <cfg.h>

 namespace smf {

	 class cfg_vmeter
	 {
	 public:
		 cfg_vmeter(cfg&);
		 cfg_vmeter(cfg_vmeter&) = default;

		 bool is_enabled() const;
		 std::string get_server() const;
		 std::string get_protocol() const;
		 std::size_t get_port_index() const;

		 constexpr static char root[] = "vmeter";
	 private:
		 cfg& cfg_;
	 };

}

#endif