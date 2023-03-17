#include <tasks/counter.h>

#include <cfg.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>

#include <fstream>
#include <iostream>

namespace smf {
    counter::counter(cyng::channel_weak wp
		, cyng::logger logger, cfg& config)
		: sigs_{
			std::bind(&counter::inc, this),	//	0
            std::bind(&counter::save, this),	//	1
            std::bind(&counter::stop, this, std::placeholders::_1)	//	2
		}	
		, channel_(wp)
		, logger_(logger)
        , cfg_(config)
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"inc", "save"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created: " << cfg_.op_time_ << " seconds");
        }
    }

    void counter::stop(cyng::eod) {
        //
        //  save current value
        //
        // cfg_.set_value("opcounter", cfg_.op_time_); // -- persistence layer could be already offline
        CYNG_LOG_INFO(logger_, "op counter stopped");
    }

    void counter::inc() {

        //
        // First restart the timer to reduce time drift
        // (minimal time drift is acceptable since this is pure relative way
        // to measure time)
        //
        if (auto sp = channel_.lock(); sp) {
            //  repeat
            //  handle dispatch errors
            sp->suspend(
                std::chrono::seconds(1),
                "inc",
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));

            //
            //	increment
            //
            std::uint32_t const optime = ++cfg_.op_time_;

            //
            // update only once per minute to reduce database load
            //
            if ((optime % 60) == 0) {
                cfg_.set_value("opcounter", optime);
            }
        }
    }

    void counter::save() { cfg_.set_value("opcounter", cfg_.op_time_); }

} // namespace smf
