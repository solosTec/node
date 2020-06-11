/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_STORE_CONTROLLER_H
#define NODE_IPT_STORE_CONTROLLER_H

#include <smf/shared/ctl.h>

namespace node
{
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
		int init_db(std::size_t config_index);
		int create_influx_dbs(std::size_t config_index, std::string cmd);

	protected:
		virtual bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag);
		virtual cyng::vector_t create_config(std::fstream&, cyng::filesystem::path&& tmp, cyng::filesystem::path&& cwd) const;
	};
}

#endif
