#include <smf/ipt/tasks/watchdog.h>

#include <smf/ipt/bus.h>

namespace smf {
    namespace ipt {
        watchdog::watchdog(cyng::channel_weak wp, cyng::logger logger, bus& ipt_bus)
        : sigs_{ 
		    std::bind(&watchdog::timeout, this, std::placeholders::_1),
		    std::bind(&watchdog::stop, this, std::placeholders::_1),
        }
        , channel_(wp)
        , logger_(logger)
        , bus_(ipt_bus) {

            if (auto sp = channel_.lock(); sp) {
                sp->set_channel_names({"timeout"});
                CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
            }
        }

        void watchdog::timeout(std::chrono::minutes period) {
            if (auto sp = channel_.lock(); sp) {

                CYNG_LOG_TRACE(logger_, "[watchdog] timeout");

                //
                //  restart
                //
                sp->suspend(
                    period,
                    "timeout",
                    //  handle dispatch errors
                    std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                    period);
                //
                //  send watchdog response
                //
                bus_.send_watchdog();
            }
        }

    } // namespace ipt
} // namespace smf
