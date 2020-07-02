/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MODEM_SERVER_H
#define NODE_MODEM_SERVER_H

#include <smf/cluster/server_stub.h>

namespace node 
{
	namespace modem
	{
		class server : public server_stub
		{
		public:
			server(cyng::async::mux&
				, cyng::logging::log_ptr logger
				, bus::shared_type
				, std::chrono::seconds timeout
				, std::set<boost::asio::ip::address> const& blacklist
				, bool auto_answer
				, std::chrono::milliseconds guard_time);

		protected:
			virtual cyng::object make_client(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket) override;
			virtual bool close_connection(boost::uuids::uuid tag, cyng::object) override;
			virtual void start_client(cyng::object) override;
			virtual void propagate(cyng::object, cyng::vector_t&& msg) override;

		private:
			 //	configuration
			 const bool auto_answer_;
			 const std::chrono::milliseconds guard_time_;
		};
	}
}

#endif

