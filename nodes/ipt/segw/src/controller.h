/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_CONTROLLER_H
#define NODE_IPT_SEGW_CONTROLLER_H

#include <smf/shared/ctl.h>

namespace node
{
	class cache;
	class controller : public ctl
	{
	public:
		/**
		 * @param index index of the configuration vector (mostly == 0)
		 * @param pool_size thread pool size
		 * @param json_path path to JSON configuration file
		 */
		controller(unsigned int index
			, unsigned int pool_size
			, std::string const& json_path
			, std::string node_name);

		int transfer_config() const;
		int list_config(std::ostream& os) const;
		int clear_config() const;
		int dump_profile(std::uint32_t) const;

		int set_value(std::string) const;
		int try_connect(std::string) const;

	protected:
		virtual bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag);
		virtual cyng::vector_t create_config(std::fstream&, cyng::filesystem::path&& tmp, cyng::filesystem::path&& cwd) const;
		virtual int prepare_config_db(cyng::param_map_t&&, cyng::object);

	private:
		boost::asio::ip::tcp::endpoint get_sml_ep(cache&) const;
		boost::asio::ip::tcp::endpoint get_nms_ep(cache&) const;
		boost::asio::ip::tcp::endpoint get_redirctor_ep(cache&, std::uint8_t) const;
	};
}
#endif
