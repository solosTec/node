/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_SEGW_CONTROLLER_H
#define SMF_SEGW_CONTROLLER_H

#include <smf/controller_base.h>
#include <cyng/db/session.h>
#include <cyng/log/logger.h>

namespace cyng {
	class controller;
}

 namespace smf {

	class controller : public config::controller_base
	{
	public:
		controller(config::startup const&);
		virtual bool run_options(boost::program_options::variables_map&) override;

	protected:
		cyng::vector_t create_default_config(std::chrono::system_clock::time_point&& now
			, std::filesystem::path&& tmp
			, std::filesystem::path&& cwd) override;

		void run(cyng::controller&, cyng::logger, cyng::object const& cfg) override;

	private:
		cyng::tuple_t create_wireless_spec(std::string const& hostname) const;
		cyng::tuple_t create_rs485_spec(std::string const& hostname) const;
		cyng::param_t create_wireless_broker(std::string const& hostname) const;
		cyng::param_t create_wireless_block_list() const;
		cyng::param_t create_rs485_broker(std::string const& hostname) const;
		cyng::param_t create_rs485_listener() const;
		cyng::param_t create_gpio_spec() const;
		cyng::param_t create_hardware_spec() const;
		cyng::param_t create_nms_server_spec(std::filesystem::path const&) const;
		cyng::param_t create_sml_server_spec() const;
		cyng::param_t create_db_spec(std::filesystem::path const&) const;
		cyng::param_t create_ipt_spec(std::string const& hostname) const;
		cyng::param_t create_ipt_config(std::string const& hostname) const;
		cyng::param_t create_ipt_params() const;
		cyng::param_t create_virtual_meter_spec() const;
		cyng::param_t create_lmn_spec(std::string const& hostname) const;
		cyng::param_t create_server_id() const;

		void init_storage(cyng::object&&);
		void transfer_config(cyng::object&&);
		void clear_config(cyng::object&&);
		void list_config(cyng::object&&);
		void set_config_value(cyng::object&&, std::string const& path, std::string const& value, std::string const& type);
		void switch_gpio(cyng::object&& cfg, std::string const& number, std::string const& state);
	};
}

#endif