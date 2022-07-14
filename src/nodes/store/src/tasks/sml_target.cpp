/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <smf/mbus/server_id.h>
#include <smf/obis/db.h>

#include <tasks/sml_target.h>

#include <cyng/io/hex_dump.hpp>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_target::sml_target(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, ipt::bus &bus)
        : sigs_{std::bind(&sml_target::register_target, this, std::placeholders::_1), std::bind(&sml_target::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), std::bind(&sml_target::add_writer, this, std::placeholders::_1), std::bind(&sml_target::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , bus_(bus)
        , writers_()
        , parser_([this](
                      std::string trx,
                      std::uint8_t group_no,
                      std::uint8_t,
                      smf::sml::msg_type type,
                      cyng::tuple_t msg,
                      std::uint16_t crc) {
            switch (type) {
            case sml::msg_type::OPEN_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                open_response(trx, msg);
                break;
            case sml::msg_type::GET_PROFILE_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                get_profile_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::GET_PROC_PARAMETER_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                get_proc_parameter_response(trx, group_no, msg);
                break;
            case sml::msg_type::GET_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                get_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::CLOSE_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                close_response(trx, msg);
                break;
            default: CYNG_LOG_WARNING(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg); break;
            }
        }) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"register", "pushdata.transfer", "add"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void sml_target::stop(cyng::eod) {}

    void sml_target::register_target(std::string name) { bus_.register_target(name, channel_); }

    void sml_target::add_writer(std::string name) { writers_.insert(name); }

    void sml_target::receive(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data, std::string target) {

        if (boost::algorithm::equals(channel_.lock()->get_name(), target)) {
            CYNG_LOG_TRACE(logger_, "[sml] " << target << " receive " << data.size() << " bytes");
            //{
            //    std::stringstream ss;
            //    cyng::io::hex_dump<8> hd;
            //    hd(ss, data.begin(), data.end());
            //    auto const dmp = ss.str();
            //    CYNG_LOG_TRACE(logger_, "[sml] " << target << " recived " << data.size() << " bytes:\n" << dmp);
            //}
            parser_.read(std::begin(data), std::end(data));

        } else {
            CYNG_LOG_WARNING(
                logger_,
                "[sml] wrong target name " << channel_.lock()->get_name() << "/" << target << " - " << data.size()
                                           << " bytes lost");
        }
    }

    void sml_target::open_response(std::string const &trx, cyng::tuple_t const &msg) {
        auto const tpl = smf::sml::read_public_open_response(msg);

        CYNG_LOG_TRACE(logger_, "[sml] open response " << srv_id_to_str(std::get<1>(tpl)) << "*" << std::get<3>(tpl));
        for (auto writer : writers_) {
            //  send client and server ID
            ctl_.get_registry().dispatch(writer, "open.response", std::get<1>(tpl), std::get<3>(tpl));
        }
    }
    void sml_target::close_response(std::string const &trx, cyng::tuple_t const &msg) {
        auto const r = smf::sml::read_public_close_response(msg);
    }
    void sml_target::get_profile_list_response(std::string const &trx, std::uint8_t group_no, cyng::tuple_t const &msg) {
        // CYNG_LOG_DEBUG(logger_, "get_profile_list_response: " << msg);
        auto const [srv, path, act, reg, val, stat, list] = sml::read_get_profile_list_response(msg);
        for (auto const &ro : list) {
            //  there is a problem with OBIS code "01 00 00 09 0b 00" which contains an invalid data type of
            //  std::chrono::duration< ... ratio<1,10000000> >
            //  CURRENT_UTC
            // if (ro.first == cyng::make_obis({0x01, 0x00, 0x00, 0x09, 0x0b, 0x00})) {
            //    CYNG_LOG_DEBUG(logger_, "get_profile_list_response: " << msg);
            //}
            CYNG_LOG_DEBUG(logger_, ro.first << ": " << ro.second);
        }

        //
        // convert value map from
        // std::map<cyng::obis, cyng::param_map_t>
        // to
        // std::map<std::string, cyng::object> == cyng::param_map_t
        //
        //  pmap contains an "key" entry that contains the original
        //  obis code.
        auto const pmap = convert_to_param_map(list);
        BOOST_ASSERT(!pmap.empty());

        //
        //  send to writers
        //
        for (auto writer : writers_) {
            ctl_.get_registry().dispatch(
                writer,
                "get.profile.list.response",
                trx,
                srv,  //  [buffer_t] server id
                act,  //  [cyng::object] actTime
                stat, //  [u32] status
                path, //  [obis_path_t] path
                pmap  //  [std::map<cyng::obis, cyng::param_map_t>] values
            );
        }
    }

    void sml_target::get_proc_parameter_response(std::string const &trx, std::uint8_t group_no, cyng::tuple_t const &msg) {
        auto const r = smf::sml::read_get_proc_parameter_response(msg);
        //
        //  ToDo: send to writers
        //
        for (auto writer : writers_) {
        }
    }

    void sml_target::get_list_response(std::string const &trx, std::uint8_t group_no, cyng::tuple_t const &msg) {
        // std::tuple<cyng::buffer_t, cyng::buffer_t, cyng::obis, cyng::object, cyng::object, sml_list_t>
        auto const r = smf::sml::read_get_list_response(msg);

        CYNG_LOG_INFO(logger_, "list response from " << srv_id_to_str(std::get<1>(r)));
        for (auto const &ro : std::get<5>(r)) {
            CYNG_LOG_TRACE(logger_, ro.first << ": " << ro.second);
        }

        // 0100010800ff:
        // %(("raw":14521),("scaler":-1),("status":00020240),("unit":1e),("unit-name":Wh),("valTime":null),("value":1452.1))
        // 0100010801ff: %(("raw":0),("scaler":-1),("status":null),("unit":1e),("unit-name":Wh),("valTime":null),("value":0))
        // 0100010802ff:
        // %(("raw":14521),("scaler":-1),("status":null),("unit":1e),("unit-name":Wh),("valTime":null),("value":1452.1))
        // 0100020800ff:
        // %(("raw":561139),("scaler":-1),("status":00020240),("unit":1e),("unit-name":Wh),("valTime":null),("value":56113.9))
        // 0100020801ff: %(("raw":0),("scaler":-1),("status":null),("unit":1e),("unit-name":Wh),("valTime":null),("value":0))
        // 0100020802ff:
        // %(("raw":561139),("scaler":-1),("status":null),("unit":1e),("unit-name":Wh),("valTime":null),("value":56113.9))
        // 0100100700ff: %(("raw":0),("scaler":-1),("status":null),("unit":1b),("unit-name":W),("valTime":null),("value":0))

        auto const pmap = convert_to_param_map(std::get<5>(r));
        //
        //  send to writers
        //
        for (auto writer : writers_) {
            ctl_.get_registry().dispatch(
                writer,
                "get.profile.list.response",
                trx,
                std::get<1>(r),                      //  [buffer_t] server id
                std::chrono::system_clock::now(),    //  [cyng::object] actTime
                std::uint32_t(0),                    //  [u32] status
                cyng::obis_path_t({std::get<2>(r)}), //  [obis_path_t] path
                pmap);
        }
    }

    cyng::param_map_t convert_to_param_map(sml::sml_list_t const &inp) {
        cyng::param_map_t r;
        std::transform(
            std::begin(inp),
            std::end(inp),
            std::inserter(r, r.end()),
            [](sml::sml_list_t::value_type value) -> cyng::param_map_t::value_type {
                //  Add the original key as value. This saves a reconversion from string type
                value.second.emplace("key", cyng::make_object(value.first));
                value.second.emplace("code", cyng::make_object(obis::get_name(value.first)));
                value.second.emplace("descr", cyng::make_object(obis::get_description(value.first)));

                return {cyng::to_string(value.first), cyng::make_object(value.second)};
            });
        return r;
    }

} // namespace smf
