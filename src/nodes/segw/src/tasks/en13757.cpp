#include <tasks/en13757.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/controller.h>

//#include <fstream>
//#include <iostream>
//
//#include <cyng/io/hex_dump.hpp>
//#include <iomanip>
//#include <iostream>
//#include <sstream>

namespace smf {
    en13757::en13757(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, cfg& config)
	: sigs_{
            std::bind(&en13757::receive, this, std::placeholders::_1),	//	0
            std::bind(&en13757::reset_target_channels, this),	//	1 - "reset-data-sinks"
            std::bind(&en13757::add_target_channel, this, std::placeholders::_1),	//	2 - "add-data-sink"
            std::bind(&en13757::update_statistics, this),	//	3
            std::bind(&en13757::stop, this, std::placeholders::_1)		//	4
    }
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
	{

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"receive", "reset-data-sinks", "add-data-sink", "update-statistics"});

            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void en13757::stop(cyng::eod) {
        CYNG_LOG_INFO(logger_, "[en13757] stopped");
        boost::system::error_code ec;
    }

    void en13757::receive(cyng::buffer_t data) { CYNG_LOG_TRACE(logger_, "[en13757] received " << data.size() << " bytes"); }

    void en13757::reset_target_channels() {
        // targets_.clear();
        CYNG_LOG_TRACE(logger_, "[en13757] clear target channels");
    }

    void en13757::add_target_channel(std::string name) {
        // targets_.insert(name);
        CYNG_LOG_TRACE(logger_, "[en13757] add target channel: " << name);
    }

    void en13757::update_statistics() { CYNG_LOG_TRACE(logger_, "[en13757] update statistics"); }

} // namespace smf
