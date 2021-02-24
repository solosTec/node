/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/lmn.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	lmn::lmn(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, cfg& c
		, lmn_type type)
	: sigs_{
		std::bind(&lmn::stop, this, std::placeholders::_1),
		std::bind(&lmn::open, this),
	}	, channel_(wp)
		, logger_(logger)
		, cfg_(c, type)
		, port_(ctl.get_ctx())
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("open", 1);
		}

		CYNG_LOG_INFO(logger_, "LMN " << cfg_.get_port() << " ready");
	}

	void lmn::stop(cyng::eod) {
		CYNG_LOG_INFO(logger_, "LMN stop");
		boost::system::error_code ec;
		port_.close(ec);
	}

	void lmn::open() {
		auto const port = cfg_.get_port();
		CYNG_LOG_INFO(logger_, "open LMN " << port);

		boost::system::error_code ec;
		if (!port_.is_open()) {
			port_.open(port, ec);
			if (!ec) {

				//
				//	set options
				//
				set_options(port);

			}
			else {
				CYNG_LOG_ERROR(logger_, "cannot open port ["
					<< port
					<< "] - "
					<< ec.message());

			}
		}
	}

	void lmn::set_options(std::string const& port) {

		try {
			auto const parity = cfg_.get_parity();
			port_.set_option(parity);	// default none
			CYNG_LOG_DEBUG(logger_, "[" << port << "] parity: " << serial::to_str(parity));

			auto const databits = cfg_.get_databits();
			port_.set_option(databits);
			CYNG_LOG_DEBUG(logger_, "[" << port << "] databits: " << +databits.value());

			auto const stopbits = cfg_.get_stopbits();
			port_.set_option(stopbits);
			CYNG_LOG_DEBUG(logger_, "[" << port << "] stopbits: " << serial::to_str(stopbits));

			auto const baud_rate = cfg_.get_baud_rate();
			port_.set_option(baud_rate);
			CYNG_LOG_DEBUG(logger_, "[" << port << "] baud-rate: " << baud_rate.value());

			auto const flow_control = cfg_.get_flow_control();
			port_.set_option(flow_control);
			CYNG_LOG_DEBUG(logger_, "[" << port << "] flow-control: " << serial::to_str(flow_control));
		}
		catch (std::exception const& ex) {

			CYNG_LOG_ERROR(logger_, "set options ["
				<< port
				<< "] failed - "
				<< ex.what());

		}
	}

}