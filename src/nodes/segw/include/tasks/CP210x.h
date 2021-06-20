/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_CP210X_H
#define SMF_SEGW_TASK_CP210X_H

#include <smf/hci/parser.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    /**
     * receiver and parser for CP210x host control protocol (HCP)
     */
    class CP210x {
        template <typename T> friend class cyng::task;

        using signatures_t =
            std::tuple<std::function<void(cyng::eod)>, std::function<void(cyng::buffer_t)>, std::function<void(std::string)>>;

      public:
        CP210x(cyng::channel_weak, cyng::controller &ctl, cyng::logger);

      private:
        void stop(cyng::eod);

        /**
         * message complete
         */
        void put(hci::msg const &);

        /**
         * parse incoming raw data
         */
        void receive(cyng::buffer_t);

        /**
         * reset target channels
         */
        void reset_target_channels(std::string);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;

        /**
         * global logger
         */
        cyng::logger logger_;

        hci::parser parser_;

        std::vector<cyng::channel_ptr> targets_;
    };

} // namespace smf

#endif
