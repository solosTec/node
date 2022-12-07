#include <tasks/emt.h>

#include <storage_functions.h>

#include <smf/config/persistence.h>
#include <smf/mbus/field_definitions.h>
#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/decode.h>
#include <smf/mbus/reader.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/defs.h>
#include <smf/sml/reader.h>
#include <smf/sml/readout.h>
#include <smf/sml/unpack.h>

#include <cyng/db/storage.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/net/client.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/util.hpp>
#include <cyng/sql/sql.hpp>
#include <cyng/task/controller.h>

#ifdef _DEBUG_SEGW
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf {
    emt::emt(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
        , cyng::db::session db
		, cfg& config)
	: sigs_{
            std::bind(&emt::receive, this, std::placeholders::_1),	//	0
            std::bind(&emt::reset_target_channels, this),	//	1 - "reset-data-sinks"
            std::bind(&emt::add_target_channel, this, std::placeholders::_1),	//	2 - "add-data-sink"
            std::bind(&emt::push, this),	//	3 - "push"
            std::bind(&emt::backup, this),	//	4 - "backup"
            std::bind(&emt::stop, this, std::placeholders::_1)		//	4
    }
		, channel_(wp)
		, ctl_(ctl)
        , client_factory_(ctl)
        , proxy_()
		, logger_(logger)
        , db_(db)
        , cfg_(config)
        , cfg_http_post_(config, lmn_type::WIRELESS)
        , parser_([this](mbus::radio::header const& h, mbus::radio::tplayer const& t, cyng::buffer_t const& data) {

            //
            // process wireless data header
            //
            this->decode(h, t, data);
        })
    {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"receive", "reset-data-sinks", "add-data-sink", "push", "backup"});

            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void emt::stop(cyng::eod) {
        proxy_.stop();
        CYNG_LOG_INFO(logger_, "[emt] stopped");
    }

    void emt::receive(cyng::buffer_t data) {
        CYNG_LOG_TRACE(logger_, "[emt] received " << data.size() << " bytes");
        parser_.read(std::begin(data), std::end(data));
    }

    void emt::reset_target_channels() { CYNG_LOG_TRACE(logger_, "[emt] clear target channels"); }

    void emt::add_target_channel(std::string name) { CYNG_LOG_TRACE(logger_, "[emt] add target channel: " << name); }

    void emt::decode(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data) {

        //
        //  update cache
        //
        if (cfg_http_post_.is_enabled()) {
            update_cache(h, tpl, data);
        } else {
            CYNG_LOG_TRACE(logger_, "[emt] is diabled");
        }

        //
        //  update load profile
        //
        // update_load_profile(h, tpl, data);
    }

    void emt::update_cache(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data) {

        auto const srv_id = h.get_server_id();

        //
        //  complete payload with header
        //
        auto payload = mbus::radio::restore_data(h, tpl, data);

        CYNG_LOG_TRACE(logger_, "[emt] update cache of " << to_string(srv_id));

        //
        //  write into cache
        //
        cfg_.get_cache().access(
            [&](cyng::table *tbl) {
                if (tbl->merge(
                        cyng::key_generator(h.get_server_id_as_buffer()),
                        cyng::data_generator(
                            payload,                // payload
                            cyng::make_utc_date()), // received
                        1u,
                        cfg_.get_tag())) {
                    CYNG_LOG_TRACE(logger_, "[httpCache] data of meter " << to_string(srv_id) << " inserted");

                } else {
                    CYNG_LOG_TRACE(logger_, "[httpCache] data of meter " << to_string(srv_id) << " updated");
                }
            },
            cyng::access::write("httpCache"));
    }

    void emt::push() {

        if (auto sp = channel_.lock(); sp) {
            //
            //  restart push
            //
            auto const period = cfg_http_post_.get_interval();
            sp->suspend(period, "push");

            auto const host = cfg_http_post_.get_host();
            auto const service = cfg_http_post_.get_service();

            //
            //  send data
            //
            proxy_ = client_factory_.create_proxy(
                [=, this](std::size_t, boost::system::error_code ec) -> std::pair<std::chrono::seconds, bool> {
                    CYNG_LOG_WARNING(
                        logger_, "[" << sp->get_name() << "] cannot connect to " << host << ':' << service << " - " << ec);

                    //
                    //  write cache to database
                    //
                    sp->dispatch("backup");
                    return {std::chrono::seconds(0), false};
                },
                [=, this](boost::asio::ip::tcp::endpoint ep, cyng::channel_ptr cp) {
                    CYNG_LOG_TRACE(logger_, "[" << sp->get_name() << "]  successfully connected to " << ep);

                    //
                    //  send data
                    //
                    push_data(cp);
                },
                [=, this](cyng::buffer_t data) {
                    //  should not receive anything - send only
                    CYNG_LOG_WARNING(logger_, "[" << sp->get_name() << "] received " << data.size() << " bytes");
                },
                [=, this](boost::system::error_code ec) {
                    //	fill async
                    CYNG_LOG_WARNING(logger_, "[" << sp->get_name() << "] connection lost " << ec.message());
                });

            CYNG_LOG_INFO(logger_, "[" << sp->get_name() << "] open connection to " << host << ":" << service);
            proxy_.connect(host, service);
        }
    }

    void emt::push_data(cyng::channel_ptr cp) {

        //
        //  read data from cache and clear the cache
        //
        std::size_t counter = 0;
        cfg_.get_cache().access(
            [&](cyng::table *tbl) {
                auto const now = cyng::make_utc_date();
                auto const serial = cfg_http_post_.get_serial();

                tbl->loop([=, this, &counter](cyng::record &&rec, std::size_t) -> bool {
                    ++counter;

                    //
                    //  build header
                    //
                    auto const received = rec.value("received", now);
                    auto const ts = cyng::as_string(received, "%Y-%M-%D %h:%m:%s");
                    auto const header = cyng::param_map_factory("Content-Type", "text/plain") // content type
                                        ("Accept", "*/*")                                     //
                                        ("X-timestamp", ts)                                   //
                                        ("X-cls-adapter-id", serial)                          //
                                            .
                                            operator cyng::param_map_t();

                    //
                    //  build body
                    //
                    auto const id = rec.value("meterID", cyng::make_buffer());
                    auto const payload = rec.value("payload", cyng::make_buffer());

                    std::stringstream ss;
                    ss << serial << ';' << cyng::to_string(id) << ';' << ts << ';' << "00000000" << ';' << "0" << ';' << "00" << ';'
                       << cyng::to_string(payload);

                    auto const body = ss.str();
                    CYNG_LOG_DEBUG(logger_, "[" << cp->get_name() << "] push " << body);
                    CYNG_LOG_TRACE(logger_, "[" << cp->get_name() << "] send data of meter " << id);
                    cp->dispatch("post", "/", cfg_http_post_.get_server(), header, body);
                    return true;
                });
                //
                //  remove all data from cache
                //
                tbl->clear(cfg_.get_tag());
            },
            cyng::access::write("httpCache"));

        //
        // Remove records from database
        // "DELETE FROM THttpCache;"
        //
        auto const m = get_table_http_cache();
        config::persistent_clear(m, db_);

        CYNG_LOG_INFO(logger_, "[" << cp->get_name() << "] " << counter << " records");
    }

    void emt::backup() {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db_);

        //
        //  remove old data
        //
        auto const m = get_table_http_cache();
        config::persistent_clear(m, db_);

        //
        //  transfer data from cache to database
        //
        cfg_.get_cache().access(
            [&](cyng::table const *tbl) {
                tbl->loop([=, this](cyng::record &&rec, std::size_t) -> bool {
                    auto const id = rec.value("meterID", cyng::make_buffer());

                    CYNG_LOG_TRACE(logger_, "[emt] backup data of meter " << id);
                    config::persistent_insert(m, db_, rec.key(), rec.data(), 0u);
                    return true;
                });
            },
            cyng::access::read("httpCache"));
    }

} // namespace smf
