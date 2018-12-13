/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_E350_SERVER_H
#define NODE_E350_SERVER_H

#include <smf/cluster/server_stub.h>

namespace node 
{
	namespace imega
	{
		class server : public server_stub
		{
		public:
			server(cyng::async::mux&
				, cyng::logging::log_ptr logger
				, bus::shared_type
				, std::chrono::seconds timeout
				, std::string pwd_policy
				, std::string const& global_pwd);

		protected:
			virtual cyng::object make_client(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket) override;
			virtual bool close_connection(boost::uuids::uuid tag, cyng::object) override;
			virtual void start_client(cyng::object) override;
			virtual void propagate(cyng::object, cyng::vector_t&& msg) override;

		private:
			 //	configuration
			 std::string const pwd_policy_;
			 std::string const global_pwd_;
		};
	}
}

#endif

