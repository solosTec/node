/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_BLOCKLIST_H
#define SMF_SEGW_CONFIG_BLOCKLIST_H

#include <config/cfg_lmn.h>
#include <vector>
#include <ostream>

 namespace smf {
	
	 class cfg_blocklist
	 {
	 public:
		 cfg_blocklist(cfg&, lmn_type);
		 cfg_blocklist(cfg_blocklist&) = default;

		 /**
		  * A broker without an active LMN is virtually useless.
		  * This method helps to detect such a misconfiguration.
		  */
		 bool is_lmn_enabled() const;

		 /**
		  * numerical value of the specified LMN enum type
		  */
		 constexpr std::uint8_t get_index() const {
			 return static_cast<std::uint8_t>(type_);
		 }

		 std::string get_path_id() const;

		 bool is_enabled() const;
		 std::string get_mode() const;
		 std::vector<std::string> get_list() const;
		 std::size_t size() const;

		 bool set_enabled(bool) const;
		 bool set_mode(std::string) const;
		 bool set_list(std::vector<std::string> const&) const;

		 constexpr static char root[] = "blocklist";

	 private:
		 cfg& cfg_;
		 lmn_type const type_;
	 };
}

#endif