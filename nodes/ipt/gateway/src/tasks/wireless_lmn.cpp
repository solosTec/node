/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "wireless_lmn.h"
#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>

#include <cyng/io/serializer.h>

//#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
//#endif

namespace node
{

	wireless_LMN::wireless_LMN(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& config_db
		, cyng::controller& vm
		, std::string port
		, std::uint8_t databits
		, std::string parity
		, std::string flow_control
		, std::string stopbits
		, std::uint32_t speed
		, std::size_t tid)
	: base_(*btp) 
		, logger_(logger)
		, config_db_(config_db)
		, vm_(vm)
		, port_(btp->mux_.get_io_service(), port)
		, buffer_()
		, task_gpio_(tid)
		, parser_([&](cyng::vector_t&& prg){

// 			CYNG_LOG_DEBUG(logger_, prg.size() << " m-bus instructions received");
// 			CYNG_LOG_TRACE(logger_, vm_.tag() << ": " << cyng::io::to_str(prg));
			vm_.async_run(std::move(prg));
	})
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< port);

		try {
			port_.set_option(serial::to_parity(parity));	// default none
			port_.set_option(boost::asio::serial_port_base::character_size(databits));
			port_.set_option(serial::to_stopbits(stopbits));	// default one
			port_.set_option(boost::asio::serial_port_base::baud_rate(speed));
			port_.set_option(serial::to_flow_control(flow_control));
		}
		catch (std::exception const& ex) {

			CYNG_LOG_FATAL(logger_, "port initialization task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< ex.what());
		}
	}

	cyng::continuation wireless_LMN::run()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> start reading");

		do_read();
		return cyng::continuation::TASK_CONTINUE;
	}

	void wireless_LMN::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	void wireless_LMN::do_read()
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

				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> input dump "
					<< bytes_transferred
					<< " bytes\n"
					<< ss.str());
//#endif

                //
                //  feed the parser
                //
                parser_.read(buffer_.cbegin(), buffer_.cbegin() + bytes_transferred);
                
				//
				//	signal LED
				//
				if (cyng::async::NO_TASK != task_gpio_) {
					base_.mux_.post(task_gpio_, 1, cyng::tuple_factory(std::uint32_t(150), std::size_t(bytes_transferred / 10)));
				}

				//
				//	continue reading
				//
				do_read();
			}
		});
    }
}

