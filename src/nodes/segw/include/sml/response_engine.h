/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_RESPONSE_ENGINE_H
#define SMF_SEGW_RESPONSE_ENGINE_H

#include <cfg.h>
#include <smf/sml/msg.h>

#include <cyng/log/logger.h>

namespace smf {

    namespace sml {
        class response_generator;
    }

    /**
     * response to IP-T/SML messages
     */
    class response_engine {
      public:
        response_engine(cfg &config, cyng::logger, sml::response_generator &);

        void generate_get_proc_parameter_response(
            sml::messages_t &,
            std::string trx,
            cyng::buffer_t,
            std::string,
            std::string,
            cyng::obis_path_t,
            cyng::object);

        void generate_set_proc_parameter_response(
            sml::messages_t &,
            std::string trx,
            cyng::buffer_t,
            std::string,
            std::string,
            cyng::obis_path_t);

      private:
        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_status_word(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_device_ident(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_device_time(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

      private:
        cfg &cfg_;
        cyng::logger logger_;
        sml::response_generator &res_gen_;
    };

} // namespace smf

#endif
