/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <tasks/client.h>
#include <smf/cluster/generator.h>

#include <cyng/vm/controller.h>
#include <cyng/io/io_bytes.hpp>

namespace node
{
	client::client(cyng::async::base_task* btp
		, cyng::controller& cluster
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, boost::asio::ip::tcp::endpoint ep
		, std::chrono::seconds monitor)
	: base_(*btp)
		, cluster_(cluster)
		, logger_(logger)
		, cache_(db)
		, ep_(ep)
		, monitor_(monitor)
		, socket_(base_.mux_.get_io_service())
		, buffer_read_()
		, buffer_write_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< ep_);

		reset_write_buffer();
	}

	cyng::continuation client::run()
	{	
		//
		//	start/restart timer
		//
		base_.suspend(monitor_);

		if (!socket_.is_open()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> open connection to "
				<< ep_);

			try {

				boost::asio::ip::tcp::resolver resolver(base_.mux_.get_io_service());
				auto const endpoints = resolver.resolve(ep_);
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


		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation client::process()
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void client::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	void client::do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints)
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

					cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
						, "connected"
						, ep
						, cluster_.tag()));

					//
					//	send query
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
						<< ep_
						<< ' '
						<< ec.message());

					cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
						, "connect failed"
						, ep_
						, cluster_.tag()));

				}
			});
	}

	void client::do_read()
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
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< ep_
						<< ' '
						<< ec.message());

					//
					//	socket should already be closed
					//
					socket_.close(ec);

					reset_write_buffer();

				}

			});

	}

	void client::do_write()
	{
		if (!buffer_write_.empty()) {
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
						//
						//	no more data will be send
						//
						socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
					}
				});
		}
	}

	void client::reset_write_buffer()
	{
		buffer_write_.clear();
		std::string const hello("/?!\r\n");
		buffer_write_.emplace_back(cyng::buffer_t(hello.begin(), hello.end()));
	}

}
