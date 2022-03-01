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
#include <boost/uuid/random_generator.hpp>
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
                    CYNG_LOG_DEBUG(logger_, "state : " << get_state());
                    auto const [s, p, code, a, l] = sml::read_get_proc_parameter_response(msg);
                    CYNG_LOG_DEBUG(logger_, "server: " << s);
                    CYNG_LOG_DEBUG(logger_, "path  : " << p);
                    CYNG_LOG_DEBUG(logger_, "code  : " << code << " - " << obis::get_name(code));
                    CYNG_LOG_DEBUG(logger_, "attr  : " << a);
                    CYNG_LOG_DEBUG(logger_, "list  : \n" << sml::dump_child_list(l, false));
                    get_proc_parameter_response(s, p, code, a, l);
                }
                break;
            case sml::msg_type::GET_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
                // get_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::CLOSE_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << ", " << sml::get_name(type) << ": " << trx << ", " << msg);
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
        , meters_()
        , access_()
        , cfg_()
        , cache_()
        , throughput_{} {}

    proxy::~proxy() {
#ifdef _DEBUG_IPT
        std::cout << "proxy(~)" << std::endl;
#endif
    }

    bool proxy::is_on() const { return state_ != state::OFF; }

    void proxy::cfg_backup(std::string name, std::string pwd, cyng::buffer_t id) {

        CYNG_LOG_TRACE(logger_, "[proxy] config backup " << name << '@' << id);
        BOOST_ASSERT_MSG(state_ == state::OFF, "proxy already active");
        BOOST_ASSERT_MSG(!id.empty(), "no gateway id");
        // state_ = state::ROOT_CFG;

        //
        //  update connection state - QRY_BASIC
        //
        update_connect_state(true);

        meters_ = {}; //  clear
        access_ = {}; //  clear
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

        req_gen_.reset(name, pwd, 22);
        id_.assign(id.begin(), id.end());

        //
        //  query some basic configurations and all active devices
        //
        session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
            sml::messages_t messages;
            messages.emplace_back(req_gen_.public_open(client_id_, id_));
            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_LAN_DSL));
            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_PARAM)); //  OK
            // messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_STATE)); this is not configuration data
            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_NTP));

            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_DEVICE_IDENT));   //  OK
            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACTIVE_DEVICES)); //  OK

            //
            //  access rights need to know the active devices, so this comes first
            //
            // temporary removed: messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACCESS_RIGHTS));
            // temporary removed: messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_WAN_PARAM)); //  cannot read
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
            throughput_ += msg.size();
            auto const open_res = req_gen_.get_trx_mgr().store_pefix();
            CYNG_LOG_TRACE(logger_, "prefix #" << open_res << ": " << req_gen_.get_trx_mgr().get_prefix());
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
        cyng::attr_t attr,
        cyng::tuple_t const &tpl) {

        if (code == OBIS_ROOT_NTP) {
            //
            //  send to configuration storage
            //
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &op, cyng::attr_t const &attr) {
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

        } else if (
            code == OBIS_ROOT_LAN_DSL || code == OBIS_ROOT_DEVICE_IDENT || code == OBIS_ROOT_WAN_PARAM ||
            code == OBIS_ROOT_IPT_PARAM || code == OBIS_ROOT_DATA_COLLECTOR || code == OBIS_ROOT_PUSH_OPERATIONS ||
            code == OBIS_ROOT_SENSOR_PARAMS) {
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, obis::get_name(code) << " [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        obis::get_name(code) << " [" << srv << "]: "
                                             << " has no entry");
                }
            });
        } else if (code.starts_with({0x81, 0x81, 0x81, 0x64}) && !access_.empty()) {
            auto const path = access_.front();
            access_.pop();

            //  attr contains the affected device
            auto const id = cyng::value_cast(attr.second, srv);
            CYNG_LOG_TRACE(logger_, "user access rights for: " << id << " - " << path);
            sml::collect(tpl, path, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(id);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, obis::get_name(code) << " [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        code << " [" << srv << "]: "
                             << " has no entry");
                }
            });

        } else if (code == OBIS_ROOT_ACCESS_RIGHTS) {
            //
            //  query configuration
            //
            CYNG_LOG_TRACE(logger_, "OBIS_ROOT_ACCESS_RIGHTS \n" << sml::dump_child_list(tpl, false));
            sml::collect(tpl, {code}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
                auto pos = cfg_.find(srv);
                if (pos != cfg_.end()) {
                    CYNG_LOG_TRACE(logger_, "OBIS_ROOT_ACCESS_RIGHTS [" << path << "]: " << attr);
                    pos->second.add(path, attr);
                    if (!path.empty() && path.back().starts_with({0x81, 0x81, 0x81, 0x64})) {
                        access_.push(path);
                        CYNG_LOG_DEBUG(logger_, "store path [" << path << "] #" << access_.size());
                    }
                } else {
                    CYNG_LOG_WARNING(
                        logger_,
                        "OBIS_ROOT_ACCESS_RIGHTS [" << srv << "]: "
                                                    << " has no entry");
                }
            });

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
                    if (id_ != id) {
                        //  meters only
                        meters_.push(id);
                    }
                    cfg_.emplace(id, sml::tree{});
                }
            });
        } else {
            CYNG_LOG_WARNING(logger_, "unknown get proc parameter response: " << obis::get_name(code));
        }
    }

    void proxy::cfg_backup_meter() {
        //  ROOT_SENSOR_PARAMS (81 81 c7 86 00 ff - Datenspiegel)
        //  ROOT_DATA_COLLECTOR (81 81 C7 86 20 FF - Datensammler)
        //  IF_1107 (81 81 C7 93 00 FF - serial interface)
        BOOST_ASSERT_MSG(!id_.empty(), "no gateway id");
        BOOST_ASSERT(!meters_.empty());

        //
        //  update state
        //
        state_ = state::QRY_METER;

        if (!meters_.empty()) {

            session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
                sml::messages_t messages;
                messages.emplace_back(req_gen_.public_open(client_id_, id_));
                auto const id = meters_.front();
                meters_.pop();
                CYNG_LOG_INFO(
                    logger_,
                    "[proxy] backup " << get_state() << " " << id << " - " << (cfg_.size() - meters_.size()) << "/" << cfg_.size());
                messages.emplace_back(req_gen_.get_proc_parameter(id, OBIS_ROOT_SENSOR_PARAMS));
                messages.emplace_back(req_gen_.get_proc_parameter(id, OBIS_ROOT_DATA_COLLECTOR));
                messages.emplace_back(req_gen_.get_proc_parameter(id, OBIS_ROOT_PUSH_OPERATIONS));
                messages.emplace_back(req_gen_.public_close());
                auto msg = sml::to_sml(messages);
                throughput_ += msg.size();
                auto const open_res = req_gen_.get_trx_mgr().store_pefix();
                CYNG_LOG_TRACE(logger_, "prefix #" << open_res << ": " << req_gen_.get_trx_mgr().get_prefix());
                return session_.serializer_.escape_data(std::move(msg));
            });
        }
    }

    void proxy::cfg_backup_access() {
        BOOST_ASSERT_MSG(!id_.empty(), "no gateway id");
        BOOST_ASSERT(state_ == state::QRY_METER);

        //
        //  update state
        //
        state_ = state::QRY_ACCESS;

        session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
            CYNG_LOG_INFO(logger_, "[proxy] backup " << get_state());
            sml::messages_t messages;
            messages.emplace_back(req_gen_.public_open(client_id_, id_));
            messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACCESS_RIGHTS));
            messages.emplace_back(req_gen_.public_close());
            auto msg = sml::to_sml(messages);
            throughput_ += msg.size();
            auto const open_res = req_gen_.get_trx_mgr().store_pefix();
            CYNG_LOG_TRACE(logger_, "prefix #" << open_res << ": " << req_gen_.get_trx_mgr().get_prefix());
            return session_.serializer_.escape_data(std::move(msg));
        });
    }

    void proxy::cfg_backup_user() {

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

            session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
                // CYNG_LOG_INFO(logger_, "[proxy] backup " << get_state());
                BOOST_ASSERT(!id_.empty());
                sml::messages_t messages;
                auto const &path = access_.front();
                CYNG_LOG_INFO(logger_, "[proxy] backup " << get_state() << " " << path << " - " << access_.size());

                messages.emplace_back(req_gen_.public_open(client_id_, id_));
                messages.emplace_back(req_gen_.get_proc_parameter(id_, path));
                // messages.emplace_back(req_gen_.get_proc_parameter_access(id_, 0x03, 0x01, 0x0101));
                //  messages.emplace_back(req_gen_.get_proc_parameter_access(id_, 0x03, 0x01, 0x0102));
                messages.emplace_back(req_gen_.public_close());
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
                throughput_ += msg.size();
                auto const open_res = req_gen_.get_trx_mgr().store_pefix();
                CYNG_LOG_TRACE(logger_, "prefix #" << open_res << ": " << req_gen_.get_trx_mgr().get_prefix());
                return session_.serializer_.escape_data(std::move(msg));
            });
        }
    }

    void proxy::close_response(std::string trx, cyng::tuple_t const msg) {
        auto const [pending, success] = req_gen_.get_trx_mgr().ack_trx(trx);
        if (success) {
            CYNG_LOG_TRACE(logger_, pending << " transactions are open - " << trx << ", proxy state: " << get_state());
        } else {
            CYNG_LOG_WARNING(logger_, pending << " transactions are open - " << trx << ", proxy state: " << get_state());
        }

        switch (state_) {
        case state::QRY_BASIC:
            if (!cfg_.empty()) {
                cfg_backup_meter();
            } else {
                state_ = state::OFF;
                complete();
            }
            break;
        case state::QRY_METER:
            if (meters_.empty()) {
                cfg_backup_access();
            } else {
                cfg_backup_meter();
            }
            break;
        case state::QRY_ACCESS:
            if (access_.empty()) {
                complete();
            } else {
                //
                //  update state
                //
                state_ = state::QRY_USER;
                cfg_backup_user();
            }
            break;
        case state::QRY_USER:
            if (access_.empty()) {
                complete();
            } else {
                cfg_backup_user();
            }
            break;
        default:
            break;
        }
    }

    void proxy::complete() {
        //
        //  update connection state
        //
        update_connect_state(false);

        boost::uuids::random_generator_mt19937 uidgen;
        auto const tag = uidgen();
        std::size_t counter{0};

        CYNG_LOG_INFO(logger_, "readout of " << cfg_.size() << " server/clients complete - tag: " << tag)
        for (auto const &cfg : cfg_) {
            auto const cl = cfg.second.to_child_list();
            CYNG_LOG_TRACE(logger_, "server " << cfg.first << " complete: \n" << sml::dump_child_list(cl, true));

            //
            //  send child list to configuration manager
            //
            counter += backup(tag, cfg.first, cl);
        }
        CYNG_LOG_INFO(logger_, counter << " config records backuped - write meta data");
        cluster_bus_.cfg_finish_backup(tag, id_, std::chrono::system_clock::now());

    }

    std::size_t proxy::backup(boost::uuids::uuid tag, cyng::buffer_t meter, cyng::tuple_t tpl) {
        std::size_t counter{0};
        // cfg_backup(boost::uuids::uuid tag, cyng::buffer_t gw, cyng::buffer_t meter, cyng::obis_path_t, cyng::object value);
        sml::collect(tpl, {}, [&, this](cyng::obis_path_t const &path, cyng::attr_t const &attr) {
            CYNG_LOG_DEBUG(logger_, "send to config manager: " << tag << ", " << meter << ", " << path << ", " << attr);
            cluster_bus_.cfg_merge_backup(tag, id_, meter, path, attr.second);
            ++counter;
        });
        return counter;
    }

    void proxy::update_connect_state(bool connected) {
        //
        //  update session state
        //
        // auto const b1 = tbl_session->modify(key_caller, cyng::make_param("cTag", dev), cfg_.get_tag());
        if (connected) {
            state_ = state::QRY_BASIC;
            session_.cluster_bus_.req_db_update(
                "session", cyng::key_generator(session_.dev_), cyng::param_map_factory()("cTag", session_.dev_));
        } else {
            state_ = state::OFF;
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

    // state{ OFF, QRY_BASIC, QRY_METER, QRY_ACCESS, QRY_USER } state_;
    std::string proxy::get_state() const {
        switch (state_) {
        case state::OFF:
            return "OFF";
        case state::QRY_BASIC:
            return "BASIC";
        case state::QRY_METER:
            return "METER";
        case state::QRY_ACCESS:
            return "ACCESS";
        case state::QRY_USER:
            return "USER";
        default:
            break;
        }
        return "invalid state";
    }

} // namespace smf
