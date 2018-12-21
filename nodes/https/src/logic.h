/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTPS_LOGIC_H
#define NODE_HTTPS_LOGIC_H

#include <smf/https/srv/server.h>
#include <cyng/vm/context.h>

namespace node 
{
	namespace https
	{
		class logic
		{
		public:
			logic(server&, cyng::controller&, cyng::logging::log_ptr);

		private:
			void ws_read(cyng::context& ctx);

		private:
			server& srv_;
			cyng::logging::log_ptr logger_;
		};
	}
}

#endif
