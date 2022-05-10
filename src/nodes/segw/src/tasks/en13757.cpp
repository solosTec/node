#include <tasks/en13757.h>

#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/decode.h>
#include <smf/mbus/server_id.h>

#include <cyng/log/record.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf {
    en13757::en13757(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, cfg& config)
	: sigs_{
            std::bind(&en13757::receive, this, std::placeholders::_1),	//	0
            std::bind(&en13757::reset_target_channels, this),	//	1 - "reset-data-sinks"
            std::bind(&en13757::add_target_channel, this, std::placeholders::_1),	//	2 - "add-data-sink"
            std::bind(&en13757::stop, this, std::placeholders::_1)		//	4
    }
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
        , cfg_(config)
        , parser_([this](mbus::radio::header const& h, mbus::radio::tpl const& t, cyng::buffer_t const& data) {

            this->decode(h, t, data);
            //   auto const res = smf::mbus::radio::decode(h.get_server_id(), t.get_access_no(), aes, payload);
        })
    {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"receive", "reset-data-sinks", "add-data-sink"});

            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void en13757::stop(cyng::eod) {
        CYNG_LOG_INFO(logger_, "[EN-13757] stopped");
        boost::system::error_code ec;
    }

    void en13757::receive(cyng::buffer_t data) {
        CYNG_LOG_TRACE(logger_, "[EN-13757] received " << data.size() << " bytes");
        parser_.read(std::begin(data), std::end(data));
    }

    void en13757::reset_target_channels() {
        // targets_.clear();
        CYNG_LOG_TRACE(logger_, "[EN-13757] clear target channels");
    }

    void en13757::add_target_channel(std::string name) {
        // targets_.insert(name);
        CYNG_LOG_TRACE(logger_, "[EN-13757] add target channel: " << name);
    }

    // void en13757::update_statistics() { CYNG_LOG_TRACE(logger_, "[EN-13757] update statistics"); }

    void en13757::decode(mbus::radio::header const &h, mbus::radio::tpl const &t, cyng::buffer_t const &data) {
        auto const flag_id = get_manufacturer_code(h.get_server_id());
        CYNG_LOG_TRACE(
            logger_,
            "[EN-13757] read meter: " << get_id(h.get_server_id()) << " (" << mbus::decode(flag_id.first, flag_id.second) << ")");

        //
        //  get the AES key from table "TMeterMBus"
        //
        cfg_.get_cache().access(
            [&](cyng::table *tbl) {
                cyng::crypto::aes_128_key aes_key;
                auto const now = std::chrono::system_clock::now();
                auto const key = cyng::key_generator(h.get_server_id_as_buffer());
                auto const rec = tbl->lookup(key);
                if (rec.empty()) {
                    //
                    //  wireless M-Bus meter not found - insert
                    //
                    tbl->insert(
                        key,
                        cyng::data_generator(now, "---", true, 0u, cyng::make_buffer({0, 0}), cyng::make_buffer({0}), aes_key),
                        1,
                        cfg_.get_tag());
                } else {
                    //
                    //  get AES key
                    //
                    aes_key = rec.value("aes", aes_key);

                    //
                    //  update
                    //
                    tbl->modify(rec.key(), cyng::make_param("lastSeen", now), cfg_.get_tag());
                }
            },
            cyng::access::write("meterMBus"));
    }

} // namespace smf
