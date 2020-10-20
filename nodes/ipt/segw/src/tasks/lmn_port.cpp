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
#include <smf/sml/obis_db.h>

#include <cyng/io/hex_dump.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/io_bytes.hpp>

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
		, std::size_t receiver_status
		, cyng::buffer_t&& init)
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
		, receiver_data_()
		, receiver_status_(receiver_status)
		, init_(std::move(init))
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
				//	send initialization string init_
				//	only once
				//
				if (!init_.empty()) {

					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> send "
						<< cyng::bytes_to_str(init_.size())
						<< " initialization data");

					process(init_);
					init_.clear();
				}
					
				//
				//	update status.word
				//
				if (receiver_status_ != cyng::async::NO_TASK)	base_.mux_.post(receiver_status_, 1u, cyng::tuple_factory(true));

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
					<< cyng::bytes_to_str(bytes_transferred)
					<< " => "
					<< receiver_data_.size()
					<< " receiver");

#ifdef SMF_IO_DEBUG
				cyng::io::hex_dump hd;
				std::stringstream ss;
				if (bytes_transferred > 128) {
					hd(ss, buffer_.cbegin(), buffer_.cbegin() + 128);
				}
				else {
					hd(ss, buffer_.cbegin(), buffer_.cbegin() + bytes_transferred);
				}
				CYNG_LOG_TRACE(logger_, "\n" << ss.str());

#endif
				//
				//	post data to receiver 
				//
				for (auto tsk : receiver_data_) {
					base_.mux_.post(tsk, 0u, cyng::tuple_factory(cyng::buffer_t(buffer_.cbegin(), buffer_.cbegin() + bytes_transferred), msg_counter_));
				}
				if (receiver_status_ != cyng::async::NO_TASK)	base_.mux_.post(receiver_status_, 0u, cyng::tuple_factory(buffer_.size(), msg_counter_));

				//
				//	update message counter
				//
				++msg_counter_;

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
				if (receiver_status_ != cyng::async::NO_TASK)	base_.mux_.post(receiver_status_, 1u, cyng::tuple_factory(false));

			}
		});
	}

	void lmn_port::set_all_options()
	{
		try {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set parity "
				<< serial::to_str(parity_));

			port_.set_option(parity_);	// default none

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set data bits "
				<< +databits_.value());

			port_.set_option(databits_);

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set stopbits "
				<< serial::to_str(stopbits_));

			port_.set_option(stopbits_);	// default one

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set baud rate "
				<< baud_rate_.value());

			port_.set_option(baud_rate_);

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set flow control "
				<< serial::to_str(flow_control_));

			port_.set_option(flow_control_);
		}
		catch (std::exception const& ex) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set options failed "
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

	cyng::continuation lmn_port::process(std::size_t tsk, bool add)
	{
		auto pos = std::find(std::begin(receiver_data_), std::end(receiver_data_), tsk);
		if (add) {
			if (pos != std::end(receiver_data_)) {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> receiver task #"
					<< tsk
					<< " already inserted");
			}
			else {
				receiver_data_.push_back(tsk);

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> receiver task #"
					<< tsk
					<< " added - "
					<< receiver_data_.size()
					<< " in total");
			}
		}
		else {
			if (pos != std::end(receiver_data_)) {

				receiver_data_.erase(pos);

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> receiver task #"
					<< tsk
					<< " removed - "
					<< receiver_data_.size()
					<< " in total");
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> cannot remove receiver task #"
					<< tsk
					<< " - not found");
			}
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	[2] modify options int
	//	HARDWARE_PORT_DATABITS
	//	HARDWARE_PORT_SPEED
	cyng::continuation lmn_port::process(cyng::buffer_t buf, std::uint32_t val)
	{
		auto const code = sml::obis(buf);
		boost::system::error_code ec;

		switch (code.to_uint64()) {
		case sml::CODE_HARDWARE_PORT_DATABITS:
			port_.set_option(boost::asio::serial_port_base::character_size(val), ec);
			if (!ec) {
				port_.get_option(databits_, ec);
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> set stopbits "
					<< +databits_.value());
			}
			else {
				CYNG_LOG_ERROR(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> set databits to "
					<< val
					<< " failed: "
					<< ec.message());
			}
			break;
		case sml::CODE_HARDWARE_PORT_SPEED:
			port_.set_option(boost::asio::serial_port_base::baud_rate(val), ec);
			if (!ec) {
				port_.get_option(baud_rate_, ec);
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> set baud rate "
					<< baud_rate_.value());
			}
			else {
				CYNG_LOG_ERROR(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> set baud rate to "
					<< val
					<< " failed: "
					<< ec.message());

			}
			break;
		default:
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< name_
				<< " - unknown option "
				<< val);
			break;
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	[3] modify options string
	//	HARDWARE_PORT_FLOW_CONTROL
	//	HARDWARE_PORT_STOPBITS
	//	HARDWARE_PORT_PARITY
	cyng::continuation lmn_port::process(cyng::buffer_t buf, std::string val)
	{
		auto const code = sml::obis(buf);
		switch (code.to_uint64()) {
		case sml::CODE_HARDWARE_PORT_FLOW_CONTROL:
			port_.set_option(serial::to_flow_control(val));
			port_.get_option(flow_control_);
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set flow control "
				<< serial::to_str(flow_control_));
			break;
		case sml::CODE_HARDWARE_PORT_STOPBITS:
			port_.set_option(serial::to_stopbits(val));
			port_.get_option(stopbits_);
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set stopbits "
				<< serial::to_str(stopbits_));
			break;
		case sml::CODE_HARDWARE_PORT_PARITY:
			port_.set_option(serial::to_parity(val));
			port_.get_option(parity_);
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set parity "
				<< serial::to_str(parity_));
			break;
		default:
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< name_
				<< " - unknown option "
				<< val);
			break;
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

