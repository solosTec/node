/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "broker.h"
#include "../cache.h"

#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/compatibility.h>

#include <iostream>

namespace node
{
	broker::broker(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, cache& cfg
		, std::string host
		, std::uint16_t port)
	: base_(*btp) 
		, logger_(logger)
		, cfg_(cfg)
		, host_(host)
		, port_(port)
		, stream_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< host_
			<< ':'
			<< port_);
	}

	cyng::continuation broker::run()
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
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> cannot connect to "
					<< host_
					<< ':'
					<< port_);
			}
		}

		//
		//	start/restart timer
		//
		base_.suspend(std::chrono::minutes(1));

		return cyng::continuation::TASK_CONTINUE;
	}

	void broker::stop(bool shutdown)
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

	cyng::continuation broker::process(cyng::buffer_t data, std::size_t msg_counter)
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
		if (stream_.socket().is_open()) {
			stream_.write(data.data(), data.size());
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	bool broker::connect()
	{
		//
		//	try to connect
		//
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> open connection to "
			<< host_
			<< ':'
			<< port_);

		try {
			stream_.connect(host_, std::to_string(port_));
			if (stream_.fail())
			{
				stream_.socket().close();
				return false;
			}
			return true;
		}
		catch (std::exception const& ex) {
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> open connection failed "
				<< ex.what());
		}
		return false;
	}

}

