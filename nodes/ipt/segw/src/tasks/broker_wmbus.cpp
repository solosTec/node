﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "broker_wmbus.h"
#include "../cache.h"

#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>

#include <iostream>

namespace node
{
	broker_wmbus::broker_wmbus(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, cache& cfg)
	: base_(*btp) 
		, logger_(logger)
		, cfg_(cfg)
		, stream_()
		//, socket_(base_.mux_.get_io_service())
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation broker_wmbus::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		if (!stream_.socket().is_open()) {

			//
			//	open connection
			//
			if (connect()) {

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> is connected to "
					<< stream_.socket().remote_endpoint());
			}
		}

		//
		//	start/restart timer
		//
		base_.suspend(std::chrono::minutes(1));

		return cyng::continuation::TASK_CONTINUE;
	}

	void broker_wmbus::stop(bool shutdown)
	{
		//
		//  close socket
		//
		stream_.close();

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation broker_wmbus::process(cyng::buffer_t data, std::size_t msg_counter)
	{

		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received msg #"
			<< msg_counter
			<< " with "
			<< cyng::bytes_to_str(data.size()));

		//
		//  send to receiver
		//
		stream_.write(data.data(), data.size());

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation broker_wmbus::process(bool b)
	{
		//cache_.set_status_word(sml::STATUS_BIT_MBUS_IF_AVAILABLE, b);

		//if (b) {
		//	CYNG_LOG_TRACE(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> wireless M-Bus interface available");
		//}
		//else {
		//	CYNG_LOG_WARNING(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> wireless M-Bus interface closed");
		//}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	bool broker_wmbus::connect()
	{
		//
		//	get configuration
		//
		auto const host = cfg_.get_broker_address();
		auto const service = std::to_string(cfg_.get_broker_port());

		//
		//	resolve address
		//
		//boost::asio::ip::tcp::resolver resolver(base_.mux_.get_io_service());
		//auto const results = resolver.resolve(host, service);

		//boost::asio::connect(socket_, results);
		//return socket_.is_open();
		stream_.connect(host, service);
		return stream_.socket().is_open();
	}

}
