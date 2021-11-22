#include <http_server.h>

#include <smf/http/session.h>
#include <smf/http/ws.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/parse/json/json_parser.h>
#include <cyng/rnd/rnd.hpp>
#include <cyng/sys/cpu.h>
#include <cyng/sys/memory.h>
#include <cyng/task/channel.h>

#include <smfsec/hash/base64.h>

#include <boost/predef.h>
#include <boost/uuid/uuid_io.hpp>

namespace smf {

    http_server::http_server(
        boost::asio::io_context &ioc,
        bus &cluster_bus,
        cyng::logger logger,
        std::string const &document_root,
        db &data,
        blocklist_type &&blocklist,
        std::map<std::string, std::string> &&redirects_intrinsic,
        http::auth_dirs const &auths)
        : cluster_bus_(cluster_bus)
        , logger_(logger)
        , document_root_(document_root)
        , db_(data)
        , blocklist_(blocklist.begin(), blocklist.end())
        , redirects_intrinsic_(redirects_intrinsic.begin(), redirects_intrinsic.end())
        , auths_(auths)
        , server_(ioc, logger, std::bind(&http_server::accept, this, std::placeholders::_1))
        , upload_(logger, cluster_bus_, db_)
        , download_(logger, cluster_bus_, db_)
        , uidgen_()
        , ws_map_() {
        CYNG_LOG_INFO(logger_, "[HTTP] server [" << document_root_ << "] started");
    }

    http_server::~http_server() {
#ifdef _DEBUG_DASH
        std::cout << "http_server(~)" << std::endl;
#endif
    }

    void http_server::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "stop http server task(" << document_root_ << ")");
        server_.stop();
    }

    void http_server::listen(boost::asio::ip::tcp::endpoint ep) {

        CYNG_LOG_INFO(logger_, "start listening at(" << ep << ")");
        server_.start(ep);
    }

    void http_server::accept(boost::asio::ip::tcp::socket &&s) {

        CYNG_LOG_INFO(logger_, "accept (" << s.remote_endpoint() << ")");

        //
        //	check blocklist
        //
        auto const pos = blocklist_.find(s.remote_endpoint().address());
        if (pos != blocklist_.end()) {

            CYNG_LOG_WARNING(logger_, "address [" << s.remote_endpoint() << "] is blocked");
            s.close();

            return;
        }

        std::make_shared<http::session>(
            std::move(s),
            logger_,
            document_root_,
            redirects_intrinsic_,
            auths_,
            db_.cfg_.get_value("http-max-upload-size", static_cast<std::uint64_t>(0xA00000)),
            db_.cfg_.get_value("http-server-nickname", "p@ladin"),
            db_.cfg_.get_value("http-session-timeout", std::chrono::seconds(30)),
            std::bind(&http_server::upgrade, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&http_server::post, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
            ->run();
    }

    void http_server::upgrade(boost::asio::ip::tcp::socket s, boost::beast::http::request<boost::beast::http::string_body> req) {

        /*boost::asio::make_strand(server_.ioc_);*/
        auto const tag = uidgen_();
        CYNG_LOG_INFO(logger_, "new ws " << tag << '@' << s.remote_endpoint() << " #" << ws_map_.size() + 1);
        ws_map_
            .emplace(
                tag,
                std::make_shared<http::ws>(
                    std::move(s), server_.ioc_, logger_, std::bind(&http_server::on_msg, this, tag, std::placeholders::_1)))
            .first->second.lock()
            ->do_accept(std::move(req));
    }

    std::filesystem::path http_server::post(std::string target, std::string content, std::string type) {

        //
        //  parse content
        //  content: {"type":"iec","fmt":"CSV"}
        //
        std::filesystem::path download;
        if (boost::algorithm::starts_with(type, "application/json")) {
            CYNG_LOG_INFO(logger_, "post: " << target << ", content_type: " << type << ", content: " << content);
            cyng::json::parser parser([this, target, &download](cyng::object &&obj) {
                CYNG_LOG_TRACE(logger_, obj);
                //	example: %(("fmt":JSON),("type":dev),("version":v50))
                auto const reader = cyng::make_reader(obj);
                auto const type = cyng::value_cast(reader.get("type"), "");
                auto const fmt = cyng::value_cast(reader.get("fmt"), "");
                if (boost::algorithm::equals(type, "dev")) {
                    //	/download.devices
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.devices"));
                    // download = "C:\\Users\\Sylko\\Documents\\teko-2021\\Physics\\chapter-05.pdf";
                    download = download_.generate("device", fmt);
                } else if (boost::algorithm::equals(type, "meter")) {
                    //	/download.meters
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.meters"));
                    download = download_.generate("meter", fmt);
                } else if (boost::algorithm::equals(type, "gw")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.gateways"));
                    download = download_.generate("gateway", fmt);
                } else if (boost::algorithm::equals(type, "msg")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.messages"));
                    download = download_.generate("sysMsg", fmt);
                } else if (boost::algorithm::equals(type, "iec")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.iec"));
                    download = download_.generate("meterIEC", fmt);
                } else if (boost::algorithm::equals(type, "wMBus")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.wMBus"));
                    download = download_.generate("meterwMBus", fmt);
                } else if (boost::algorithm::equals(type, "uplinkIec")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.IECUplink"));
                    download = download_.generate("iecUplink", fmt);
                } else if (boost::algorithm::equals(type, "uplinkwMBus")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.wMBusUplink"));
                    download = download_.generate("wMBusUplink", fmt);
                } else if (boost::algorithm::equals(type, "gwIEC")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.gwIEC"));
                    download = download_.generate("gwIEC", fmt);
                } else if (boost::algorithm::equals(type, "gwwMBus")) {
                    BOOST_ASSERT(boost::algorithm::equals(target, "/download.gwwMBus"));
                    download = download_.generate("gwwMBus", fmt);
                } else {
                    CYNG_LOG_WARNING(logger_, "[HTTP] unknown download type: [" << type << "]");
                }
            });
            parser.read(content.begin(), content.end());

        } else if (boost::algorithm::starts_with(type, "application/xml")) {
            CYNG_LOG_INFO(logger_, "post: " << target << ", content_type: " << type << ", content: " << content);

        } else {
            CYNG_LOG_WARNING(logger_, "post: " << target << ", content_type: [" << type << "] not supported, content: " << content);
        }
        return download;
    }

    void http_server::on_msg(boost::uuids::uuid tag, std::string msg) {

        //
        //	JSON parser
        //
        CYNG_LOG_TRACE(logger_, "ws (" << tag << ") received " << msg);
        cyng::json::parser parser([this, tag](cyng::object &&obj) {
            CYNG_LOG_TRACE(logger_, "[HTTP] ws parsed " << obj);
            auto const reader = cyng::make_reader(std::move(obj));
            auto const cmd = cyng::value_cast(reader["cmd"].get(), "uups");

            auto pos = ws_map_.find(tag);
            if (pos != ws_map_.end() && !pos->second.expired()) {

                if (boost::algorithm::equals(cmd, "exit")) {
                    auto const reason = cyng::value_cast(reader["reason"].get(), "uups");
                    CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] closed: " << reason);

                    //
                    //	remove from subscription list
                    //
                    auto const count = db_.remove_from_subscription(tag);
                    CYNG_LOG_INFO(logger_, "[HTTP] ws [" << tag << "] removed from #" << count << " subscriptions");

                    //
                    //	remove from ws list
                    //
                    ws_map_.erase(pos);

                    //
                    //	iterator invalid!
                    //
                } else if (boost::algorithm::equals(cmd, "subscribe")) {
                    auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
                    CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] subscribes channel " << channel);

                    //
                    //	send response
                    //
                    BOOST_ASSERT(pos->second.lock());
                    if (response_subscribe_channel(pos->second.lock(), channel, tag)) {

                        //
                        //	add to subscription list
                        //
                        db_.add_to_subscriptions(channel, tag);
                    }

                } else if (boost::algorithm::equals(cmd, "update")) {
                    //	{"cmd":"update","channel":"sys.cpu.usage.total"}
                    auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
                    CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] update channel " << channel);
                    response_update_channel(pos->second.lock(), channel);

                } else if (boost::algorithm::equals(cmd, "modify")) {
                    //	%(("channel":config.device),("cmd":modify),("rec":%(("data":%(("enabled":false))),("key":[....]))))
                    auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
                    CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] modify request from channel " << channel);

                    modify_request(
                        channel,
                        cyng::container_cast<cyng::vector_t>(reader["rec"]["key"].get()),
                        cyng::container_cast<cyng::param_map_t>(reader["rec"]["data"].get()));

                } else if (boost::algorithm::equals(cmd, "insert")) {
                    auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
                    CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] insert request from channel " << channel);
                    insert_request(
                        channel,
                        cyng::container_cast<cyng::vector_t>(reader["rec"]["key"].get()),
                        cyng::container_cast<cyng::param_map_t>(reader["rec"]["data"].get()));

                } else if (boost::algorithm::equals(cmd, "delete")) {
                    //	%(("channel":config.wmbus),("cmd":delete),("key":%(("tag":[e6e62eae-2a80-4185-8e72-5505d4dfe74b]))))
                    auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
                    CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] delete request from channel " << channel);
                    delete_request(channel, cyng::container_cast<cyng::vector_t>(reader["key"]["tag"].get()));
                } else {
                    CYNG_LOG_WARNING(logger_, "[HTTP] unknown ws command " << cmd);
                }
            } else {

                CYNG_LOG_WARNING(logger_, "[HTTP] ws [" << tag << "] is not registered");
            }
        });

        parser.read(msg.begin(), msg.end());
    }

    void http_server::insert_request(std::string const &channel, cyng::vector_t &&key, cyng::param_map_t &&data) {
        BOOST_ASSERT_MSG(!data.empty(), "no modify data");
        auto const rel = db_.by_channel(channel);
        if (!rel.empty()) {

            BOOST_ASSERT(boost::algorithm::equals(channel, rel.channel_));

            //
            //	tidy
            //	remove all empty records and records that starts with an underline '_'
            // and with the name "tag"
            //
            tidy(data);

            //
            //	produce a new key
            //
            key = db_.generate_new_key(rel.table_, std::move(key), data);

            //
            //	convert data types
            //
            db_.convert(rel.table_, key, data);

            CYNG_LOG_TRACE(logger_, "[HTTP] insert request for table " << rel.table_ << ": " << data);
            cluster_bus_.req_db_insert(rel.table_, key, db_.complete(rel.table_, std::move(data)), 0);

        } else if (boost::algorithm::equals(channel, "config.upload.bridge")) {
            //	"rec":{"data":{"fileName"
            //		"fileContent"
            //	"policy":"append"
            auto const reader = cyng::make_reader(data);
            auto const name = cyng::value_cast(reader["fileName"].get(), "no-file");
            auto const policy = cyng::value_cast(reader["policy"].get(), "merge");
            auto const content = cyng::crypto::base64_decode(cyng::value_cast(reader["fileContent"].get(), ""));
            CYNG_LOG_INFO(logger_, "[HTTP] upload (" << policy << ") [" << name << "] " << content.size() << " bytes");
            upload_.config_bridge(name, to_upload_policy(policy), content, ',');
        } else {
            CYNG_LOG_WARNING(logger_, "[HTTP] insert: undefined channel " << channel);
        }
    }

    void http_server::delete_request(std::string const &channel, cyng::vector_t &&key) {

        BOOST_ASSERT_MSG(!key.empty(), "no modify key");
        auto const rel = db_.by_channel(channel);
        if (!rel.empty()) {

            BOOST_ASSERT(boost::algorithm::equals(channel, rel.channel_));

            //
            //	convert key data type(s)
            //
            db_.convert(rel.table_, key);

            CYNG_LOG_TRACE(logger_, "[HTTP] delete request for table " << rel.table_ << ": " << key);
            cluster_bus_.req_db_remove(rel.table_, key);
        } else {
            CYNG_LOG_WARNING(logger_, "[HTTP] delete: undefined channel " << channel);
        }
    }

    void http_server::modify_request(std::string const &channel, cyng::vector_t &&key, cyng::param_map_t &&data) {

        BOOST_ASSERT_MSG(!key.empty(), "no modify key");
        BOOST_ASSERT_MSG(!data.empty(), "no modify data");
        auto const rel = db_.by_channel(channel);
        if (!rel.empty() && !key.empty()) {

            BOOST_ASSERT(boost::algorithm::equals(channel, rel.channel_));

            //
            //	tidy
            //	remove all empty records and records that starts with an underline '_'
            // and with the name "tag"
            //
            tidy(data);

            //
            //	convert data types
            //
            db_.convert(rel.table_, key, data);

            CYNG_LOG_TRACE(logger_, "[HTTP] modify request for table " << rel.table_ << ": " << data);
            cluster_bus_.req_db_update(rel.table_, key, data);
        } else {
            CYNG_LOG_WARNING(logger_, "[HTTP] modify: undefined channel or empty key: " << channel);
        }
    }

    bool http_server::response_subscribe_channel(ws_sptr wsp, std::string const &name, boost::uuids::uuid tag) {

        auto const rel = db_.by_channel(name);
        if (!rel.empty()) {

            BOOST_ASSERT(boost::algorithm::equals(name, rel.channel_));

            //
            //	some tables require additional data
            //
            if (boost::algorithm::equals(rel.table_, "meterwMBus")) {
                return response_subscribe_channel_meterwMBus(wsp, name, rel.table_);
            } else if (boost::algorithm::equals(rel.table_, "meterIEC")) {
                return response_subscribe_channel_meterIEC(wsp, name, rel.table_);
            } else if (boost::algorithm::equals(rel.table_, "gwIEC")) {
                return response_subscribe_channel_gwIEC(wsp, name, rel.table_);
            } else if (boost::algorithm::equals(rel.table_, "gateway")) {
                return response_subscribe_channel_gateway(wsp, name, rel.table_);
            } else {
                return response_subscribe_channel_def(wsp, name, rel.table_);
            }
        } else {

            auto const rel = db_.by_counter(name);
            if (!rel.empty()) {

                //
                //	add subscription table
                //
                db_.add_to_subscriptions(rel.counter_, tag);

                //
                //	Send table size
                //
                auto const str = json_update_channel(name, db_.cache_.size(rel.table_));
                wsp->push_msg(str);
                return true; //	valid channel
            } else {
                CYNG_LOG_WARNING(logger_, "[HTTP] subscribe undefined channel " << name);
            }
        }

        return false;
    }

    bool http_server::response_subscribe_channel_def(ws_sptr wsp, std::string const &name, std::string const &table_name) {
        //
        //	Send initial data set
        //
        db_.cache_.access(
            [&](cyng::table const *tbl) -> void {
                //
                //	inform client that data upload is starting
                //
                wsp->push_msg(json_load_icon(name, true));

                //
                //	get total record size
                //
                auto total_size{tbl->size()};
                std::size_t percent{0}, idx{0};

                tbl->loop([&](cyng::record &&rec, std::size_t idx) -> bool {
                    auto const str = json_insert_record(name, rec.to_tuple());
                    CYNG_LOG_DEBUG(logger_, str);
                    wsp->push_msg(str);

                    //
                    //	update current index and percentage calculation
                    //
                    ++idx;
                    const auto prev_percent = percent;
                    percent = (100u * idx) / total_size;
                    if (prev_percent != percent) {
                        wsp->push_msg(json_load_level(name, percent));
                    }

                    return true;
                });

                //
                //	inform client that data upload is finished
                //
                if (percent != 100)
                    wsp->push_msg(json_load_level(name, 100));
                wsp->push_msg(json_load_icon(name, false));
            },
            cyng::access::read(table_name));

        return true; //	valid channel
    }

    bool http_server::response_subscribe_channel_meterwMBus(ws_sptr wsp, std::string const &name, std::string const &table_name) {
        //
        //	Send initial data set
        //
        db_.cache_.access(
            [&](cyng::table const *tbl_wmbus, cyng::table const *tbl_meter) -> void {
                //
                //	inform client that data upload is starting
                //
                wsp->push_msg(json_load_icon(name, true));

                //
                //	get total record size
                //
                auto total_size{tbl_wmbus->size()};
                std::size_t percent{0}, idx{0};

                tbl_wmbus->loop([&](cyng::record &&rec, std::size_t idx) -> bool {
                    //
                    // get additional data from meter
                    //
                    auto const rec_meter = tbl_meter->lookup(rec.key());
                    if (!rec_meter.empty()) {
                        auto const meter_name = rec_meter.value("meter", "");
                        auto tpl = rec.to_tuple(cyng::param_map_factory("meter", meter_name)("online", 0));
                        auto const str = json_insert_record(name, std::move(tpl));
                        CYNG_LOG_DEBUG(logger_, str);
                        wsp->push_msg(str);
                    } else {
                        CYNG_LOG_WARNING(logger_, "[HTTP] missing meter configuration: " << rec.to_string());
                        auto tpl = rec.to_tuple(cyng::param_map_factory("meter", "no-meter")("online", 0));
                        auto const str = json_insert_record(name, std::move(tpl));
                        CYNG_LOG_DEBUG(logger_, str);
                        wsp->push_msg(str);
                    }

                    //
                    //	update current index and percentage calculation
                    //
                    ++idx;
                    const auto prev_percent = percent;
                    percent = (100u * idx) / total_size;
                    if (prev_percent != percent) {
                        wsp->push_msg(json_load_level(name, percent));
                    }

                    return true;
                });

                //
                //	inform client that data upload is finished
                //
                if (percent != 100)
                    wsp->push_msg(json_load_level(name, 100));
                wsp->push_msg(json_load_icon(name, false));
            },
            cyng::access::read(table_name),
            cyng::access::read("meter"));

        return true;
    }

    bool http_server::response_subscribe_channel_meterIEC(ws_sptr wsp, std::string const &name, std::string const &table_name) {
        //
        //	Send initial data set
        //
        db_.cache_.access(
            [&](cyng::table const *tbl_wmbus, cyng::table const *tbl_meter) -> void {
                //
                //	inform client that data upload is starting
                //
                wsp->push_msg(json_load_icon(name, true));

                //
                //	get total record size
                //
                auto total_size{tbl_wmbus->size()};
                std::size_t percent{0}, idx{0};

                tbl_wmbus->loop([&](cyng::record &&rec, std::size_t idx) -> bool {
                    //
                    // get additional data from meter
                    //
                    auto const rec_meter = tbl_meter->lookup(rec.key());
                    if (!rec_meter.empty()) {
                        auto const meter_name = rec_meter.value("meter", "");
                        auto tpl = rec.to_tuple(cyng::param_map_factory("meter", meter_name)("online", 0));
                        auto const str = json_insert_record(name, std::move(tpl));
                        CYNG_LOG_DEBUG(logger_, str);
                        wsp->push_msg(str);
                    } else {
                        CYNG_LOG_WARNING(logger_, "[HTTP] missing meter configuration: " << rec.to_string());
                        auto tpl = rec.to_tuple(cyng::param_map_factory("meter", "no-meter")("online", 0));
                        auto const str = json_insert_record(name, std::move(tpl));
                        CYNG_LOG_DEBUG(logger_, str);
                        wsp->push_msg(str);
                    }

                    //
                    //	update current index and percentage calculation
                    //
                    ++idx;
                    const auto prev_percent = percent;
                    percent = (100u * idx) / total_size;
                    if (prev_percent != percent) {
                        wsp->push_msg(json_load_level(name, percent));
                    }

                    return true;
                });

                //
                //	inform client that data upload is finished
                //
                if (percent != 100)
                    wsp->push_msg(json_load_level(name, 100));
                wsp->push_msg(json_load_icon(name, false));
            },
            cyng::access::read(table_name),
            cyng::access::read("meter"));

        return true;
    }

    bool http_server::response_subscribe_channel_gwIEC(ws_sptr wsp, std::string const &name, std::string const &table_name) {
        //
        //	Send initial data set
        //
        db_.cache_.access(
            [&](cyng::table const *tbl_gw, cyng::table const *tbl_meter, cyng::table const *tbl_iec) -> void {
                //
                //	inform client that data upload is starting
                //
                wsp->push_msg(json_load_icon(name, true));

                //
                //	get total record size
                //
                auto total_size{tbl_gw->size()};
                std::size_t percent{0}, idx{0};

                tbl_gw->loop([&](cyng::record &&rec, std::size_t idx) -> bool {
                    cyng::vector_t units;

                    //
                    //  meter counter
                    //
                    std::uint32_t counter{0};

                    //
                    // find all associated meterIEC and meter
                    //
                    auto const host = rec.value("host", "");
                    auto const port = rec.value<std::uint16_t>("port", 0);
                    auto const meter_counter = rec.value<std::uint32_t>("meterCounter", 0);

                    tbl_iec->loop([&](cyng::record &&rec_iec, std::size_t idx) -> bool {
                        auto const host_iec = rec_iec.value("host", "");
                        auto const port_iec = rec_iec.value<std::uint16_t>("port", 0);
                        if (boost::algorithm::equals(host, host_iec) && (port == port_iec)) {

                            //
                            //  update meter counter
                            //
                            ++counter;

                            auto const rec_meter = tbl_meter->lookup(rec_iec.key());
                            if (!rec_meter.empty()) {
                                auto const ident = rec_meter.value("ident", "");
                                auto const meter = rec_meter.value("meter", "");
                                units.push_back(cyng::make_object(meter));
                            }
                        }
                        //
                        //  small optimization
                        //  return false if all configured meters complete
                        //
                        return counter != meter_counter;
                    });

                    auto tpl = rec.to_tuple(cyng::param_map_factory("units", units));
                    auto const str = json_insert_record(name, std::move(tpl));
                    CYNG_LOG_DEBUG(logger_, str);
                    wsp->push_msg(str);
                    return true;
                });

                //
                //	inform client that data upload is finished
                //
                if (percent != 100)
                    wsp->push_msg(json_load_level(name, 100));
                wsp->push_msg(json_load_icon(name, false));
            },
            cyng::access::read(table_name),
            cyng::access::read("meter"),
            cyng::access::read("meterIEC"));

        return true;
    }

    bool http_server::response_subscribe_channel_gateway(ws_sptr wsp, std::string const &name, std::string const &table_name) {
        //
        //	Send initial data set
        //
        db_.cache_.access(
            [&](cyng::table const *tbl_wmbus, cyng::table const *tbl_dev, cyng::table const *tbl_session) -> void {
                //
                //	inform client that data upload is starting
                //
                wsp->push_msg(json_load_icon(name, true));

                //
                //	get total record size
                //
                auto total_size{tbl_wmbus->size()};
                std::size_t percent{0}, idx{0};

                tbl_wmbus->loop([&](cyng::record &&rec, std::size_t idx) -> bool {
                    //
                    // get online state
                    //
                    auto const online = tbl_session->exist(rec.key());

                    //
                    // get additional data from meter
                    //
                    auto const rec_dev = tbl_dev->lookup(rec.key());
                    if (!rec_dev.empty()) {
                        auto tpl = rec.to_tuple(
                            cyng::param_map_factory("model", rec_dev.value("id", ""))("vFirmware", rec_dev.value("vFirmware", ""))(
                                "name", rec_dev.value("name", ""))("descr", rec_dev.value("descr", ""))("online", online ? 1 : 0));
                        auto const str = json_insert_record(name, std::move(tpl));
                        CYNG_LOG_DEBUG(logger_, str);
                        wsp->push_msg(str);
                    } else {
                        CYNG_LOG_WARNING(logger_, "[HTTP] missing device configuration: " << rec.to_string());
                        auto tpl = rec.to_tuple(cyng::param_map_factory("model", "no-model")("vFirmware", "v0.0")(
                            "descr", "no-configuration")("online", online ? 1 : 0));
                        auto const str = json_insert_record(name, std::move(tpl));
                        CYNG_LOG_DEBUG(logger_, str);
                        wsp->push_msg(str);
                    }

                    //
                    //	update current index and percentage calculation
                    //
                    ++idx;
                    const auto prev_percent = percent;
                    percent = (100u * idx) / total_size;
                    if (prev_percent != percent) {
                        wsp->push_msg(json_load_level(name, percent));
                    }

                    return true;
                });

                //
                //	inform client that data upload is finished
                //
                if (percent != 100)
                    wsp->push_msg(json_load_level(name, 100));
                wsp->push_msg(json_load_icon(name, false));
            },
            cyng::access::read(table_name),
            cyng::access::read("device"),
            cyng::access::read("session"));
        return true;
    }

    void http_server::response_update_channel(ws_sptr wsp, std::string const &name) {

        if (boost::algorithm::starts_with(name, "sys.cpu.usage.total")) {
            auto const load = cyng::sys::get_cpu_load(0);
            wsp->push_msg(json_update_channel(name, load));
        } else if (boost::algorithm::starts_with(name, "sys.cpu.count")) {
            wsp->push_msg(json_update_channel(name, std::thread::hardware_concurrency()));
        } else if (boost::algorithm::starts_with(name, "sys.mem.virtual.total")) {
            wsp->push_msg(json_update_channel(name, cyng::sys::get_total_ram()));
        } else if (boost::algorithm::starts_with(name, "sys.mem.virtual.used")) {
            wsp->push_msg(json_update_channel(name, cyng::sys::get_used_ram()));
        } else if (boost::algorithm::starts_with(name, "sys.mem.virtual.stat")) {
            wsp->push_msg(json_update_channel(name, cyng::sys::get_used_ram()));
        } else {
            CYNG_LOG_WARNING(logger_, "[HTTP] update undefined channel " << name);
        }
    }

    void http_server::notify_remove(std::string channel, cyng::key_t key, boost::uuids::uuid tag) {
        auto const pos = ws_map_.find(tag);
        if (pos != ws_map_.end()) {
            auto sp = pos->second.lock();
            if (sp) {
                sp->push_msg(json_delete_record(channel, key));
            } else {
                //	shouldn't be necessary
                BOOST_ASSERT_MSG(false, "invalid ws map - notify_remove");
                ws_map_.erase(pos);
            }
        }
    }

    void http_server::notify_clear(std::string channel, boost::uuids::uuid tag) {
        auto const pos = ws_map_.find(tag);
        if (pos != ws_map_.end()) {
            auto sp = pos->second.lock();
            if (sp) {
                sp->push_msg(json_clear_table(channel));
            } else {
                //	shouldn't be necessary
                BOOST_ASSERT_MSG(false, "invalid ws map - notify_clear");
                ws_map_.erase(pos);
            }
        }
    }

    void http_server::notify_insert(std::string channel, cyng::record &&rec, boost::uuids::uuid tag) {
        auto const pos = ws_map_.find(tag);
        if (pos != ws_map_.end()) {
            auto sp = pos->second.lock();
            if (sp) {
                sp->push_msg(json_insert_record(channel, rec.to_tuple()));
            } else {
                //	shouldn't be necessary
                BOOST_ASSERT_MSG(false, "invalid ws map - notify_insert");
                ws_map_.erase(pos);
            }
        }
    }

    void http_server::notify_update(std::string channel, cyng::key_t key, cyng::param_t const &param, boost::uuids::uuid tag) {
        auto const pos = ws_map_.find(tag);
        if (pos != ws_map_.end()) {
            auto sp = pos->second.lock();
            if (sp)
                sp->push_msg(json_update_record(channel, key, param));

            else {
                //	shouldn't be necessary
                BOOST_ASSERT_MSG(false, "invalid ws map - notify_update");
                ws_map_.erase(pos);
            }
        }
    }

    void http_server::notify_size(std::string channel, std::size_t size, boost::uuids::uuid tag) {

        auto const pos = ws_map_.find(tag);
        if (pos != ws_map_.end()) {
            auto sp = pos->second.lock();
            if (sp) {
                auto const str = json_update_channel(channel, size);
                sp->push_msg(str);
            }
        }
    }

    std::string json_insert_record(std::string channel, cyng::tuple_t &&tpl) {

        return cyng::io::to_json(cyng::make_tuple(
            cyng::make_param("cmd", "insert"), cyng::make_param("channel", channel), cyng::make_param("rec", std::move(tpl))));
    }

    std::string json_update_record(std::string channel, cyng::key_t const &key, cyng::param_t const &param) {

        return cyng::io::to_json(cyng::make_tuple(
            cyng::make_param("cmd", "modify"),
            cyng::make_param("channel", channel),
            cyng::make_param("key", key),
            cyng::make_param("value", param)));
    }

    std::string json_load_icon(std::string channel, bool b) {
        return cyng::io::to_json(
            cyng::make_tuple(cyng::make_param("cmd", "load"), cyng::make_param("channel", channel), cyng::make_param("show", b)));
    }

    std::string json_load_level(std::string channel, std::size_t level) {

        return cyng::io::to_json(cyng::make_tuple(
            cyng::make_param("cmd", "load"), cyng::make_param("channel", channel), cyng::make_param("level", level)));
    }

    std::string json_delete_record(std::string channel, cyng::key_t const &key) {

        return cyng::io::to_json(cyng::make_tuple(
            cyng::make_param("cmd", "delete"), cyng::make_param("channel", channel), cyng::make_param("key", key)));
    }

    std::string json_clear_table(std::string channel) {

        return cyng::io::to_json(cyng::make_tuple(cyng::make_param("cmd", "clear"), cyng::make_param("channel", channel)));
    }

    void tidy(cyng::param_map_t &pm) {

        for (auto pos = pm.begin(); pos != pm.end();) {
            if (!pos->first.empty() && pos->first.at(0) == '_')
                pos = pm.erase(pos);
            else if (pos->first.empty())
                pos = pm.erase(pos);
            else if (boost::algorithm::equals(pos->first, "tag"))
                pos = pm.erase(pos);
            else if (boost::algorithm::equals(pos->first, "source"))
                pos = pm.erase(pos);
            else
                ++pos;
        }
    }

} // namespace smf
