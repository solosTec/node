#include <proxy.h>
#include <session.h>

#include <smf/config/schemes.h>
#include <smf/mbus/flag_id.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/conv.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/list.h>
#include <smf/sml/select.h>
#include <smf/sml/serializer.h>
#include <smf/sml/status.h>
#include <smf/sml/value.hpp>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/task/channel.h>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <iostream>

namespace smf {

    proxy::proxy(cyng::logger logger, ipt_session &session, bus &cluster_bus, cyng::mac48 const &client_id)
        : state_(initial_s{})
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
            CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
            switch (type) {
            case sml::msg_type::OPEN_RESPONSE:
                // open_response(trx, msg);
                break;
            case sml::msg_type::GET_PROFILE_LIST_RESPONSE: {
                auto const [s, p, act, reg, val, stat, l] = sml::read_get_profile_list_response(msg);
                CYNG_LOG_DEBUG(logger_, "server   : " << s);
                CYNG_LOG_DEBUG(logger_, "path     : " << p);
                CYNG_LOG_DEBUG(logger_, "actTime  : " << act);
                CYNG_LOG_DEBUG(logger_, "regPeriod: " << reg.count() << " seconds");
                CYNG_LOG_DEBUG(logger_, "valTime  : " << val);
                CYNG_LOG_DEBUG(logger_, "status   : " << stat);
                CYNG_LOG_DEBUG(logger_, "index    : " << state_.index());
                CYNG_LOG_DEBUG(logger_, "list size: " << l.size());
                // for (auto const &entry : l) {
                //     CYNG_LOG_DEBUG(logger_, entry.first << ": " << entry.second);
                // }
                on(evt_get_profile_list_response{s, p, act, reg, val, stat, l});
            } break;
            case sml::msg_type::GET_PROC_PARAMETER_RESPONSE: {
                // std::cout << cyng::io::to_pretty(msg) << std::endl;
                // CYNG_LOG_DEBUG(logger_, cyng::io::to_pretty(msg));
                //  cyng::obis, cyng::attr_t, cyng::tuple_t
                // CYNG_LOG_DEBUG(logger_, "state : " << get_state());
                auto const [s, p, code, a, l] = sml::read_get_proc_parameter_response(msg);
                CYNG_LOG_DEBUG(logger_, "server: " << s);
                CYNG_LOG_DEBUG(logger_, "path  : " << p);
                CYNG_LOG_DEBUG(logger_, "code  : " << code << " - " << obis::get_name(code));
                CYNG_LOG_DEBUG(logger_, "attr  : " << a);
                CYNG_LOG_DEBUG(logger_, "index : " << state_.index());
                CYNG_LOG_DEBUG(logger_, "list  : \n" << sml::dump_child_list(l, false));

                on(evt_get_proc_parameter_response{s, p, code, a, l});
            } break;
            case sml::msg_type::SET_PROC_PARAMETER_RESPONSE: {
                // auto const [s, p, code, a, l] = sml::read_set_proc_parameter_response(msg);
            } break;
            case sml::msg_type::GET_LIST_RESPONSE:
                // get_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::CLOSE_RESPONSE:
                on(evt_close_response{trx, msg});
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
        , req_gen_("operator", "operator")
        , throughput_{} {}

    bool proxy::is_online() const {
        return std::visit([](auto const &s) { return s.get_online_state(); }, state_);
    }

    void proxy::get_proc_param_req(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        cyng::buffer_t id,
        cyng::obis section,
        boost::uuids::uuid source,
        cyng::obis_path_t path,
        boost::uuids::uuid rpeer) {
        CYNG_LOG_TRACE(
            logger_, "[proxy] GetProcParameterRequest " << name << '@' << id << ", section: " << obis::get_name(section));

        req_gen_.reset(name, pwd, 18);
        state_ = get_proc_param_req_s{tag, id, section, source, rpeer, true};
        on(evt_init{});
    }
    void proxy::set_proc_param_req(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        cyng::buffer_t id,
        cyng::obis section,
        cyng::param_map_t params,
        boost::uuids::uuid source,
        cyng::obis_path_t path,
        boost::uuids::uuid tag_cluster) {
        CYNG_LOG_TRACE(logger_, "[proxy] SetProcParameterRequest " << path << " -> " << params);

        req_gen_.reset(name, pwd, 18);
        state_ = set_proc_param_req_s{tag, id, section, params, source, tag_cluster};
        on(evt_init{});
    }

    void proxy::get_profile_list_req(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        cyng::buffer_t id,
        cyng::obis section,
        cyng::param_map_t params,
        boost::uuids::uuid source,
        boost::uuids::uuid tag_cluster) {

        req_gen_.reset(name, pwd, 18);
        state_ = get_profile_list_req_s{tag, id, section, params, source, tag_cluster};
        on(evt_init{});
    }

    void proxy::cfg_backup(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        cyng::buffer_t id,
        std::string fw, //  firware version
        std::chrono::system_clock::time_point tp) {

        CYNG_LOG_TRACE(logger_, "[proxy] config backup " << name << '@' << id);
        // BOOST_ASSERT_MSG(state_ == state::OFF, "proxy already active");
        BOOST_ASSERT_MSG(!tag.is_nil(), "no key");
        BOOST_ASSERT_MSG(!id.empty(), "no gateway id");

        //
        // * login to gateway
        // * send SML requests to get current configuration
        // * parse SML responses
        // * update "connection" table
        // * update some progress information
        // * write SML data into config table (setup node)
        // * ready
        //

        req_gen_.reset(name, pwd, 22);
        state_ = backup_s{backup_s::query_state::OFF, tag, id, fw, tp, {}, {}};
        on(evt_init{});
    }

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

    std::string proxy::backup_s::get_state() const {
        switch (state_) {
        case query_state::OFF:
            return "OFF";
        case query_state::QRY_BASIC:
            return "BASIC";
        case query_state::QRY_METER:
            return "METER";
        case query_state::QRY_ACCESS:
            return "ACCESS";
        case query_state::QRY_USER:
            return "USER";
        default:
            break;
        }
        return "invalid state";
    }

    void proxy::backup_s::on(proxy &prx, evt_init) {

        //
        //  update connection state - QRY_BASIC
        //
        update_connect_state(prx.session_, true);

        cfg_.emplace(id_, sml::tree_t{});

        //
        //  query some basic configurations and all active devices
        //
        prx.session_.ipt_send([&, this]() mutable -> cyng::buffer_t {
            sml::messages_t messages;
            messages.emplace_back(prx.req_gen_.public_open(prx.client_id_, id_));
            messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_LAN_DSL));
            messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_PARAM)); //  OK
            // messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_STATE)); this is not configuration data
            messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_NTP));

            messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_DEVICE_IDENT));   //  OK
            messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACTIVE_DEVICES)); //  OK

            messages.emplace_back(prx.req_gen_.public_close());
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
            prx.throughput_ += msg.size();
            auto const open_res = prx.req_gen_.get_trx_mgr().store_pefix();
            CYNG_LOG_TRACE(prx.session_.logger_, "prefix #" << open_res << ": " << prx.req_gen_.get_trx_mgr().get_prefix());
            return prx.session_.serializer_.escape_data(std::move(msg));
        });
    }

    void proxy::backup_s::on(proxy &prx, evt_get_proc_parameter_response &&evt) {

        if (evt.code_ == OBIS_ROOT_NTP) {
            //
            //  send to configuration storage
            //
            sml::collect(evt.tpl_, {evt.code_}, [&, this](cyng::obis_path_t const &op, cyng::attr_t const &attr) {
                auto pos = cfg_.find(evt.srv_);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(prx.logger_, "OBIS_ROOT_NTP [" << op << "]: " << attr.second);
                    pos->second.add(evt.path_, attr);
                } else {
                    CYNG_LOG_WARNING(
                        prx.logger_,
                        "OBIS_ROOT_NTP [" << evt.srv_ << "]: "
                                          << " has no entry");
                }
            });

        } else if (
            evt.code_ == OBIS_ROOT_LAN_DSL || evt.code_ == OBIS_ROOT_DEVICE_IDENT || evt.code_ == OBIS_ROOT_WAN_PARAM ||
            evt.code_ == OBIS_ROOT_IPT_PARAM || evt.code_ == OBIS_ROOT_DATA_COLLECTOR || evt.code_ == OBIS_ROOT_PUSH_OPERATIONS ||
            evt.code_ == OBIS_ROOT_SENSOR_PARAMS) {
            sml::collect(evt.tpl_, {evt.code_}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(evt.srv_);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(prx.logger_, obis::get_name(evt.code_) << " [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        prx.logger_,
                        obis::get_name(evt.code_) << " [" << evt.srv_ << "]: "
                                                  << " has no entry");
                }
            });
        } else if (evt.code_.starts_with({0x81, 0x81, 0x81, 0x64}) && !access_.empty()) {
            auto const path = access_.front();
            access_.pop();

            //  attr contains the affected device
            auto const id = cyng::value_cast(evt.attr_.second, evt.srv_);
            CYNG_LOG_TRACE(prx.logger_, "user access rights for: " << id << " - " << path);
            sml::collect(evt.tpl_, path, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(id);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(prx.logger_, obis::get_name(evt.code_) << " [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        prx.logger_,
                        evt.code_ << " [" << evt.srv_ << "]: "
                                  << " has no entry");
                }
            });

        } else if (evt.code_ == OBIS_ROOT_ACCESS_RIGHTS) {
            //
            //  query configuration
            //
            CYNG_LOG_TRACE(prx.logger_, "OBIS_ROOT_ACCESS_RIGHTS \n" << sml::dump_child_list(evt.tpl_, false));
            sml::collect(evt.tpl_, {evt.code_}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(evt.srv_);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(prx.logger_, "OBIS_ROOT_ACCESS_RIGHTS [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                    if (!path.empty() && path.back().starts_with({0x81, 0x81, 0x81, 0x64})) {
                        access_.push(path);
                        CYNG_LOG_DEBUG(prx.logger_, "store path [" << path << "] #" << access_.size());
                    }
                } else {
                    CYNG_LOG_WARNING(
                        prx.logger_,
                        "OBIS_ROOT_ACCESS_RIGHTS [" << evt.srv_ << "]: "
                                                    << " has no entry");
                }
            });

        } else if (evt.code_ == OBIS_ROOT_ACTIVE_DEVICES) {
            //
            //  query device configuration (81 81 11 06 ff ff)
            //  Add all found meters to the cfg_ member.
            //
            //
            sml::select(evt.tpl_, evt.code_, [&, this](cyng::prop_map_t const &om, std::size_t idx) {
                CYNG_LOG_TRACE(prx.logger_, "OBIS_ROOT_ACTIVE_DEVICES#" << idx << " - " << om);
                auto const pos = om.find(OBIS_SERVER_ID);
                if (pos != om.end()) {
                    cyng::buffer_t id;
                    id = cyng::value_cast(pos->second, id);
                    if (id_ != id) {
                        //  meters only
                        meters_.push(id);
                    }
                    cfg_.emplace(id, sml::tree_t{});
                }
            });
        } else {
            CYNG_LOG_WARNING(prx.logger_, "unknown get proc parameter response: " << obis::get_name(evt.code_));
        }
    }
    void proxy::backup_s::on(proxy &prx, evt_close_response &&evt) {
        auto const [pending, success] = prx.req_gen_.get_trx_mgr().ack_trx(evt.trx_);
        if (success) {
            CYNG_LOG_TRACE(prx.logger_, pending << " transactions are open - " << evt.trx_ << ", proxy state: " << get_state());
        } else {
            CYNG_LOG_WARNING(prx.logger_, pending << " transactions are open - " << evt.trx_ << ", proxy state: " << get_state());
        }

        switch (state_) {
        case query_state::QRY_BASIC:
            if (!cfg_.empty()) {
                cfg_backup_meter(prx);
            } else {
                state_ = query_state::OFF;
                complete(prx);
            }
            break;
        case query_state::QRY_METER:
            if (meters_.empty()) {
                cfg_backup_access(prx);
            } else {
                cfg_backup_meter(prx);
            }
            break;
        case query_state::QRY_ACCESS:
            if (access_.empty()) {
                complete(prx);
            } else {
                //
                //  update state
                //
                state_ = query_state::QRY_USER;
                cfg_backup_user(prx);
            }
            break;
        case query_state::QRY_USER:
            if (access_.empty()) {
                complete(prx);
            } else {
                cfg_backup_user(prx);
            }
            break;
        default:
            break;
        }
    }

    void proxy::backup_s::cfg_backup_meter(proxy &prx) {
        //  ROOT_SENSOR_PARAMS (81 81 c7 86 00 ff - Datenspiegel)
        //  ROOT_DATA_COLLECTOR (81 81 C7 86 20 FF - Datensammler)
        //  IF_1107 (81 81 C7 93 00 FF - serial interface)
        BOOST_ASSERT_MSG(!id_.empty(), "no gateway id");
        BOOST_ASSERT(!meters_.empty());

        //
        //  update state
        //
        state_ = query_state::QRY_METER;

        if (!meters_.empty()) {

            prx.session_.ipt_send([&, this]() mutable -> cyng::buffer_t {
                sml::messages_t messages;
                messages.emplace_back(prx.req_gen_.public_open(prx.client_id_, id_));
                auto const id = meters_.front();
                meters_.pop();
                CYNG_LOG_INFO(
                    prx.logger_,
                    "[proxy] backup " << get_state() << " " << id << " - " << (cfg_.size() - meters_.size()) << "/" << cfg_.size());
                messages.emplace_back(prx.req_gen_.get_proc_parameter(id, OBIS_ROOT_SENSOR_PARAMS));
                messages.emplace_back(prx.req_gen_.get_proc_parameter(id, OBIS_ROOT_DATA_COLLECTOR));
                messages.emplace_back(prx.req_gen_.get_proc_parameter(id, OBIS_ROOT_PUSH_OPERATIONS));
                messages.emplace_back(prx.req_gen_.public_close());
                auto msg = sml::to_sml(messages);
                prx.throughput_ += msg.size();
                auto const open_res = prx.req_gen_.get_trx_mgr().store_pefix();
                CYNG_LOG_TRACE(prx.logger_, "prefix #" << open_res << ": " << prx.req_gen_.get_trx_mgr().get_prefix());
                return prx.session_.serializer_.escape_data(std::move(msg));
            });
        }
    }

    void proxy::backup_s::cfg_backup_access(proxy &prx) {
        BOOST_ASSERT_MSG(!id_.empty(), "no gateway id");
        BOOST_ASSERT(state_ == query_state::QRY_METER);

        //
        //  update state
        //
        state_ = query_state::QRY_ACCESS;

        prx.session_.ipt_send([&, this]() mutable -> cyng::buffer_t {
            CYNG_LOG_INFO(prx.logger_, "[proxy] backup " << get_state());
            sml::messages_t messages;
            messages.emplace_back(prx.req_gen_.public_open(prx.client_id_, id_));
            messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACCESS_RIGHTS));
            messages.emplace_back(prx.req_gen_.public_close());
            auto msg = sml::to_sml(messages);
            prx.throughput_ += msg.size();
            auto const open_res = prx.req_gen_.get_trx_mgr().store_pefix();
            CYNG_LOG_TRACE(prx.logger_, "prefix #" << open_res << ": " << prx.req_gen_.get_trx_mgr().get_prefix());
            return prx.session_.serializer_.escape_data(std::move(msg));
        });
    }

    void proxy::backup_s::cfg_backup_user(proxy &prx) {

        //  1b 1b 1b 1b 01 01 01 01  76 81 06 32 32 30 32 31
        //  30 31 36 35 33 34 33 39  37 36 39 34 38 2d 31 62
        //  00 62 00 72 63 01 00 77  01 07 00 50 56 c0 00 08
        //  0f 32 30 32 32 30 32 31  30 31 36 35 33 34 33 08
        //  05 00 15 3b 01 ec 46 09  6f 70 65 72 61 74 6f 72
        //  09 6f 70 65 72 61 74 6f  72 01 63 41 64 00 76 81
        //  06 32 32 30 32 31 30 31  36 35 33 34 33 39 37 36
        //  39 34 38 2d 32 62 01 62  00 72 63 05 00 75 08 05
        //  00 15 3b 01 ec 46 09 6f  70 65 72 61 74 6f 72 09
        //  6f 70 65 72 61 74 6f 72  74 07 81 81 81 60 ff ff
        //  07 81 81 81 60 03 ff 07  81 81 81 60 03 01 07 81
        //  81 81 64 01 01 01 63 85  73 00 76 81 06 32 32 30
        //  32 31 30 31 36 35 33 34  33 39 37 36 39 34 38 2d
        //  33 62 00 62 00 72 63 02  00 71 01 63 72 cf 00 00
        //  1b 1b 1b 1b 1a 01 c0 5f

        //  76                                                SML_Message(Sequence):
        //    81063232303231303136353334333937363934382D32    transactionId: 220210165343976948-2
        //    6201                                            groupNo: 1
        //    6200                                            abortOnError: 0
        //    72                                              messageBody(Choice):
        //      630500                                        messageBody: 1280 => SML_GetProcParameter_Req (0x00000500)
        //      75                                            SML_GetProcParameter_Req(Sequence):
        //        080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46
        //        096F70657261746F72                          username: operator
        //        096F70657261746F72                          password: operator
        //        74                                          parameterTreePath(SequenceOf):
        //          0781818160FFFF                            path_Entry: ___`__
        //          078181816003FF                            path_Entry: 81 81 81 60 03 FF
        //          07818181600301                            path_Entry: 81 81 81 60 03 01
        //          07818181640101                            path_Entry: 81 81 81 64 01 01
        //        01                                          attribute: not set
        //    638573                                          crc16: 34163
        //    00                                              endOfSmlMsg: 00

        BOOST_ASSERT_MSG(!id_.empty(), "no gateway id");
        BOOST_ASSERT(!access_.empty());

        if (!access_.empty()) {

            prx.session_.ipt_send([&, this]() mutable -> cyng::buffer_t {
                // CYNG_LOG_INFO(logger_, "[proxy] backup " << get_state());
                BOOST_ASSERT(!id_.empty());
                sml::messages_t messages;
                auto const &path = access_.front();
                CYNG_LOG_INFO(prx.logger_, "[proxy] backup " << get_state() << " " << path << " - " << access_.size());

                messages.emplace_back(prx.req_gen_.public_open(prx.client_id_, id_));
                messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, path));
                // messages.emplace_back(req_gen_.get_proc_parameter_access(id_, 0x03, 0x01, 0x0101));
                //  messages.emplace_back(req_gen_.get_proc_parameter_access(id_, 0x03, 0x01, 0x0102));
                messages.emplace_back(prx.req_gen_.public_close());
                auto msg = sml::to_sml(messages);
#ifdef __DEBUG
                {
                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, msg.begin(), msg.end());
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[query user access right] :\n" << dmp);
                }
#endif
                prx.throughput_ += msg.size();
                auto const open_res = prx.req_gen_.get_trx_mgr().store_pefix();
                CYNG_LOG_TRACE(prx.logger_, "prefix #" << open_res << ": " << prx.req_gen_.get_trx_mgr().get_prefix());
                return prx.session_.serializer_.escape_data(std::move(msg));
            });
        }
    }
    void proxy::backup_s::complete(proxy &prx) {
        //
        //  update connection state
        //
        update_connect_state(prx.session_, false);

        boost::uuids::random_generator_mt19937 uidgen;
        auto const tag = uidgen();
        std::size_t counter{0};

        CYNG_LOG_INFO(prx.logger_, "readout of " << cfg_.size() << " server/clients complete - tag: " << tag)
        for (auto const &cfg : cfg_) {
            auto const cl = cfg.second.to_child_list();
            CYNG_LOG_TRACE(prx.logger_, "server " << cfg.first << " complete: \n" << sml::dump_child_list(cl, true));

            //
            //  send child list to configuration manager
            //
            counter += backup(prx, tag, cfg.first, cl);
        }
        CYNG_LOG_INFO(prx.logger_, counter << " config records backuped - write meta data");
        std::stringstream ss;
        ss << cyng::to_string(id_) << " has " << counter << " entries";
        prx.cluster_bus_.req_db_insert(
            "cfgSetMeta", cyng::key_generator(tag), cyng::data_generator(tag_, fw_, start_time_, ss.str()), 0);
    }

    void proxy::backup_s::update_connect_state(ipt_session &ipts, bool connected) {
        //
        //  update session state
        //
        if (connected) {
            state_ = query_state::QRY_BASIC;
            ipts.cluster_bus_.req_db_update(
                "session", cyng::key_generator(ipts.dev_), cyng::param_map_factory()("cTag", ipts.dev_));
        } else {
            state_ = query_state::OFF;
            ipts.cluster_bus_.req_db_update(
                "session", cyng::key_generator(ipts.dev_), cyng::param_map_factory()("cTag", boost::uuids::nil_uuid()));
        }

        //
        //  communicate to main node
        //
        ipts.update_connect_state(connected);
    }

    std::size_t proxy::backup_s::backup(proxy &prx, boost::uuids::uuid tag, cyng::buffer_t meter, cyng::tuple_t tpl) {
        std::size_t counter{0};
        sml::collect(tpl, {}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
            CYNG_LOG_DEBUG(prx.logger_, "send to config manager: " << tag << ", " << meter << ", " << path << ", " << attr);
            prx.cluster_bus_.cfg_backup_merge(tag, id_, meter, path, attr.second);
            ++counter;
        });
        return counter;
    }

    bool proxy::backup_s::get_online_state() const { return state_ != query_state::OFF; }

    void proxy::get_proc_param_req_s::get_proc_param_req_s::on(proxy &prx, evt_init) {

        //
        //  update connection state
        //
        prx.session_.update_connect_state(true);
        online_ = true;

        //
        //  query proc parameter
        //
        prx.session_.ipt_send([&, this]() mutable -> cyng::buffer_t {
            sml::messages_t messages;
            messages.emplace_back(prx.req_gen_.public_open(prx.client_id_, id_));
            messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, root_));
            if (root_ == OBIS_ROOT_IPT_PARAM) {
                //  ipt parameters will be complemented by an ipt state
                messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_STATE));
            }
            if (root_ == OBIS_ROOT_VISIBLE_DEVICES) {
                //  visible devices will be complemented by active devices
                messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACTIVE_DEVICES));
            }
            if (root_ == OBIS_ROOT_W_MBUS_PARAM) {
                //  get wireless M-Bus status too
                messages.emplace_back(prx.req_gen_.get_proc_parameter(id_, OBIS_ROOT_W_MBUS_STATUS));
            }
            messages.emplace_back(prx.req_gen_.public_close());
            auto msg = sml::to_sml(messages);
            prx.throughput_ += msg.size();
            auto const open_res = prx.req_gen_.get_trx_mgr().store_pefix();
            CYNG_LOG_TRACE(prx.logger_, "prefix #" << open_res << ": " << prx.req_gen_.get_trx_mgr().get_prefix());
            return prx.session_.serializer_.escape_data(std::move(msg));
        });
    }

    void proxy::get_proc_param_req_s::on(proxy &prx, evt_get_proc_parameter_response &&evt) {
        if (evt.code_ == OBIS_CLASS_OP_LOG_STATUS_WORD) {
            //  example response:
            //  [2022-03-07T10:52:25+0100] DEBUG 2a36b3de -- msg: 222178441860016453-2 - GET_PROC_PARAMETER_RESPONSE
            //  [2022-03-07T10:52:25+0100] DEBUG 2a36b3de -- server: 0500153b01ec46
            //  [2022-03-07T10:52:25+0100] DEBUG 2a36b3de -- path  : 810060050000
            //  [2022-03-07T10:52:25+0100] DEBUG 2a36b3de -- code  : 810060050000 - CLASS_OP_LOG_STATUS_WORD
            //  [2022-03-07T10:52:25+0100] DEBUG 2a36b3de -- attr  : (1:00070202)
            //  [2022-03-07T10:52:25+0100] DEBUG 2a36b3de -- list  :
            auto const word = cyng::value_cast<sml::status_word_t>(evt.attr_.second, 0);
            CYNG_LOG_INFO(prx.logger_, "status word: " << word);
            prx.session_.cluster_bus_.cfg_sml_channel_back(
                cyng::key_generator(cyng::to_string(id_)),                  //  key
                sml::get_name(sml::msg_type::GET_PROC_PARAMETER_RESPONSE),  //  channel
                root_,                                                      //  section
                cyng::param_map_factory()("word", sml::to_param_map(word)), //  params
                source_,
                rpeer_);
        } else if (
            evt.code_ == OBIS_ROOT_IPT_PARAM || evt.code_ == OBIS_ROOT_IPT_STATE || evt.code_ == OBIS_ROOT_DEVICE_IDENT ||
            evt.code_ == OBIS_ROOT_MEMORY_USAGE || evt.code_ == OBIS_ROOT_W_MBUS_PARAM || evt.code_ == OBIS_ROOT_W_MBUS_STATUS ||
            evt.code_ == OBIS_IF_1107 || evt.code_ == OBIS_CLASS_OP_LOG || evt.code_ == OBIS_ROOT_NTP) {

            //  //  replace attribute by it's object
            auto const pm = smf::sml::compress(evt.tpl_);
            prx.session_.cluster_bus_.cfg_sml_channel_back(
                cyng::key_generator(cyng::to_string(id_)),                 //  key
                sml::get_name(sml::msg_type::GET_PROC_PARAMETER_RESPONSE), //  channel
                evt.code_,                                                 //  section
                cyng::to_param_map(pm),                                    //  props
                source_,
                rpeer_);
        } else if (evt.code_ == OBIS_ROOT_VISIBLE_DEVICES || evt.code_ == OBIS_ROOT_ACTIVE_DEVICES) {
            // CYNG_LOG_DEBUG(prx.logger_, "device response: " << evt.tpl_);
            BOOST_ASSERT(!evt.srv_.empty());
            std::size_t index = 0u;
            sml::select(evt.tpl_, evt.code_, [&, this](cyng::prop_map_t const &props, std::size_t depth) {
                auto pm = cyng::to_param_map(props); //  props
                pm.emplace("nr", cyng::make_object(index));
                pm.erase("8181c78202ff"); //  "---"
                //  get meter id
                auto const pos = pm.find("8181c78204ff");
                if (pos != pm.end()) {
                    //  meter id
                    auto const id = cyng::to_buffer(pos->second);
                    auto const type = detect_server_type(id);
                    pm.emplace("type", cyng::make_object(static_cast<std::uint32_t>(type)));
                    if (type == srv_type::MBUS_WIRED || type == srv_type::MBUS_RADIO) {
                        auto srv_id = to_srv_id(id);
                        pm.emplace("serial", cyng::make_object(get_id_as_buffer(srv_id)));
                        auto const [c1, c2] = get_manufacturer_code(srv_id);
                        pm.emplace("maker", cyng::make_object(mbus::decode(c1, c2)));
                    } else {
                        pm.emplace("serial", cyng::make_object(id));
                        pm.emplace("maker", cyng::make_object("")); //  ToDo: dash
                    }
                    pm.emplace("visible", cyng::make_object(evt.code_ == OBIS_ROOT_VISIBLE_DEVICES));
                    pm.emplace("active", cyng::make_object(evt.code_ == OBIS_ROOT_ACTIVE_DEVICES));
                    pm.emplace("serverId", cyng::make_object(cyng::clone(evt.srv_)));
                    CYNG_LOG_TRACE(prx.logger_, "device " << pm);
                    prx.session_.cluster_bus_.cfg_sml_channel_back(
                        cyng::key_generator(cyng::to_string(id_)),                 //  key
                        sml::get_name(sml::msg_type::GET_PROC_PARAMETER_RESPONSE), //  channel
                        evt.code_,                                                 //  section
                        pm,
                        source_,
                        rpeer_);
                    ++index;
                } else {
                    CYNG_LOG_WARNING(prx.logger_, "no meter id: " << pm);
                }
            });

        } else {
            CYNG_LOG_WARNING(prx.logger_, "unknown get proc parameter response: " << obis::get_name(evt.code_));
        }
    }
    void proxy::get_proc_param_req_s::on(proxy &prx, evt_close_response &&) {
        //
        //  close local connection
        //
        prx.session_.update_connect_state(false);
        online_ = false;
    }

    void proxy::set_proc_param_req_s::on(proxy &prx, evt_init) {

        //
        //  update connection state
        //
        prx.session_.update_connect_state(true);
        online_ = true;

        //  param
        //  81490d070002:8149633c0102 -> %(("path":[81490d070002,8149633c0102]),("value":LSMTest2))
        auto const reader = cyng::make_reader(params_);

        auto const vec = cyng::vector_cast<std::string>(reader.get("path"), "000000000000");
        BOOST_ASSERT_MSG(!vec.empty(), "SetProcParameter.Req without OBIS path");
        auto const ptree = obis::to_obis_path(vec);

        //  full path
        cyng::obis_path_t path{root_};
        path.insert(path.end(), ptree.begin(), ptree.end());

        auto attr = sml::to_attribute(reader.get("value"));

        //
        //  set proc parameter
        //
        prx.session_.ipt_send([&, this, path, attr]() mutable -> cyng::buffer_t {
            sml::messages_t messages;
            messages.emplace_back(prx.req_gen_.public_open(prx.client_id_, id_));

            messages.emplace_back(prx.req_gen_.set_proc_parameter(id_, path, attr));

            messages.emplace_back(prx.req_gen_.public_close());
            auto msg = sml::to_sml(messages);
#ifdef _DEBUG
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, msg.begin(), msg.end());
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(prx.logger_, "[set_proc_parameter request] :\n" << dmp);
            }
#endif
            prx.throughput_ += msg.size();
            auto const open_res = prx.req_gen_.get_trx_mgr().store_pefix();
            CYNG_LOG_TRACE(prx.logger_, "prefix #" << open_res << ": " << prx.req_gen_.get_trx_mgr().get_prefix());
            return prx.session_.serializer_.escape_data(std::move(msg));
        });

        //
        // Update name/password in device table.
        // Use only the primary redundancy since one device cannot have more than one set of credentials.
        //
        if (path ==
            cyng::obis_path_t({OBIS_ROOT_IPT_PARAM, {0x81, 0x49, 0x0d, 0x07, 0x00, 0x01}, {0x81, 0x49, 0x63, 0x3c, 0x01, 0x01}})) {

            prx.session_.cluster_bus_.req_db_update(
                "device", cyng::key_generator(tag_), cyng::param_map_factory()("name", attr.second));
        } else if (
            path ==
            cyng::obis_path_t({OBIS_ROOT_IPT_PARAM, {0x81, 0x49, 0x0d, 0x07, 0x00, 0x01}, {0x81, 0x49, 0x63, 0x3c, 0x02, 0x01}})) {

            prx.session_.cluster_bus_.req_db_update(
                "device", cyng::key_generator(tag_), cyng::param_map_factory()("pwd", attr.second));
        }
    }

    void proxy::set_proc_param_req_s::on(proxy &prx, evt_close_response &&) {
        //
        //  close local connection
        //
        prx.session_.update_connect_state(false);
        online_ = false;
    }

    void proxy::get_profile_list_req_s::on(proxy &prx, evt_init) {
        online_ = true;
        //
        //  query profile
        //
        prx.session_.ipt_send([&, this]() mutable -> cyng::buffer_t {
            sml::messages_t messages;
            auto const now = std::chrono::system_clock::now();

            messages.emplace_back(prx.req_gen_.public_open(prx.client_id_, id_));
            messages.emplace_back(prx.req_gen_.get_profile_list(id_, root_, now - std::chrono::hours(12), now));
            messages.emplace_back(prx.req_gen_.public_close());
            auto msg = sml::to_sml(messages);
#ifdef _DEBUG
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, msg.begin(), msg.end());
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(prx.logger_, "[get_profile_list request] :\n" << dmp);
            }
#endif
            prx.throughput_ += msg.size();
            auto const open_res = prx.req_gen_.get_trx_mgr().store_pefix();
            CYNG_LOG_TRACE(prx.logger_, "prefix #" << open_res << ": " << prx.req_gen_.get_trx_mgr().get_prefix());
            return prx.session_.serializer_.escape_data(std::move(msg));
        });
    }
    void proxy::get_profile_list_req_s::on(proxy &prx, evt_get_profile_list_response &&evt) {

        BOOST_ASSERT(!evt.path_.empty());

        cyng::param_map_t pm;
        std::transform(evt.list_.begin(), evt.list_.end(), std::inserter(pm, pm.end()), [](sml::sml_list_t::value_type const &v) {
            auto const pos = v.second.find("value");
            return (pos != v.second.end()) ? cyng::make_param(cyng::to_str(v.first), pos->second)
                                           : cyng::make_param(cyng::to_str(v.first), "-");
        });
        pm.emplace("status", cyng::make_object(evt.mbus_state_));

        prx.session_.cluster_bus_.cfg_sml_channel_back(
            cyng::key_generator(cyng::to_string(id_)),               //  key
            sml::get_name(sml::msg_type::GET_PROFILE_LIST_RESPONSE), //  channel
            evt.path_.front(),                                       //  section
            pm,                                                      //  props
            source_,
            rpeer_);
    }
    void proxy::get_profile_list_req_s::on(proxy &prx, evt_close_response &&) {
        //
        //  close local connection
        //
        prx.session_.update_connect_state(false);
        online_ = false;
    }
} // namespace smf
