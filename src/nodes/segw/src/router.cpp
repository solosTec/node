/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <router.h>
#include <config/cfg_ipt.h>
#include <config/cfg_hardware.h>

#include <cyng/task/controller.h>
#include <cyng/log/record.h>

namespace smf {

	router::router(cyng::controller& ctl, cfg& config, cyng::logger logger)
		: ctl_(ctl)
		, cfg_(config)
		, logger_(logger)
		, bus_()
	{	}

	void router::start() {

		cfg_ipt ipt_cfg(cfg_);
		if (ipt_cfg.is_enabled()) {
			//
			//	start IP-T bus
			//
			cfg_hardware hw_cfg(cfg_);
			CYNG_LOG_INFO(logger_, "initialize: IP-T client as " << hw_cfg.get_model());
			try {
				bus_ = std::make_unique<ipt::bus>(ctl_.get_ctx()
					, logger_
					, ipt_cfg.get_toggle()
					, hw_cfg.get_model()
					, std::bind(&router::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
					, std::bind(&router::ipt_stream, this, std::placeholders::_1));
				bus_->start();
			}
			catch (std::exception const& ex) {
				CYNG_LOG_ERROR(logger_, "start IP-T client failed " << ex.what());
				bus_.reset();
			}

			//
			//	start OBIS log
			//
		}
		else {
			CYNG_LOG_WARNING(logger_, "IP-T client not enabled");
		}

	}

	void router::ipt_cmd(ipt::header const& h, cyng::buffer_t&& body) {

		CYNG_LOG_TRACE(logger_, "router ipt cmd " << ipt::command_name(h.command_));

	}
	void router::ipt_stream(cyng::buffer_t&& data) {
		CYNG_LOG_TRACE(logger_, "router ipt stream " << data.size() << " byte");

	}


}