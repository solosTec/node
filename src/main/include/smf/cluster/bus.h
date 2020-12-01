/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_CLUSTER_BUS_H
#define NODE_CLUSTER_BUS_H

#include <NODE_project_info.h>
#include <smf/cluster/serializer.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <cyng/io/parser/parser.h>
#include <array>

namespace node
{
	class bus : public std::enable_shared_from_this<bus>
	{
	public:
		using shared_type = std::shared_ptr<bus>;

	public:
		bus(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, std::size_t tsk);

		bus(bus const&) = delete;
		bus& operator=(bus const&) = delete;

		void start();
        void stop();

		/**
		 * @return true if online with master
		 */
		bool is_online() const;

	private:
		void do_read();

	public:
		/**
		 * Interface to cyng VM
		 */
		cyng::controller vm_;

	private:
		/**
		 * connection socket
		 */
		boost::asio::ip::tcp::socket socket_;
	
		/**
		 * Buffer for incoming data.
		 */
		std::array<char, NODE::PREFERRED_BUFFER_SIZE> buffer_;

		/**
		 * task multiplexer 
		 */
		cyng::async::mux& mux_;

		/**
		 * The logger instance
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * task to signal a change in the bus state
		 */
		std::size_t const task_;

		/**
		 * Parser for binary cyng data stream (from cluster master)
		 */
		cyng::parser 	parser_;

		/**
		 * wrapper class to serialize and send
		 * data and code.
		 */
		serializer		serializer_;

		/**
		 * session state
		 */
		enum class state {
			ERROR_,
			INITIAL_,
			AUTHORIZED_,
			SHUTDOWN_,
		} state_;

		/**
		 * master session tag
		 */
		boost::uuids::uuid remote_tag_;
		cyng::version	remote_version_;

		/**
		 * cluster message sequence
		 */
		std::uint64_t seq_;
	};

	/**
	 * factory method for cluster bus
	 */
	bus::shared_type bus_factory(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::size_t tsk);

}

#endif
