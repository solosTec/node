/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MQTT_SERVER_H
#define NODE_MQTT_SERVER_H

#include <smf/cluster/bus.h>

#include <mqtt/tcp_endpoint.hpp>
#include <mqtt/endpoint.hpp>

#include <cyng/async/mux.h>
#include <cyng/log.h>

#include <memory>
#include <set>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace node 
{
	class server
	{
		//  import mqtt classes
        using socket_t = mqtt::tcp_endpoint<boost::asio::ip::tcp::socket, boost::asio::io_context::strand>;
		using endpoint_t = mqtt::endpoint<socket_t, std::mutex, std::lock_guard>;
		using shared_ep = std::shared_ptr<endpoint_t>;
//  		using accept_handler = std::function<void(endpoint_t& ep)>;

		struct sub_con {
			sub_con(std::string const& topic, shared_ep const& con, std::uint8_t qos)
			:topic(topic), con(con), qos(qos) {}
			std::string topic;
			shared_ep con;
			std::uint8_t qos;
		};
		
		struct tag_topic {};
		struct tag_con {};

		using mi_sub_con = boost::multi_index::multi_index_container<
			sub_con,
			boost::multi_index::indexed_by<
				boost::multi_index::ordered_non_unique<
					boost::multi_index::tag<tag_topic>,
					BOOST_MULTI_INDEX_MEMBER(sub_con, std::string, topic)
				>,
				boost::multi_index::ordered_non_unique<
					boost::multi_index::tag<tag_con>,
					BOOST_MULTI_INDEX_MEMBER(sub_con, shared_ep, con)
				>
			>
		>;

	public:
		server(cyng::async::mux&
			, cyng::logging::log_ptr logger
			, bus::shared_type);

		/**
		 * start listening
		 */
		void run(std::string const&, std::string const&);

		/**
		 * close acceptor
		 */
		void close();

	private:
		/**
		 * Perform an asynchronous accept operation.
		 */
		void do_accept();

		void set_callbacks(shared_ep);
		void close_proc(shared_ep con);
		
	private:
		/*
		 * task manager and running I/O context
		 */
		cyng::async::mux& mux_;

		/**
		 * The logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * cluster bus
		 */
		bus::shared_type bus_;

		/*
		 * Acceptor used to listen for incoming connections.
		 */
		boost::asio::ip::tcp::acceptor acceptor_;

		std::unique_ptr<socket_t> socket_;
		
		/**
		 * connection management
		 */
		std::set<shared_ep> connections;
		mi_sub_con subs;
		
	};
}

#endif

