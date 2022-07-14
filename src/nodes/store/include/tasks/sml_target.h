/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_TASK_SML_TARGET_H
#define SMF_STORE_TASK_SML_TARGET_H

#include <smf/ipt/bus.h>
#include <smf/sml/reader.h>
#include <smf/sml/unpack.h>

#include <cyng/log/logger.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    class sml_target {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::string)>,
            std::function<void(std::uint32_t, std::uint32_t, cyng::buffer_t, std::string)>,
            std::function<void(std::string)>,
            std::function<void(cyng::eod)>>;

      public:
        sml_target(cyng::channel_weak, cyng::controller &ctl, cyng::logger logger, ipt::bus &);
        ~sml_target() = default;

        void stop(cyng::eod);

      private:
        void register_target(std::string);
        void receive(std::uint32_t, std::uint32_t, cyng::buffer_t, std::string);
        void add_writer(std::string);

        void open_response(std::string const &trx, cyng::tuple_t const &msg);
        void close_response(std::string const &trx, cyng::tuple_t const &msg);
        void get_profile_list_response(std::string const &trx, std::uint8_t group_no, cyng::tuple_t const &msg);
        void get_proc_parameter_response(std::string const &trx, std::uint8_t group_no, cyng::tuple_t const &msg);
        void get_list_response(std::string const &trx, std::uint8_t group_no, cyng::tuple_t const &msg);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        ipt::bus &bus_;
        /**
         * vector of SML writers
         */
        std::set<std::string> writers_;
        sml::unpack parser_;
    };

    //      std::vector<std::string> to_str_vector(cyng::obis_path_t const &path, bool translate);
    cyng::param_map_t convert_to_param_map(sml::sml_list_t const &);

} // namespace smf

#endif
