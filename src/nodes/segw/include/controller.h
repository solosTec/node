/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/controller_base.h>


 namespace smf {

	class controller : public config::controller_base
	{
	public:
		controller(config::startup const&);
		virtual int run() override;

	protected:
		cyng::vector_t create_default_config(std::chrono::system_clock::time_point&& now
			, std::filesystem::path&& tmp
			, std::filesystem::path&& cwd) override;
		void print_configuration(std::ostream&) override;

	private:
		cyng::param_t create_segw_wireless_spec() const;
		cyng::param_t create_segw_rs485_spec() const;
		cyng::param_t create_gpio_spec() const;
		cyng::param_t create_hardware_spec() const;
		cyng::param_t create_nms_server_spec(std::filesystem::path const&) const;
		cyng::param_t create_sml_server_spec() const;
		cyng::param_t create_db_spec(std::filesystem::path const&);
		cyng::param_t create_ipt_spec(std::string const& hostname);
		cyng::param_t create_ipt_params();

	};
}