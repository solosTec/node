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

    class storage;
    /**
     * response to IP-T/SML messages
     */
    class response_engine {
      public:
        response_engine(cyng::logger, cfg &config, storage &, sml::response_generator &);

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
            cyng::obis_path_t,
            cyng::obis,
            cyng::attr_t,
            cyng::tuple_t);

        void generate_get_profile_list_response(
            sml::messages_t &,
            std::string trx,
            cyng::buffer_t,
            std::string,
            std::string,
            bool,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            cyng::obis_path_t,
            cyng::object,
            cyng::object);

      private:
        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_status_word(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_device_ident(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_device_time(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_ntp(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_access_rights(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_custom_interface(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_custom_param(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_wan(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_gsm(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_gprs(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_ipt_state(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_ipt_param(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t set_proc_parameter_ipt_param(
            std::string const &trx,
            cyng::buffer_t const &server,
            cyng::obis_path_t const &path,
            cyng::attr_t const &attr);

        [[nodiscard]] cyng::tuple_t set_proc_parameter_data_collector(
            std::string const &trx,
            cyng::buffer_t const &server,
            cyng::obis_path_t const &path,
            cyng::tuple_t const &child_list);

        [[nodiscard]] cyng::tuple_t clear_proc_parameter_data_collector(
            std::string const &trx,
            cyng::buffer_t const &server,
            cyng::obis_path_t const &path,
            cyng::tuple_t const &child_list);

        [[nodiscard]] cyng::tuple_t set_proc_parameter_push_ops(
            std::string const &trx,
            cyng::buffer_t const &server,
            cyng::obis_path_t const &path,
            cyng::tuple_t const &child_list);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_wmbus_state(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_wmbus_param(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_lan_dsl_state(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_lan_dsl_param(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_memory(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_visisble_devices(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_active_devices(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_device_info(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_sensor_param(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_data_collector(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_1107_interface(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_storage_timeshift(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_push_ops(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_list_services(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_actuators(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_edl(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_mbus(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] cyng::tuple_t
        get_proc_parameter_security(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path);

        [[nodiscard]] void get_profile_list_response_oplog(
            sml::messages_t &msgs,
            std::string const &trx,
            cyng::buffer_t const &server,
            cyng::obis_path_t const &path,
            std::chrono::system_clock::time_point const &begin,
            std::chrono::system_clock::time_point const &end);

      private:
        cyng::logger logger_;
        cfg &cfg_;
        storage &storage_;
        sml::response_generator &res_gen_;
    };

    cyng::tuple_t generate_tree_ntp(std::vector<std::string> const &ntp_servers);

    cyng::tuple_t generate_active_device_entry(std::uint8_t, std::uint8_t, cyng::buffer_t, std::chrono::system_clock::time_point);
    cyng::tuple_t generate_visible_device_entry(std::uint8_t, std::uint8_t, cyng::buffer_t, std::chrono::system_clock::time_point);

    void insert_data_collector(cyng::table *, cyng::table *, cyng::key_t const &, cyng::prop_map_t const &, boost::uuids::uuid);
    void update_data_collector(cyng::table *, cyng::table *, cyng::key_t const &, cyng::prop_map_t const &, boost::uuids::uuid);

    /**
     * update register codes
     */
    void update_data_mirror(cyng::table *, cyng::key_t const &, cyng::obis, boost::uuids::uuid);

    cyng::tuple_t get_collector_registers(cyng::table const *, cyng::buffer_t const &, std::uint8_t);

    bool insert_push_op(
        cyng::table *,
        cyng::table *,
        cyng::table const *tbl_col,
        cyng::table const *tbl_mir,
        cyng::key_t const &,
        cyng::prop_map_t const &,
        cyng::buffer_t const &server,
        boost::uuids::uuid);
    void update_push_op(
        cyng::table *,
        cyng::table *,
        cyng::key_t const &,
        cyng::record const &,
        cyng::prop_map_t const &,
        cyng::buffer_t const &server,
        boost::uuids::uuid);

} // namespace smf

#endif
