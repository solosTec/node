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
            std::bind(&en13757::stop, this, std::placeholders::_1)		//	3
    }
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
	{

        if (auto sp = channel_.lock(); sp) {
            // sp->set_channel_names({"receive", "reset-data-sinks", "update-statistics"});

            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void en13757::stop(cyng::eod) {
        CYNG_LOG_INFO(logger_, "[en13757] stopped");
        boost::system::error_code ec;
    }

    void en13757::receive(cyng::buffer_t data) {}

} // namespace smf
