/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_CONTROLLER_H
#define NODE_HTTP_CONTROLLER_H

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

		/**
		 * @param idx index of configuration vector
		 * @param conf_count number of default devices to insert
		 */
		int init_db(std::size_t idx, std::size_t conf_count);

		/**
		 * @param idx index of configuration vector
		 * @param conf_count number of default devices to insert
		 */
		int generate_access_rights(std::size_t idx, std::size_t conf_count, std::string user);

	protected:
		virtual bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag);
		virtual cyng::vector_t create_config(std::fstream&, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const;
	};
}
#endif
