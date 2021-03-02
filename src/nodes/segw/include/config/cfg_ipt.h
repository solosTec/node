/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_IPT_H
#define SMF_SEGW_CONFIG_IPT_H

#include <cfg.h>

 namespace smf {

	 class cfg_ipt
	 {
	 public:
		 cfg_ipt(cfg&);
		 cfg_ipt(cfg_ipt&) = default;

		 bool is_enabled() const;

		 static const std::string root;
	 private:
		 cfg& cfg_;
	 };

}

#endif