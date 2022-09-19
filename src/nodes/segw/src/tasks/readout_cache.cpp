#include <tasks/readout_cache.h>

#include <smf/mbus/field_definitions.h>
#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/decode.h>
#include <smf/mbus/reader.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/defs.h>
#include <smf/sml/reader.h>
#include <smf/sml/readout.h>
//#include <smf/sml/unpack.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf {
    readout_cache::readout_cache(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, cfg& config)
	: sigs_{
            std::bind(&readout_cache::receive, this, std::placeholders::_1),	//	0
            std::bind(&readout_cache::reset_target_channels, this),	//	1 - "reset-data-sinks"
            std::bind(&readout_cache::add_target_channel, this, std::placeholders::_1),	//	2 - "add-data-sink"
            std::bind(&readout_cache::stop, this, std::placeholders::_1)		//	4
    }
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
        , cfg_(config)
        , cfg_sml_(config)
        , parser_([this](mbus::radio::header const& h, mbus::radio::tplayer const& t, cyng::buffer_t const& data) {

            this->decode(h, t, data);
            //   auto const res = smf::mbus::radio::decode(h.get_server_id(), t.get_access_no(), aes, payload);
        })
    {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"receive", "reset-data-sinks", "add-data-sink"});

            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void readout_cache::stop(cyng::eod) {
        CYNG_LOG_INFO(logger_, "[roCache] stopped");
        boost::system::error_code ec;
    }

    void readout_cache::receive(cyng::buffer_t data) {
        CYNG_LOG_TRACE(logger_, "[roCache] received " << data.size() << " bytes");
        parser_.read(std::begin(data), std::end(data));
    }

    void readout_cache::reset_target_channels() {
        // targets_.clear();
        CYNG_LOG_TRACE(logger_, "[roCache] clear target channels");
    }

    void readout_cache::add_target_channel(std::string name) {
        // targets_.insert(name);
        CYNG_LOG_TRACE(logger_, "[roCache] add target channel: " << name);
    }

    void readout_cache::decode(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data) {

        //
        //  update cache
        //
        cfg_.get_cache().access(
            [&](cyng::table *tbl) {

                auto const address = h.get_server_id_as_buffer();
                auto const srv_id = h.get_server_id();
                auto payload = mbus::radio::restore_data(h, tpl, data);

                if (tbl->merge(
                    cyng::key_generator(address),
                    cyng::data_generator(
                        payload, // payload
                        h.get_frame_type(),
                        get_manufacturer_flag(srv_id), // flag
                        h.get_version(),
                        get_medium(srv_id),
                        std::chrono::system_clock::now()),
                    1u,
                        cfg_.get_tag())) {
                    CYNG_LOG_TRACE(logger_, "[roCache] data of meter " << to_string(srv_id) << " inserted");
                
                } else {
                    CYNG_LOG_TRACE(logger_, "[roCache] data of meter " << to_string(srv_id) << " updated");
                }
            },
            cyng::access::write("mbusCache"));
    }

} // namespace smf
