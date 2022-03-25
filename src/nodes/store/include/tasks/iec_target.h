/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_IEC_TARGET_H
#define SMF_STORE_TASK_IEC_TARGET_H

#include <smf/iec/parser.h>
#include <smf/ipt/bus.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class consumer {
      public:
        consumer(cyng::controller &, cyng::logger logger, std::set<std::string> const &writers);
        consumer(consumer const &) = delete;

      public:
        cyng::controller &ctl_;
        cyng::logger logger_;
        std::set<std::string> const writers_;
        iec::parser parser_;
        std::map<cyng::obis, std::pair<std::string, std::string>> data_;
        std::string id_; //!< meter id (8 characters)
    };

    class iec_target {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::string)>,
            std::function<void(std::uint32_t, std::uint32_t, cyng::buffer_t data, std::string)>,
            std::function<void(std::string)>,
            std::function<void(cyng::eod)>>;

        using channel_list = std::map<std::uint64_t, consumer>;

      public:
        iec_target(cyng::channel_weak, cyng::controller &ctl, cyng::logger logger, ipt::bus &);
        iec_target(iec_target const &) = delete;
        ~iec_target() = default;

        void stop(cyng::eod);

      private:
        void register_target(std::string);
        void receive(std::uint32_t, std::uint32_t, cyng::buffer_t data, std::string);
        void add_writer(std::string);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        ipt::bus &bus_;
        std::set<std::string> writers_;
        channel_list channel_list_;
    };

} // namespace smf

#endif
