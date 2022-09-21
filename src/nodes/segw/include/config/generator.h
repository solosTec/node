/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_GENERATOR_H
#define SMF_SEGW_CONFIG_GENERATOR_H

#include <cfg.h>

namespace smf {

    cyng::vector_t create_default_config(
        std::chrono::system_clock::time_point now,
        std::filesystem::path tmp,
        std::filesystem::path cwd,
        std::string hostname,
        cyng::mac48 srv_mac,
        std::string srv_id,
        boost::uuids::uuid tag);
    cyng::tuple_t create_wireless_spec(std::string const &hostname);
    cyng::tuple_t create_rs485_spec(std::string const &hostname);
    cyng::param_t create_wireless_broker(std::string const &hostname);
    cyng::param_t create_wireless_block_list();
    cyng::param_t create_rs485_broker(std::string const &hostname);
    cyng::param_t create_rs485_listener();
    cyng::param_t create_rs485_block_list();
    cyng::param_t create_gpio_spec();
    cyng::param_t create_hardware_spec(std::string const &);
    cyng::param_t create_nms_server_spec(std::filesystem::path const &);
    cyng::param_t create_sml_server_spec();
    cyng::param_t create_db_spec(std::filesystem::path const &);
    cyng::param_t create_ipt_spec(std::string const &hostname);
    cyng::param_t create_ipt_config(std::string const &hostname);
    cyng::param_t create_ipt_params();
    cyng::param_t create_wireless_virtual_meter_spec();
    cyng::param_t create_wired_virtual_meter_spec();
    cyng::param_t create_lmn_spec(std::string const &hostname);
    cyng::param_t create_net(cyng::mac48 const &, std::string const &);

    /**
     * Examine hardware to generate the model name.
     * Since the model name contains the server id, it's possible to
     * generate a proper configuration automatically.
     */
    std::string detect_model(std::string const &srv_id);

} // namespace smf

#endif
