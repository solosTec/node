#include <proxy.h>
#include <session.h>

#include <smf/config/schemes.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/list.h>
#include <smf/sml/reader.h>
#include <smf/sml/select.h>
#include <smf/sml/serializer.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

#include <boost/uuid/nil_generator.hpp>
#include <iostream>

namespace smf {

    proxy::proxy(cyng::logger logger, ipt_session &session, bus &cluster_bus, cyng::mac48 const &client_id)
        : state_(state::OFF)
        , logger_(logger)
        , session_(session)
        , cluster_bus_(cluster_bus)
        , client_id_(client_id)
        , parser_([this](
                      std::string trx,
                      std::uint8_t group_no,
                      std::uint8_t,
                      sml::msg_type type,
                      cyng::tuple_t msg,
                      std::uint16_t crc) {
            switch (type) {
            case sml::msg_type::OPEN_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
                // open_response(trx, msg);
                break;
            case sml::msg_type::GET_PROFILE_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
                // get_profile_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::GET_PROC_PARAMETER_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
                {
                    // std::cout << cyng::io::to_pretty(msg) << std::endl;
                    // CYNG_LOG_DEBUG(logger_, cyng::io::to_pretty(msg));
                    //  cyng::obis, cyng::attr_t, cyng::tuple_t
                    auto const [s, p, code, a, l] = sml::read_get_proc_parameter_response(msg);
                    CYNG_LOG_DEBUG(logger_, "server: " << s);
                    CYNG_LOG_DEBUG(logger_, "path  : " << p);
                    CYNG_LOG_DEBUG(logger_, "code  : " << code << " - " << obis::get_name(code));
                    CYNG_LOG_DEBUG(logger_, "attr  : " << a);
                    CYNG_LOG_DEBUG(logger_, "list  : \n" << sml::dump_child_list(l));
                    get_proc_parameter_response(s, p, code, a, l);
                }
                break;
            case sml::msg_type::GET_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
                // get_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::CLOSE_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
                close_response(trx, msg);
                break;
            case sml::msg_type::ATTENTION_RESPONSE: {
                auto const code = sml::read_attention_response(msg);
                CYNG_LOG_WARNING(
                    logger_, "[sml] " << sml::get_name(type) << ": " << trx << ", " << msg << " - " << obis::get_name(code));
            } break;
            default:
                CYNG_LOG_WARNING(logger_, "[sml] " << sml::get_name(type) << ": " << trx << ", " << msg);
                break;
            }
        })
        , id_()
        , req_gen_("operator", "operator")
        , cfg_()
        , cache_()
        , throughput_{} {

        if (!cache_.create_table(config::get_store_cfg_set())) {
            CYNG_LOG_FATAL(logger_, "[proxy] cannot create table cfgSet");
        }
    }

    proxy::~proxy() {
#ifdef _DEBUG_IPT
        std::cout << "proxy(~)" << std::endl;
#endif
    }

    bool proxy::is_on() const { return state_ != state::OFF; }

    void proxy::cfg_backup(std::string name, std::string pwd, cyng::buffer_t id) {

        CYNG_LOG_TRACE(logger_, "[proxy] config backup " << name << '@' << id);
        BOOST_ASSERT_MSG(state_ != state::ON, "proxy already active");
        state_ = state::ROOT_CFG;

        //
        //  update connection state
        //
        update_connect_state(true);

        cfg_.clear();
        cfg_.emplace(id, sml::tree{});

        //
        // * login to gateway
        // * send SML requests to get current configuration
        // * parse SML responses
        // * update "connection" table
        // * update some progress information
        // * write SML data into config table (setup node)
        // * ready
        //

        req_gen_.reset(name, pwd, 11);

        session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
            sml::messages_t messages;
            messages.emplace_back(req_gen_.public_open(client_id_, id_));
            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_LAN_DSL));
            // messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_STATE));

            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_DEVICE_IDENT)); //  OK
            // temporary removed: messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_PARAM)); //  OK
            // temporary removed: messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACTIVE_DEVICES)); //  OK

            //
            //  access rights need to know the active devices, so this comes first
            //
            // temporary removed: messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACCESS_RIGHTS));
            // temporary removed: messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_WAN_PARAM)); //  cannot read
            // temporary removed: messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_NTP));
            //  messages.emplace_back(req_gen_.get_proc_parameter(id_, ROOT_DEVICE_INFO));   //  empty result set

            messages.emplace_back(req_gen_.public_close());
            auto msg = sml::to_sml(messages);
#ifdef __DEBUG
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, msg.begin(), msg.end());
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(logger_, "[get_proc_parameter request] :\n" << dmp);
            }
#endif
            return session_.serializer_.escape_data(std::move(msg));
        });
    }

    // void proxy::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[proxy] stop"); }
    void proxy::read(cyng::buffer_t &&data) {

        //
        //  parse SML data
        //
        parser_.read(std::begin(data), std::end(data));

        //
        //  update throughput
        //
        throughput_ += data.size();
        auto const key_conn = cyng::key_generator(cyng::merge(session_.dev_, session_.vm_.get_tag()));
        session_.cluster_bus_.req_db_update("connection", key_conn, cyng::param_map_factory()("throughput", throughput_));
    }

    void proxy::get_proc_parameter_response(
        cyng::buffer_t const &srv,
        cyng::obis_path_t const &path,
        cyng::obis code,
        cyng::attr_t,
        cyng::tuple_t const &tpl) {
        if (code == OBIS_ROOT_NTP) {
            //
            //  send to configuration storage
            //
            sml::collect(tpl, {}, [&, this](cyng::obis_path_t const &op, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_NTP [" << op << "]: " << attr.second);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_NTP [" << srv << "]: "
                                          << " has no entry");
                }
            });

        } else if (code == OBIS_ROOT_LAN_DSL) {
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_LAN_DSL [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_LAN_DSL [" << srv << "]: "
                                              << " has no entry");
                }
            });
        } else if (code == OBIS_ROOT_DEVICE_IDENT) {
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_DEVICE_IDENT [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_DEVICE_IDENT [" << srv << "]: "
                                                   << " has no entry");
                }
            });

        } else if (code == OBIS_ROOT_WAN_PARAM) {
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_WAN_PARAM [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_WAN_PARAM [" << srv << "]: "
                                                << " has no entry");
                }
            });
        } else if (code == OBIS_ROOT_IPT_PARAM) {
            //
            //  send to configuration storage
            //
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_IPT_PARAM [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_IPT_PARAM [" << srv << "]: "
                                                << " has no entry");
                }
            });

        } else if (code == OBIS_ROOT_ACCESS_RIGHTS) {
            //
            //  query configuration
            //
            CYNG_LOG_TRACE(logger_, "OBIS_ROOT_ACCESS_RIGHTS \n" << sml::dump_child_list(tpl));
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_ACCESS_RIGHTS [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_ACCESS_RIGHTS [" << srv << "]: "
                                                    << " has no entry");
                }
            });

            // session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
            //     CYNG_LOG_INFO(logger_, "[proxy] backup " << cfg_.size() << " access");
            //     sml::messages_t messages;
            //     messages.emplace_back(req_gen_.public_open(client_id_, id_));
            //     messages.emplace_back(req_gen_.get_proc_parameter_access(id_, 0x03, 0x01, 0x0101));
            //     // messages.emplace_back(req_gen_.get_proc_parameter_access(id_, 0x03, 0x01, 0x0102));
            //     messages.emplace_back(req_gen_.public_close());
            //     auto msg = sml::to_sml(messages);
            //     return session_.serializer_.escape_data(std::move(msg));
            // });

        } else if (code == OBIS_ROOT_ACTIVE_DEVICES) {
            //
            //  query device configuration (81 81 11 06 ff ff)
            //  Add all found meters to the cfg_ member.
            //
            //
            sml::select(tpl, code, [&, this](cyng::prop_map_t const &om, std::size_t idx) {
                CYNG_LOG_TRACE(logger_, "OBIS_ROOT_ACTIVE_DEVICES#" << idx << " - " << om);
                auto const pos = om.find(OBIS_SERVER_ID);
                if (pos != om.end()) {
                    cyng::buffer_t id;
                    id = cyng::value_cast(pos->second, id);
                    cfg_.emplace(id, sml::tree{});
                }
            });

        } else if (code == OBIS_ROOT_SENSOR_PARAMS) {
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_SENSOR_PARAMS [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_SENSOR_PARAMS [" << srv << "]: "
                                                    << " has no entry");
                }
            });

        } else if (code == OBIS_ROOT_DATA_COLLECTOR) {
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_DATA_COLLECTOR [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_DATA_COLLECTOR [" << srv << "]: "
                                                     << " has no entry");
                }
            });
        } else if (code == OBIS_ROOT_PUSH_OPERATIONS) {
            //  {{ 8181c78a0101:obis, #0, null
            //  . . { { 8181c78a02ff:obis, #1,f0, {}},    -  push intervall in seconds
            //  . . . { 8181c78a03ff:obis, #1,0,  {}},    -  push delay in seconds
            //  . . . { 8181c78a04ff:obis, #1,8181c78a42ff - push source (profile, install or sensor list)
            //  . . . . { { 8181c78a81ff:obis, #1,01a815743145040102, {}},
            //  . . . . . { 8181c78a83ff:obis, #1,8181c78611ff, {}},
            //  . . . . . { 8181c78a82ff:obis, #0,null, {}}}},
            //  . . . { 8147170700ff:obis, #1,power@solostec,{}},    -  push target name
            //  . . . { 8149000010ff:obis, #1,8181c78a21ff, {}}
            //}}}
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_PUSH_OPERATIONS [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_PUSH_OPERATIONS [" << srv << "]: "
                                                      << " has no entry");
                }
            });

        } else {
            CYNG_LOG_WARNING(logger_, "unknown get proc parameter response :" << obis::get_name(code));
        }
    }

    void proxy::cfg_backup_meter() {
        //  ROOT_SENSOR_PARAMS (81 81 c7 86 00 ff - Datenspiegel)
        //  ROOT_DATA_COLLECTOR (81 81 C7 86 20 FF - Datensammler)
        //  IF_1107 (81 81 C7 93 00 FF - serial interface)
        session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
            CYNG_LOG_INFO(logger_, "[proxy] backup " << cfg_.size() << " meters");
            sml::messages_t messages;
            messages.emplace_back(req_gen_.public_open(client_id_, id_));
            for (auto const &cfg : cfg_) {
                CYNG_LOG_TRACE(logger_, "[proxy] meter backup " << cfg.first);
                messages.emplace_back(req_gen_.get_proc_parameter(cfg.first, OBIS_ROOT_SENSOR_PARAMS));
                messages.emplace_back(req_gen_.get_proc_parameter(cfg.first, OBIS_ROOT_DATA_COLLECTOR));
                messages.emplace_back(req_gen_.get_proc_parameter(cfg.first, OBIS_ROOT_PUSH_OPERATIONS));
            }
            messages.emplace_back(req_gen_.public_close());
            auto msg = sml::to_sml(messages);
            return session_.serializer_.escape_data(std::move(msg));
        });
    }

    void proxy::cfg_backup_access() {
        session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
            CYNG_LOG_INFO(logger_, "[proxy] backup " << cfg_.size() << " access");
            sml::messages_t messages;
            messages.emplace_back(req_gen_.public_open(client_id_, id_));
            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACCESS_RIGHTS));
            messages.emplace_back(req_gen_.public_close());

            messages.emplace_back(req_gen_.public_open(client_id_, id_));
            messages.emplace_back(req_gen_.get_proc_parameter_access(id_, 0x03, 0x01, 0x0101));
            messages.emplace_back(req_gen_.public_close());
            auto msg = sml::to_sml(messages);
            return session_.serializer_.escape_data(std::move(msg));
        });
    }

    void proxy::close_response(std::string trx, cyng::tuple_t const msg) {
        switch (state_) {
        case state::ROOT_CFG:
            if (!cfg_.empty()) {
                state_ = state::METER_CFG;
                cfg_backup_meter();
            } else {
                state_ = state::OFF;
                complete();
            }
            break;
        case state::METER_CFG:
            state_ = state::ACCESS_CFG;
            cfg_backup_access();
            break;
        default:
            state_ = state::OFF;
            complete();
            break;
        }
    }

    void proxy::complete() {
        //
        //  update connection state
        //
        update_connect_state(false);

        CYNG_LOG_INFO(logger_, "readout of " << cfg_.size() << " server/clients complete")
        for (auto const &cfg : cfg_) {
            auto const cl = cfg.second.to_child_list();
            CYNG_LOG_TRACE(logger_, "server " << cfg.first << " complete: \n" << sml::dump_child_list(cl));
        }
    }

    void proxy::update_connect_state(bool connected) {
        //
        //  update session state
        //
        // auto const b1 = tbl_session->modify(key_caller, cyng::make_param("cTag", dev), cfg_.get_tag());
        if (connected) {
            session_.cluster_bus_.req_db_update(
                "session", cyng::key_generator(session_.dev_), cyng::param_map_factory()("cTag", session_.dev_));
        } else {
            session_.cluster_bus_.req_db_update(
                "session", cyng::key_generator(session_.dev_), cyng::param_map_factory()("cTag", boost::uuids::nil_uuid()));
        }

        //
        //  build key for "connection" table
        //
        auto const key_conn = cyng::key_generator(cyng::merge(session_.dev_, session_.vm_.get_tag()));
        if (connected) {
            session_.cluster_bus_.req_db_insert(
                "connection",
                key_conn,
                cyng::data_generator(
                    session_.name_ + "-proxy",
                    session_.name_,
                    std::chrono::system_clock::now(),
                    true,  //  local
                    "ipt", //  protocol layer
                    "ipt",
                    static_cast<std::uint64_t>(0)),
                1);
        } else {
            session_.cluster_bus_.req_db_remove("connection", key_conn);
        }
    }

} // namespace smf
