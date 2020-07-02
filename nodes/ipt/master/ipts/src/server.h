/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_SERVER_H
#define NODE_IPT_MASTER_SERVER_H

#include <smf/cluster/server_stub.h>
#include <smf/ipt/scramble_key.h>

namespace node 
{
	namespace ipt
	{
		class server : public server_stub
		{
		public:
			server(cyng::async::mux&
				, cyng::logging::log_ptr logger
				, bus::shared_type 
				, std::chrono::seconds timeout
				, scramble_key const& sk
				, uint16_t watchdog
				, std::set<boost::asio::ip::address> const&
				, bool sml_log);

		protected:
			virtual cyng::object make_client(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket) override;
			virtual bool close_connection(boost::uuids::uuid tag, cyng::object) override;
			virtual void start_client(cyng::object) override;
			virtual void propagate(cyng::object, cyng::vector_t&& msg) override;

		private:
			 //	configuration
			 scramble_key const sk_;
			 std::uint16_t const watchdog_;
			 bool const sml_log_;
		};
	}
}

#endif

