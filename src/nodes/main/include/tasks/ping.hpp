/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_TASK_PING_HPP
#define SMF_MAIN_TASK_PING_HPP

#include <cyng/log/logger.h>
#include <cyng/log/record.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <iostream>

#include <boost/uuid/uuid.hpp>

namespace smf {

    /**
     * send cyclic ping requests
     */
    template <typename S> class ping {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void()>,         //   update
            std::function<void(cyng::eod)> //  stop
            >;

        using session_t = S;

      public:
        ping(cyng::channel_weak wp, cyng::logger logger, std::shared_ptr<S> sp)
            : sigs_{
                  std::bind(&ping::update, this),                     // update
                  std::bind(&ping::stop, this, std::placeholders::_1) // stop
              } 
        , channel_(wp)
        , logger_(logger)
        , sp_(sp) {

            if (auto sp = channel_.lock(); sp) {
                sp->set_channel_names({"update"});
                CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
            }
        }

        ~ping() = default;

      private:
        void stop(cyng::eod) {
            //  decrease reference counter
            // sp_.reset();
        }
        void update() {
            if (auto sp = channel_.lock(); sp) {

                //
                // send ping
                //
                if (sp_) {
                    sp_->send_ping_request();
                }

                //  repeat
                sp->suspend(
                    std::chrono::minutes(1),
                    "update",
                    //  handle dispatch errors
                    std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));
            }
        }

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        std::shared_ptr<session_t> sp_;
    };

} // namespace smf

#endif
