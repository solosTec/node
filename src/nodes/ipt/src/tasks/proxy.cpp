#include <session.h>
#include <tasks/proxy.h>

#include <smf/obis/defs.h>
#include <smf/sml/serializer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>
#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

#include <iostream>

namespace smf {

    proxy::proxy(cyng::channel_weak wp
		, cyng::logger logger
		, std::shared_ptr<ipt_session> iptsp
        , bus &cluster_bus
        , std::string name
        , std::string pwd
        , cyng::buffer_t id)
    : state_(state::INITIAL)
	, sigs_{ 
        std::bind(&proxy::cfg_backup, this, std::placeholders::_1),
        std::bind(&proxy::stop, this, std::placeholders::_1),
	}
	, channel_(wp)
	, logger_(logger)
	, iptsp_(iptsp)
    , cluster_bus_(cluster_bus)
    , parser_([this](
        std::string trx,
        std::uint8_t group_no,
        std::uint8_t,
        smf::sml::msg_type type,
        cyng::tuple_t msg,
        std::uint16_t crc) {
            switch (type) {
                //open_response(trx, msg);
                break;
            case sml::msg_type::GET_PROFILE_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                //get_profile_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::GET_PROC_PARAMETER_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                //get_proc_parameter_response(trx, group_no, msg);
                break;
            case sml::msg_type::GET_LIST_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] #" << +group_no << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                //get_list_response(trx, group_no, msg);
                break;
            case sml::msg_type::CLOSE_RESPONSE:
                CYNG_LOG_TRACE(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                //close_response(trx, msg);
                break;
            default:
                CYNG_LOG_WARNING(logger_, "[sml] " << smf::sml::get_name(type) << ": " << trx << ", " << msg);
                break;
            }
     })
     , id_(id)
#ifdef _DEBUG
     , req_gen_("operator", "operator")
#else
     , req_gen_(name, pwd)
#endif
     {
        BOOST_ASSERT(iptsp_);
        auto sp = wp.lock();
        if (sp) {
            sp->set_channel_names({"cfg.backup"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    proxy::~proxy() {
#ifdef _DEBUG_IPT
        std::cout << "proxy(~)" << std::endl;
#endif
    }

    void proxy::cfg_backup(cyng::mac48 client_id) {
        CYNG_LOG_TRACE(logger_, "[proxy] config backup " << req_gen_.get_name() << '@' << id_);

        //
        // * login to gateway
        // * send SML requests to get current configuration
        // * parse SML responses
        // * update "connection" table
        // * update some progress information
        // * write SML data into config table (setup node)
        // * ready
        //

        //  login
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
            return iptsp_->serializer_.scramble(std::move(msg));
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
            return iptsp_->serializer_.scramble(std::move(msg));
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
            return iptsp_->serializer_.scramble(std::move(msg));
        });
    }

    void proxy::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[proxy] stop"); }

} // namespace smf
