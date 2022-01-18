#include <proxy.h>
#include <session.h>

#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/sml/generator.h>
#include <smf/sml/reader.h>
#include <smf/sml/select.h>
#include <smf/sml/serializer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>
#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

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
                    //  cyng::obis, cyng::attr_t, cyng::tuple_t
                    auto const [p, code, a, l] = sml::read_get_proc_parameter_response(msg);
                    CYNG_LOG_DEBUG(logger_, "path: " << p);
                    CYNG_LOG_DEBUG(logger_, "code: " << code << " - " << obis::get_name(code));
                    CYNG_LOG_DEBUG(logger_, "attr: " << a);
                    CYNG_LOG_DEBUG(logger_, "list: " << l);
                    get_proc_parameter_response(p, code, a, l);
                }
                break;
            case sml::msg_type::GET_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << " " << sml::get_name(type) << ": " << trx << ", " << msg);
                // get_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::CLOSE_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] " << sml::get_name(type) << ": " << trx << ", " << msg);
                // close_response(trx, msg);
                state_ = state::OFF;
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
        , id_() {}

    proxy::~proxy() {
#ifdef _DEBUG_IPT
        std::cout << "proxy(~)" << std::endl;
#endif
    }

    bool proxy::is_on() const { return state_ != state::OFF; }

    void proxy::cfg_backup(std::string name, std::string pwd, cyng::buffer_t id) {

        CYNG_LOG_TRACE(logger_, "[proxy] config backup " << name << '@' << id);
        BOOST_ASSERT_MSG(state_ != state::ON, "proxy already active");
        state_ = state::ON;

        //
        // * login to gateway
        // * send SML requests to get current configuration
        // * parse SML responses
        // * update "connection" table
        // * update some progress information
        // * write SML data into config table (setup node)
        // * ready
        //

        sml::request_generator req_gen(name, pwd);

        //
        //  This works!
        //
        // iptsp_->debug_get_profile_list();

        //
        //  This works too!
        //
        //        iptsp_->ipt_send([=, this]() -> cyng::buffer_t {
        //            std::deque<cyng::buffer_t> deq;
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.public_open(client_id, id_))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_DEVICE_IDENT))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_STATE))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_PARAM))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACCESS_RIGHTS))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_ACTIVE_DEVICES))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_WAN_PARAM))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_NTP))));
        //            deq.push_back(sml::set_crc16(sml::serialize(req_gen_.public_close())));
        //            auto msg = sml::boxing(ipt::merge(deq));
        //
        //#ifdef _DEBUG
        //            {
        //                std::stringstream ss;
        //                cyng::io::hex_dump<8> hd;
        //                hd(ss, msg.begin(), msg.end());
        //                auto const dmp = ss.str();
        //                CYNG_LOG_TRACE(logger_, "[OBIS_ROOT_LAN_DSL, OBIS_ROOT_IPT_PARAM] :\n" << dmp);
        //            }
        //#endif
        //            // return iptsp_->serializer_.scramble(std::move(msg));
        //            return iptsp_->serializer_.escape_data(std::move(msg));
        //        });

        session_.ipt_send([=, this]() mutable -> cyng::buffer_t {
            sml::messages_t messages;
            messages.emplace_back(req_gen.public_open(client_id_, id_));
            // messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_LAN_DSL));
            // messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_PARAM));

            messages.emplace_back(req_gen.get_proc_parameter(id_, OBIS_ROOT_DEVICE_IDENT));
            messages.emplace_back(req_gen.get_proc_parameter(id_, OBIS_ROOT_IPT_PARAM));
            // messages.emplace_back(req_gen_.get_proc_parameter(id_, OBIS_ROOT_IPT_STATE));
            messages.emplace_back(req_gen.get_proc_parameter(id_, OBIS_ROOT_ACTIVE_DEVICES));
            //
            //  access rights need to know the active devices, so this comes first
            //
            messages.emplace_back(req_gen.get_proc_parameter(id_, OBIS_ROOT_ACCESS_RIGHTS));
            messages.emplace_back(req_gen.get_proc_parameter(id_, OBIS_ROOT_WAN_PARAM));
            messages.emplace_back(req_gen.get_proc_parameter(id_, OBIS_ROOT_NTP));

            messages.emplace_back(req_gen.public_close());
            auto msg = sml::to_sml(messages);
#ifdef _DEBUG
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, msg.begin(), msg.end());
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(logger_, "[OBIS_ROOT_LAN_DSL] :\n" << dmp);
            }
#endif
            // return iptsp_->serializer_.scramble(std::move(msg));
            return session_.serializer_.escape_data(std::move(msg));
        });

        //  login
        /*
        iptsp_->ipt_send([=, this]() -> cyng::buffer_t {
            auto msg = sml::set_crc16(sml::serialize(req_gen_.public_open(client_id, id_)));

#ifdef _DEBUG
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, msg.begin(), msg.end());
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(logger_, "[public login] :\n" << dmp);
            }
#endif
            //  only scrambling (no escaping)
            return iptsp_->serializer_.escape_data(std::move(msg));
        });

        //  get_proc_parameter
        iptsp_->ipt_send([=, this]() -> cyng::buffer_t {
            auto msg = sml::set_crc16(sml::serialize(req_gen_.get_proc_parameter(id_, OBIS_ROOT_LAN_DSL)));

#ifdef _DEBUG
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, msg.begin(), msg.end());
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(logger_, "[get proc parameter] :\n" << dmp);
            }
#endif
            //  only scrambling (no escaping)
            return iptsp_->serializer_.escape_data(std::move(msg));
        });

        //  logout
        iptsp_->ipt_send([=, this]() -> cyng::buffer_t {
            auto msg = sml::set_crc16(sml::serialize(req_gen_.public_close()));

#ifdef _DEBUG
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, msg.begin(), msg.end());
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(logger_, "[public close] :\n" << dmp);
            }
#endif
            //  only scrambling (no escaping)
            return iptsp_->serializer_.escape_data(std::move(msg));
        });
        */
    }

    // void proxy::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[proxy] stop"); }
    void proxy::read(cyng::buffer_t &&data) { parser_.read(std::begin(data), std::end(data)); }

    void
    proxy::get_proc_parameter_response(cyng::obis_path_t const &path, cyng::obis code, cyng::attr_t, cyng::tuple_t const &tpl) {
        if (code == OBIS_ROOT_NTP) {
            //
            //  send to configuration storage
            //

        } else if (code == OBIS_ROOT_DEVICE_IDENT) {

        } else if (code == OBIS_ROOT_IPT_PARAM) {
            //
            //  send to configuration storage
            //

        } else if (code == OBIS_ROOT_ACCESS_RIGHTS) {
            //
            //  query configuration
            //

        } else if (code == OBIS_ROOT_ACTIVE_DEVICES) {
            //
            //  query device configuration
            //
            sml::select_devices(tpl);

        } else {
            CYNG_LOG_WARNING(logger_, "unknown get proc parameter response :" << obis::get_name(code));
        }
    }

} // namespace smf
