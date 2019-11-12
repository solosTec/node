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

namespace node
{
	lmn_port::lmn_port(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::chrono::seconds monitor
		, std::string port
		, std::uint8_t databits
		, std::string parity
		, std::string flow_control
		, std::string stopbits
		, std::uint32_t speed
		, std::size_t tid)
	: base_(*btp) 
		, logger_(logger)
		, port_(btp->mux_.get_io_service())
		, monitor_(monitor)
		, name_(port)
		, databits_(databits)
		, parity_(serial::to_parity(parity))
		, flow_control_(serial::to_flow_control(flow_control))
		, stopbits_(serial::to_stopbits(stopbits))
		, baud_rate_(speed)
		, tid_(tid)
		, buffer_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< port);

		//boost::asio::serial_port_base::baud_rate baud_rate(speed_);
		//port_.set_option(baud_rate);
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
					<< "> received "
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
			}
		});
	}

	void lmn_port::set_all_options()
	{
		try {
			port_.set_option(parity_);	// default none
			port_.set_option(databits_);
			port_.set_option(databits_);	// default one
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

	cyng::continuation lmn_port::process(bool on)
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}
}

