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
		, std::string account
		, std::string pwd
		, std::string host
		, std::uint16_t port)
	: base_(*btp) 
		, logger_(logger)
		, cfg_(cfg)
		, account_(account)
		, pwd_(pwd)
		, host_(host)
		, port_(port)
		, socket_(base_.mux_.get_io_service())
		, buffer_read_()
		, buffer_write_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< account_
			<< ':'
			<< pwd_
			<< '@'
			<< host_
			<< ':'
			<< port_);

		//
		//	login message
		//
		reset_write_buffer();
	}

	void broker::reset_write_buffer()
	{
		buffer_write_.clear();
		buffer_write_.emplace_back(cyng::buffer_t(account_.begin(), account_.end()));
		buffer_write_.emplace_back(cyng::buffer_t(1, ':'));
		buffer_write_.emplace_back(cyng::buffer_t(pwd_.begin(), pwd_.end()));
		buffer_write_.emplace_back(cyng::buffer_t(1, '\n'));
	}

	cyng::continuation broker::run()
	{
		if (!socket_.is_open()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> open connection to "
				<< host_
				<< ':'
				<< port_);

			try {

				boost::asio::ip::tcp::resolver resolver(base_.mux_.get_io_service());
				auto const endpoints = resolver.resolve(host_, std::to_string(port_));
				do_connect(endpoints);
			}
			catch (std::exception const& ex) {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< ex.what());
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
		try {
			//
			//  close socket
			//
			socket_.close();
		}
		catch (std::exception const&) {}

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

		buffer_write_.push_back(data);

		//
		//  send to receiver
		//
		if (socket_.is_open()) {
			do_write();
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void broker::do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints)
	{
		boost::asio::async_connect(socket_, endpoints,
			[this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint ep)
			{
				if (!ec) {
					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> connected to "
						<< ep);

					//
					//	send login
					//
					do_write();

					//
					//	start reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< host_
						<< ':'
						<< port_
						<< ' '
						<< ec.message());

				}
		});
	}

	void broker::do_read()
	{
		socket_.async_read_some(boost::asio::buffer(buffer_read_.data(), buffer_read_.size()),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec) {

					//
					//	ToDo: forward to serial interface
					//

					//
					//	continue reading
					//
					do_read();

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> received "
						<< cyng::bytes_to_str(bytes_transferred));
				}
				else
				{
					socket_.close();
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< host_
						<< ':'
						<< port_
						<< ' '
						<< ec.message());

					reset_write_buffer();

				}

			});
			
	}

	void broker::do_write()
	{
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					buffer_write_.pop_front();
					if (!buffer_write_.empty())
					{
						do_write();
					}
				}
				else
				{
					socket_.close();
				}
		});
	}

}

