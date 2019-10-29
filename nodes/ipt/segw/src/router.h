/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_ROUTER_H
#define NODE_IPT_SEGW_ROUTER_H

#include <smf/sml/protocol/generator.h>

#include <cyng/store/db.h>
#include <cyng/log.h>

namespace cyng
{
	class controller;
	class context;
}

namespace node
{
	/**
	 * Central message dispatcher 
	 */
	class storage;
	class cache;
	class router
	{

	public:
		router(cyng::logging::log_ptr
			, bool const server_mode
			, cyng::controller& vm
			, cache&);


	private:
		void sml_msg(cyng::context& ctx);

		/**
		 * End Of Message
		 */
		void sml_eom(cyng::context& ctx);

	private:
		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * Working in client (as session) or server (customer interface)
		 */
		bool const server_mode_;

		/**
		 * global data cache
		 */
		cache& cache_;

		/**
		 * buffer for current SML message
		 */
		sml::res_generator sml_gen_;

	};


}
#endif
