/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_ipt.h>
#include <smf/ipt/scramble_key_format.h>
#include <smf/obis/conv.h>
#include <smf/obis/defs.h>
#include <smf/obis/tree.hpp>
#include <smf/sml/msg.h>
#include <smf/sml/value.hpp>

#include <cyng/obj/numeric_cast.hpp>
#include <cyng/parse/hex.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {

    cfg_ipt::cfg_ipt(cfg &c)
        : cfg_(c) {}

    namespace {
        std::string enabled_path() { return cyng::to_path(cfg::sep, cfg_ipt::root, "enabled"); }

        std::string host_path(std::uint8_t idx) {
            return cyng::to_path(
                cfg::sep,
                OBIS_ROOT_IPT_PARAM,
                cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                cyng::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx));
        }
        std::string target_port_path(std::uint8_t idx) {
            return cyng::to_path(
                cfg::sep,
                OBIS_ROOT_IPT_PARAM,
                cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                cyng::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx));
        }
        std::string source_port_path(std::uint8_t idx) {
            return cyng::to_path(
                cfg::sep,
                OBIS_ROOT_IPT_PARAM,
                cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                cyng::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx));
        }
        std::string account_path(std::uint8_t idx) {
            return cyng::to_path(
                cfg::sep,
                OBIS_ROOT_IPT_PARAM,
                cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx));
        }

        std::string pwd_path(std::uint8_t idx) {
            return cyng::to_path(
                cfg::sep,
                OBIS_ROOT_IPT_PARAM,
                cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx));
        }
        std::string sk_path(std::uint8_t idx) {
            return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), "sk");
        }
        std::string scrambled_path(std::uint8_t idx) {
            return cyng::to_path(
                cfg::sep,
                OBIS_ROOT_IPT_PARAM,
                cyng::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
                cyng::make_obis(0x81, 0x49, 0x63, 0x3C, 0x03, idx));
        }

        std::string reconnect_count_path() { return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, OBIS_TCP_CONNECT_RETRIES); }

        std::string reconnect_timeout_path() { return cyng::to_path(cfg::sep, OBIS_ROOT_IPT_PARAM, OBIS_TCP_WAIT_TO_RECONNECT); }

        std::string local_ep_path() { return cyng::to_path(cfg::sep, cfg_ipt::root, "local.ep"); }
        std::string remote_ep_path() { return cyng::to_path(cfg::sep, cfg_ipt::root, "remote.ep"); }

    } // namespace

    bool cfg_ipt::is_enabled() const { return cfg_.get_value(enabled_path(), false); }

    std::string cfg_ipt::get_host(std::uint8_t idx) const { return cfg_.get_value(host_path(idx), "segw.ch"); }
    std::string cfg_ipt::get_service(std::uint8_t idx) const { return cfg_.get_value(target_port_path(idx), "26862"); }
    std::uint16_t cfg_ipt::get_service_as_port(std::uint8_t idx) const {
        return cyng::string_to_numeric<std::uint32_t>(cfg_.get_obj(target_port_path(idx)), 26862u);
    }
    std::uint16_t cfg_ipt::get_source_port(std::uint8_t idx) const { return 0u; }

    std::string cfg_ipt::get_account(std::uint8_t idx) const { return cfg_.get_value(account_path(idx), "no-account"); }
    std::string cfg_ipt::get_pwd(std::uint8_t idx) const { return cfg_.get_value(pwd_path(idx), "no-password"); }
    ipt::scramble_key cfg_ipt::get_sk(std::uint8_t idx) const {
        return ipt::to_sk(cfg_.get_value(sk_path(idx), "0102030405060708090001020304050607080900010203040506070809000001"));
    }
    bool cfg_ipt::is_scrambled(std::uint8_t idx) const { return cfg_.get_value(scrambled_path(idx), true); }

    ipt::server cfg_ipt::get_server(std::uint8_t idx) const {

        return ipt::server(
            get_host(idx),
            get_service(idx),
            get_account(idx),
            get_pwd(idx),
            get_sk(idx),
            is_scrambled(idx),
            12); //	monitor
    }

    ipt::toggle::server_vec_t cfg_ipt::get_toggle() const {
        return {get_server(1), get_server(2)}; //	two configurations available
    }

    std::size_t cfg_ipt::get_reconnect_count() const {
        return cfg_.get_value(reconnect_count_path(), static_cast<std::size_t>(12));
    }

    std::chrono::seconds cfg_ipt::get_reconnect_timeout() const {
        return cfg_.get_value(reconnect_timeout_path(), std::chrono::seconds(10));
    }

    cyng::tuple_t cfg_ipt::get_params_as_child_list() const {

        //
        //  The response is hard coded since
        //  cfg_.loop(cyng::to_string(OBIS_ROOT_IPT_PARAM),...
        //  obis::to_obis_path(vec)
        //  didn't produce the expected results.
        //
        return sml::make_child_list_tree(
            OBIS_ROOT_IPT_PARAM,
            cyng::make_tuple(
                //  redundance 1
                sml::make_child_list_tree(
                    cyng::make_obis(OBIS_ROOT_IPT_PARAM, 1u),
                    cyng::make_tuple( // tuple start
                        sml::make_param_tree(cyng::make_obis(OBIS_TARGET_IP_ADDRESS, 1u), sml::make_attribute(get_host(1))),
                        // sml::make_param_tree(cyng::make_obis(OBIS_TARGET_IP_ADDRESS, 1u), sml::make_attribute(0x5201a8c0)),
                        sml::make_param_tree(cyng::make_obis(OBIS_TARGET_PORT, 1u), sml::make_attribute(get_service_as_port(1))),
                        sml::make_param_tree(cyng::make_obis(OBIS_SOURCE_PORT, 1u), sml::make_attribute(get_source_port(1))),
                        sml::make_param_tree(cyng::make_obis(OBIS_IPT_ACCOUNT, 1u), sml::make_attribute(get_account(1))),
                        sml::make_param_tree(cyng::make_obis(OBIS_IPT_PASSWORD, 1u), sml::make_attribute(get_pwd(1)))) // tuple end
                    ),

                //  redundance 2
                sml::make_child_list_tree(
                    cyng::make_obis(OBIS_ROOT_IPT_PARAM, 2u),
                    cyng::make_tuple( // tuple start
                        sml::make_param_tree(cyng::make_obis(OBIS_TARGET_IP_ADDRESS, 2u), sml::make_attribute(get_host(2))),
                        sml::make_param_tree(cyng::make_obis(OBIS_TARGET_PORT, 2u), sml::make_attribute(get_service_as_port(2))),
                        sml::make_param_tree(cyng::make_obis(OBIS_SOURCE_PORT, 2u), sml::make_attribute(get_source_port(2))),
                        sml::make_param_tree(cyng::make_obis(OBIS_IPT_ACCOUNT, 2u), sml::make_attribute(get_account(2))),
                        sml::make_param_tree(cyng::make_obis(OBIS_IPT_PASSWORD, 2u), sml::make_attribute(get_pwd(2)))) //  tuple end
                    ),
                sml::make_param_tree(OBIS_TCP_WAIT_TO_RECONNECT, sml::make_attribute(get_reconnect_timeout().count())),
                sml::make_param_tree(OBIS_TCP_CONNECT_RETRIES, sml::make_attribute(get_reconnect_count())),
                sml::make_param_tree(OBIS_HAS_SSL_CONFIG, sml::make_attribute(false)),
                sml::make_empty_tree(OBIS_SSL_CERTIFICATES))); //  no certificates
    }

    bool cfg_ipt::set_proc_parameter(cyng::obis_path_t path, cyng::object obj) {
        BOOST_ASSERT(!path.empty());
        BOOST_ASSERT(path.front() == OBIS_ROOT_IPT_PARAM);
        if (!path.empty() && path.front() == OBIS_ROOT_IPT_PARAM) {
            auto const key = obis::to_string(path, false, cfg::sep);
            return cfg_.set_value(key, obj);
        }
        return false;
    }

    bool cfg_ipt::set_local_enpdoint(boost::asio::ip::tcp::endpoint ep) const { return cfg_.set_value(local_ep_path(), ep); }
    bool cfg_ipt::set_remote_enpdoint(boost::asio::ip::tcp::endpoint ep) const { return cfg_.set_value(remote_ep_path(), ep); }

    boost::asio::ip::tcp::endpoint cfg_ipt::get_local_ep() const {
        return cfg_.get_value(local_ep_path(), boost::asio::ip::tcp::endpoint());
    }
    boost::asio::ip::tcp::endpoint cfg_ipt::get_remote_ep() const {
        return cfg_.get_value(remote_ep_path(), boost::asio::ip::tcp::endpoint());
    }

    const std::string cfg_ipt::root = cyng::to_string(OBIS_ROOT_IPT_PARAM);

} // namespace smf
