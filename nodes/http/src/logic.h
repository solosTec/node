/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_LOGIC_H
#define NODE_HTTP_LOGIC_H

#include <smf/http/srv/server.h>
#include <cyng/vm/context.h>

namespace node 
{
	namespace http
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
