/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_target.h>

#include <smf/mbus/server_id.h>
#include <smf/obis/db.h>
#include <smf/obis/profile.h>

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
            CYNG_LOG_TRACE(logger_, "[sml] " << smf::sml::get_name(type) << "#" << +group_no << ": " << trx << ", " << msg);
            switch (type) {
            case sml::msg_type::OPEN_RESPONSE: open_response(trx, msg); break;
            case sml::msg_type::GET_PROFILE_LIST_RESPONSE: get_profile_list_response(trx, group_no, msg); break;
            case sml::msg_type::GET_PROC_PARAMETER_RESPONSE: get_proc_parameter_response(trx, group_no, msg); break;
            case sml::msg_type::GET_LIST_RESPONSE: get_list_response(trx, group_no, msg); break;
            case sml::msg_type::CLOSE_RESPONSE: close_response(trx, msg); break;
            default: CYNG_LOG_WARNING(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg); break;
            }
        }) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"register", "pushdata.transfer", "add"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void sml_target::stop(cyng::eod) {}

    void sml_target::register_target(std::string name) {
        CYNG_LOG_TRACE(logger_, "[sml] register target [" << name << "]");
        bus_.register_target(name, channel_);
    }

    void sml_target::add_writer(std::string name) { writers_.insert(name); }

    void sml_target::receive(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data, std::string target) {

        if (boost::algorithm::equals(channel_.lock()->get_name(), target)) {
            CYNG_LOG_TRACE(logger_, "[sml] target [" << target << "] received " << data.size() << " bytes");
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
        // std::string, cyng::buffer_t, cyng::buffer_t, std::string, cyng::object, std::uint8_t
        // codepage, client ID (05+MAC), reqFileId, server ID, refTime, SML version
        auto const [codepage, client, file, server, reftime, version] = smf::sml::read_public_open_response(msg);

        CYNG_LOG_TRACE(logger_, "[sml] open response " << srv_id_to_str(client, true) << "*" << server);
        for (auto writer : writers_) {
            //  send client and server ID
            ctl_.get_registry().dispatch(
                writer,
                "open.response",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                client,
                server);
        }
    }
    void sml_target::close_response(std::string const &trx, cyng::tuple_t const &msg) {
        auto const r = smf::sml::read_public_close_response(msg);
    }
    void sml_target::get_profile_list_response(std::string const &trx, std::uint8_t group_no, cyng::tuple_t const &msg) {
        auto const [srv, path, act, reg, val, stat, list] = sml::read_get_profile_list_response(msg);

        BOOST_ASSERT(path.size() == 1);
        if (!path.empty()) {
            auto const &profile = path.front();
            if (!sml::is_profile(profile)) {
                CYNG_LOG_WARNING(
                    logger_,
                    "[sml] get_profile_list_response with unsupported profile: " << profile << " (" << obis::get_name(profile)
                                                                                 << ") from server " << srv_id_to_str(srv, true)
                                                                                 << ", trx: " << trx);
            }
        }

        //
        // convert value map from
        // std::map<cyng::obis, cyng::param_map_t>
        // to
        // std::map<cyng::obis, cyng::object> == cyng::prop_map_t
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
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
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

        //  client, server, obis, sensor-time, gw-time, sml_list_t (data)
        auto const r = smf::sml::read_get_list_response(msg);

        CYNG_LOG_INFO(logger_, "[sml] list response from " << srv_id_to_str(std::get<1>(r), true));
        for (auto const &ro : std::get<5>(r)) {
            CYNG_LOG_TRACE(logger_, ro.first << ": " << ro.second);
        }

        //  list response from 01-e61e-57140621-36-03
        // 0000600601ff: %(("raw":3715),("scaler":0),("status":0),("unit":4),("unit.name":day),("valTime":1970-...,("value":3715))
        // 0100000102ff: %(("raw":525),("scaler":0),("status":0),("unit":7),("unit.name":second),("valTime":1970-...),("value":525))
        // 0100020000ff:
        // %(("raw":1632093),("scaler":-2),("status":0),("unit":d),("unit.name":m3),("valTime":1970-...),("value":16320.93))

        auto const pmap = convert_to_param_map(std::get<5>(r));
        //
        //  send to writers
        //
        for (auto writer : writers_) {
            ctl_.get_registry().dispatch(
                writer,
                "get.profile.list.response",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                trx,
                std::get<1>(r),                      //  [buffer_t] server id
                std::chrono::system_clock::now(),    //  [cyng::object] actTime
                std::uint32_t(0),                    //  [u32] status
                cyng::obis_path_t({std::get<2>(r)}), //  [obis_path_t] path
                pmap);
        }
    }

    cyng::prop_map_t convert_to_param_map(sml::sml_list_t const &inp) {
        cyng::prop_map_t r;
        std::transform(
            std::begin(inp),
            std::end(inp),
            std::inserter(r, r.end()),
            [](sml::sml_list_t::value_type value) -> cyng::prop_map_t::value_type {
                //  Add the original key as value. This saves a reconversion from string type
                value.second.emplace("key", cyng::make_object(value.first));
                value.second.emplace("code", cyng::make_object(obis::get_name(value.first)));
                value.second.emplace("descr", cyng::make_object(obis::get_description(value.first)));

                return {value.first, cyng::make_object(value.second)};
            });
        return r;
    }

} // namespace smf
