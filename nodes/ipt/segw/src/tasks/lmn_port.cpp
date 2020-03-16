/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "lmn_port.h"
#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>

#include <cyng/io/hex_dump.hpp>
#include <cyng/factory/set_factory.h>

namespace node
{
	lmn_port::lmn_port(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::chrono::seconds monitor
		, std::string port
		, boost::asio::serial_port_base::character_size databits
		, boost::asio::serial_port_base::parity parity
		, boost::asio::serial_port_base::flow_control flow_control
		, boost::asio::serial_port_base::stop_bits stopbits
		, boost::asio::serial_port_base::baud_rate speed
		, std::size_t receiver_data
		, std::size_t receiver_status)
	: base_(*btp) 
		, logger_(logger)
		, port_(btp->mux_.get_io_service())
		, monitor_(monitor)
		, name_(port)
		, databits_(databits)
		, parity_(parity)
		, flow_control_(flow_control)
		, stopbits_(stopbits)
		, baud_rate_(speed)
		, receiver_data_(receiver_data)
		, receiver_status_(receiver_status)
		, buffer_()
		, msg_counter_(0u)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< port);
	}

	cyng::continuation lmn_port::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		//
		//	see: https://github.com/boostorg/asio/issues/280
		//	patch boost_1_71_0\boost\asio\detail\impl\win_iocp_serial_port_service.ipp line #87 with:
		//	dcb.BaudRate = 9600; // 0 baud by default
		//
		if (!port_.is_open()) {

			boost::system::error_code ec;
			port_.open(name_, ec);
			if (!ec) {
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> port "
					<< name_
					<< " is open");

				//
				//	set options
				//
				set_all_options();

				//
				//	update status.word
				//
				base_.mux_.post(receiver_status_, 1u, cyng::tuple_factory(true));

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
					<< "> cannot open port "
					<< name_
					<< ": "
					<< ec.message());
			}
		}

		//
		//	setup monitor
		//
		base_.suspend(monitor_);

		return cyng::continuation::TASK_CONTINUE;
	}

	void lmn_port::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	void lmn_port::do_read()
	{
		port_.async_read_some(boost::asio::buffer(buffer_), [this](boost::system::error_code ec, std::size_t bytes_transferred) {

			if (!ec) {

				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< name_
					<< " received "
					<< bytes_transferred
					<< " bytes");

//#ifdef SMF_IO_DEBUG
				cyng::io::hex_dump hd;
				std::stringstream ss;
				if (bytes_transferred > 128) {
					hd(ss, buffer_.cbegin(), buffer_.cbegin() + 128);
				}
				else {
					hd(ss, buffer_.cbegin(), buffer_.cbegin() + bytes_transferred);
				}
				CYNG_LOG_TRACE(logger_, "\n" << ss.str());

//#endif
				base_.mux_.post(receiver_data_
					, 0u
					, cyng::tuple_factory(cyng::buffer_t(buffer_.cbegin(), buffer_.cbegin() + bytes_transferred), msg_counter_++));

				//
				//	continue reading
				//
				do_read();
			}
			else {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> connection closed "
					<< ec.message());

				//
				//	update status.word
				//
				base_.mux_.post(receiver_status_, 1u, cyng::tuple_factory(false));

			}
		});
	}

	void lmn_port::set_all_options()
	{
		try {
			port_.set_option(parity_);	// default none
			port_.set_option(databits_);
			port_.set_option(stopbits_);	// default one
			port_.set_option(baud_rate_);
			port_.set_option(flow_control_);
		}
		catch (std::exception const& ex) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set options failes "
				<< ex.what());
		}
	}

	cyng::continuation lmn_port::process(cyng::buffer_t data)
	{
		boost::system::error_code ec;
		boost::asio::write(port_, boost::asio::buffer(data, data.size()), ec);

		if (ec) {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> write to port "
				<< name_ 
				<< " failed: "
				<< ec.message());
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}
}

