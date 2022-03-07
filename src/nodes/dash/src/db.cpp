/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    db::db(cyng::store &cache, cyng::logger logger, boost::uuids::uuid tag)
        : cfg_(cache, tag)
        , cache_(cache)
        , logger_(logger)
        , store_map_()
        , subscriptions_()
        , uidgen_() {}

    void db::init(
        std::uint64_t max_upload_size,
        std::string const &nickname,
        std::chrono::seconds timeout,
        std::string const &country_code,
        std::string const &lang_code,
        cyng::slot_ptr slot) {
        //
        //	initialize cache
        //
        auto const vec = get_store_meta_data();
        for (auto const m : vec) {
            CYNG_LOG_INFO(logger_, "init table [" << m.get_name() << "]");
            store_map_.emplace(m.get_name(), m);
            cache_.create_table(m);
            cache_.connect_only(m.get_name(), slot);
        }
        BOOST_ASSERT(vec.size() == cache_.size());

        //
        //	set start values
        //
        set_start_values(max_upload_size, nickname, timeout, country_code, lang_code);
    }

    void db::set_start_values(
        std::uint64_t max_upload_size,
        std::string const &nickname,
        std::chrono::seconds timeout,
        std::string const &country_code,
        std::string const &lang_code) {

        cfg_.set_value("http-session-timeout", timeout);
        cfg_.set_value("http-max-upload-size", max_upload_size); //	10.485.760 bytes
        cfg_.set_value("http-server-nickname", nickname);        //	X-ServerNickName
        cfg_.set_value("https-redirect", false);                 //	redirect HTTP traffic to HTTPS port
        cfg_.set_value("country-code", country_code);
        cfg_.set_value("language-code", lang_code);
    }

    db::rel db::by_table(std::string const &name) const {

        auto const pos =
            std::find_if(rel_.begin(), rel_.end(), [name](rel const &rel) { return boost::algorithm::equals(name, rel.table_); });

        return (pos == rel_.end()) ? rel() : *pos;
    }

    /**
     * search a relation record by channel name
     */

    db::rel db::by_channel(std::string const &name) const {
        auto pos =
            std::find_if(rel_.begin(), rel_.end(), [name](rel const &rel) { return boost::algorithm::equals(name, rel.channel_); });

        return (pos == rel_.end()) ? rel() : *pos;
    }

    db::rel db::by_counter(std::string const &name) const {

        auto pos =
            std::find_if(rel_.begin(), rel_.end(), [name](rel const &rel) { return boost::algorithm::equals(name, rel.counter_); });

        return (pos == rel_.end()) ? rel() : *pos;
    }

    void db::loop_rel(std::function<void(rel const &)> cb) const {
        for (auto const &r : rel_) {
            cb(r);
        }
    }

    void db::res_insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {

        std::reverse(key.begin(), key.end());
        std::reverse(data.begin(), data.end());

        CYNG_LOG_TRACE(logger_, "[cluster] db.res.insert: " << table_name << " - " << data);

        cache_.insert(table_name, key, data, gen, tag);
    }

    void db::res_update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] db.res.update: " << table_name << " - " << attr.first << " => " << attr.second);

        //
        //	notify all subscriptions
        //
        std::reverse(key.begin(), key.end());
        cache_.modify(table_name, key, std::move(attr), tag);
    }

    void db::res_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[cluster] db.res.remove: " << table_name << " - " << key);

        //
        //	remove from cache
        //
        cache_.erase(table_name, key, tag);
    }

    void db::res_clear(std::string table_name, boost::uuids::uuid tag) {

        //
        //	clear table
        //
        cache_.clear(table_name, tag);
    }

    void db::add_to_subscriptions(std::string const &channel, boost::uuids::uuid tag) { subscriptions_.emplace(channel, tag); }

    std::size_t db::remove_from_subscription(boost::uuids::uuid tag) {

        std::size_t counter{0};
        for (auto pos = subscriptions_.begin(); pos != subscriptions_.end();) {
            if (pos->second == tag) {
                pos = subscriptions_.erase(pos);
                ++counter;
            } else {
                ++pos;
            }
        }
        return counter;
    }

    void db::convert(std::string const &table_name, cyng::vector_t &key) {

        auto const pos = store_map_.find(table_name);
        if (pos != store_map_.end()) {

            //
            //	table meta data
            //
            auto const &meta = pos->second;

            std::size_t index{0};
            for (auto &k : key) {
                auto const col = meta.get_column(index);
                k = convert_to_type(col.type_, k);
            }
        }
    }

    void db::convert(std::string const &table_name, cyng::vector_t &key, cyng::param_map_t &data) {

        auto const pos = store_map_.find(table_name);
        if (pos != store_map_.end()) {

            //
            //	table meta data
            //
            auto const &meta = pos->second;

            std::size_t index{0};
            for (auto &k : key) {
                auto const col = meta.get_column(index);
                k = convert_to_type(col.type_, k);
            }

            for (auto pos = data.begin(); pos != data.end();) {

                auto const idx = meta.get_index_by_name(pos->first);
                if (idx != std::numeric_limits<std::size_t>::max()) {

                    auto const col = meta.get_column(idx);
                    BOOST_ASSERT(boost::algorithm::equals(col.name_, pos->first));

                    pos->second = convert_to_type(col.type_, pos->second);
                    ++pos;
                } else if (boost::algorithm::equals(table_name, "meterwMBus") && boost::algorithm::equals(pos->first, "meter")) {

                    //
                    //	second update required if "meter" has changed
                    //
                    pos = data.erase(pos);
                    CYNG_LOG_DEBUG(logger_, "[db] convert: column [" << pos->first << "] in table " << table_name << " removed");
                } else {
                    CYNG_LOG_ERROR(logger_, "[db] convert: unknown column [" << pos->first << "] in table " << table_name);
                    ++pos;
                }
            }
        } else {
            CYNG_LOG_ERROR(logger_, "[db] convert: unknown table " << table_name);
        }
    }

    cyng::vector_t db::generate_new_key(std::string const &table_name, cyng::vector_t &&key, cyng::param_map_t const &data) {

        if (boost::algorithm::equals(table_name, "device") || boost::algorithm::equals(table_name, "meter") ||
            boost::algorithm::equals(table_name, "meterIEC") || boost::algorithm::equals(table_name, "meterwMBus") ||
            boost::algorithm::equals(table_name, "gateway") || boost::algorithm::equals(table_name, "loRaDevice") ||
            boost::algorithm::equals(table_name, "guiUser") || boost::algorithm::equals(table_name, "location")) {
            return cyng::key_generator(uidgen_());
        }
        CYNG_LOG_WARNING(logger_, "[db] cannot generate pk for table " << table_name);
        return key;
    }

    cyng::data_t db::complete(std::string const &table_name, cyng::param_map_t &&pm) {

        cyng::data_t data;
        auto const pos = store_map_.find(table_name);
        if (pos != store_map_.end()) {

            //
            //	table meta data
            //
            auto const &meta = pos->second;
            meta.loop([&](std::size_t, cyng::column const &col, bool pk) -> void {
                if (!pk) {
                    auto pos = pm.find(col.name_);
                    if (pos != pm.end()) {

                        //
                        //	column found
                        //	convert to expected data type
                        //
                        data.push_back(convert_to_type(col.type_, pos->second));

                    } else {
                        //
                        //	column is missing
                        //	generate a default value
                        //
                        data.push_back(generate_empty_value(col.type_));
                    }
                }
            });
        } else {
            CYNG_LOG_ERROR(logger_, "[db] complete: unknown table " << table_name);
        }
        return data;
    }

    cyng::object db::generate_empty_value(cyng::type_code tc) {
        switch (tc) {
        case cyng::TC_UUID:
            return cyng::make_object(boost::uuids::nil_uuid());
        case cyng::TC_TIME_POINT:
            return cyng::make_object(std::chrono::system_clock::now());
        case cyng::TC_IP_ADDRESS:
            return cyng::make_object(boost::asio::ip::address());
        case cyng::TC_AES128:
            return cyng::make_object(cyng::to_aes_key<cyng::crypto::aes128_size>("00000000000000000000000000000000"));
        case cyng::TC_AES192:
            return cyng::make_object(
                cyng::to_aes_key<cyng::crypto::aes192_size>("000000000000000000000000000000000000000000000000"));
        case cyng::TC_AES256:
            return cyng::make_object(
                cyng::to_aes_key<cyng::crypto::aes256_size>("0000000000000000000000000000000000000000000000000000000000000000"));

        case cyng::TC_BUFFER:
            return cyng::make_object(cyng::buffer_t());
        case cyng::TC_VERSION:
            return cyng::make_object(cyng::version());
        case cyng::TC_REVISION:
            return cyng::make_object(cyng::revision());
        case cyng::TC_OP:
            return cyng::make_object(cyng::op::NOOP);
        case cyng::TC_SEVERITY:
            return cyng::make_object(cyng::severity::LEVEL_INFO);
        case cyng::TC_MAC48:
            return cyng::make_object(cyng::mac48());
        case cyng::TC_MAC64:
            return cyng::make_object(cyng::mac64());
        case cyng::TC_PID:
            return cyng::make_object(cyng::pid());
        case cyng::TC_OBIS:
            return cyng::make_object(cyng::obis());
        case cyng::TC_EDIS:
            return cyng::make_object(cyng::edis());

        case cyng::TC_UINT8:
            return cyng::make_object<std::uint8_t>(0);
        case cyng::TC_UINT16:
            return cyng::make_object<std::uint16_t>(0);
        case cyng::TC_UINT32:
            return cyng::make_object<std::uint32_t>(0);
        case cyng::TC_UINT64:
            return cyng::make_object<std::uint64_t>(0);
        case cyng::TC_INT8:
            return cyng::make_object<std::int8_t>(0);
        case cyng::TC_INT16:
            return cyng::make_object<std::int16_t>(0);
        case cyng::TC_INT32:
            return cyng::make_object<std::int32_t>(0);
        case cyng::TC_INT64:
            return cyng::make_object<std::int64_t>(0);

        case cyng::TC_STRING:
            return cyng::make_object("");
        case cyng::TC_NANO_SECOND:
            return cyng::make_object(std::chrono::nanoseconds(0));
        case cyng::TC_MICRO_SECOND:
            return cyng::make_object(std::chrono::microseconds(0));
        case cyng::TC_MILLI_SECOND:
            return cyng::make_object(std::chrono::milliseconds(0));
        case cyng::TC_SECOND:
            return cyng::make_object(std::chrono::seconds(0));
        case cyng::TC_MINUTE:
            return cyng::make_object(std::chrono::minutes(0));
        case cyng::TC_HOUR:
            return cyng::make_object(std::chrono::hours(0));

        default:
            break;
        }
        return cyng::make_object();
    }

    cyng::key_t db::lookup_meter_by_id(std::string const &id) {

        cyng::key_t key;
        cache_.access(
            [&](cyng::table const *tbl) {
                tbl->loop([&](cyng::record &&rec, std::size_t) -> bool {
                    auto const meter = rec.value("meter", "");
                    if (boost::algorithm::equals(meter, id)) {
                        //	found
                        key = rec.key();
                        return false;
                    }

                    return true;
                });
            },
            cyng::access::read("meter"));

        return key;
    }

    void db::update_table_meter_full() {

        cache_.access(
            [&](cyng::table *tbl_full,
                cyng::table const *tbl_device,
                cyng::table const *tbl_meter,
                cyng::table const *tbl_iec,
                cyng::table const *tbl_wmbus,
                cyng::table const *tbl_loc) {
                //
                //  remove old data
                //
                tbl_full->clear(cfg_.get_tag());

                tbl_iec->loop([&](cyng::record &&rec_iec, std::size_t) -> bool {
                    auto const &key = rec_iec.key();
                    auto const rec_meter = tbl_meter->lookup(key);
                    if (!rec_meter.empty()) {

                        auto const rec_loc = tbl_loc->lookup(key);
                        auto const region = rec_loc.empty() ? std::string("-") : rec_loc.value("region", "");
                        auto const address = rec_loc.empty() ? std::string("-") : rec_loc.value("address", "");

                        //  insert into "meterFull"
                        tbl_full->insert(
                            key,
                            cyng::data_generator(
                                rec_meter.at("meter"),    //  meter -> "Meter ID"
                                rec_meter.at("code"),     //  code -> Meter_ID
                                rec_meter.at("maker"),    //  maker-> Manufacturer
                                rec_meter.at("protocol"), //  protocol -> Protocol
                                rec_iec.at("host"),       //  host -> GWY_IP
                                rec_iec.at("port"),       //  port -> Ports
                                "-",                      //  no AES key avaliable
                                region,
                                address),
                            1u,
                            cfg_.get_tag());
                    }

                    return true;
                });
                tbl_wmbus->loop([&](cyng::record &&rec_wmbus, std::size_t) -> bool {
                    auto const &key = rec_wmbus.key();
                    auto const rec_meter = tbl_meter->lookup(key);
                    if (!rec_meter.empty()) {

                        auto const rec_loc = tbl_loc->lookup(key);
                        auto const region = rec_loc.empty() ? std::string("-") : rec_loc.value("region", "");
                        auto const address = rec_loc.empty() ? std::string("-") : rec_loc.value("address", "");

                        //  insert into "meterFull"
                        tbl_full->insert(
                            key,
                            cyng::data_generator(
                                rec_meter.at("meter"),    //  meter -> "Meter ID"
                                rec_meter.at("code"),     //  code -> Meter_ID
                                rec_meter.at("maker"),    //  maker-> Manufacturer
                                rec_meter.at("protocol"), //  protocol -> Protocol
                                rec_wmbus.at("address"),  //  address -> GWY_IP
                                rec_wmbus.at("port"),     //  port -> Ports
                                rec_wmbus.at("aes"),      //  aes -> Key
                                region,
                                address),
                            1u,
                            cfg_.get_tag());
                    }
                    return true;
                });
            },
            cyng::access::write("meterFull"),
            cyng::access::read("device"),
            cyng::access::read("meter"),
            cyng::access::read("meterIEC"),
            cyng::access::read("meterwMBus"),
            cyng::access::read("location"));
    }

    db::rel::rel(std::string table, std::string channel, std::string counter)
        : table_(table)
        , channel_(channel)
        , counter_(counter) {}

    bool db::rel::empty() const { return table_.empty(); }

    db::array_t const db::rel_ = {
        db::rel{"device", "config.device", "table.device.count"},
        db::rel{"loRaDevice", "config.lora", "table.LoRa.count"},
        db::rel{"guiUser", "config.user", "table.user.count"},
        db::rel{"target", "status.target", "table.target.count"},
        db::rel{"cluster", "status.cluster", "table.cluster.count"},
        db::rel{"config", "config.system", ""},
        db::rel{"gateway", "config.gateway", "table.gateway.count"},
        db::rel{"meter", "config.meter", "table.meter.count"},
        db::rel{"meterIEC", "config.iec", "table.iec.count"},
        db::rel{"meterwMBus", "config.wmbus", "table.wmbus.count"},
        db::rel{"location", "config.location", "table.location.count"},
        db::rel{"session", "status.session", "table.session.count"},
        db::rel{"connection", "status.connection", "table.connection.count"},
        db::rel{"sysMsg", "monitor.msg", "table.msg.count"},
        db::rel{"wMBusUplink", "monitor.wMBus", "table.wMBusUplink.count"},
        db::rel{"iecUplink", "monitor.IEC", "table.IECUplink.count"},
        db::rel{"gwIEC", "status.IECgw", "table.gwIEC.count"},
        db::rel{"gwwMBus", "status.wMBusgw", "table.gwwMBus.count"},
        db::rel{"cfgSetMeta", "config.cfgSetMeta", "table.cfgSetMeta.count"}
        //  don't add table "meterFull" - no syncing
    };

    std::vector<cyng::meta_store> get_store_meta_data() {
        return {
            config::get_store_device(),
            config::get_store_meter(),
            config::get_store_meterIEC(),
            config::get_store_meterwMBus(),
            config::get_store_gateway(),
            config::get_store_lora(),
            config::get_store_gui_user(),
            config::get_store_target(),
            config::get_store_cluster(),
            config::get_store_location(),
            config::get_config(), //	"config"
            config::get_store_session(),
            config::get_store_connection(),
            config::get_store_sys_msg(),
            config::get_store_uplink_wmbus(),
            config::get_store_uplink_iec(),
            config::get_store_gwIEC(),
            config::get_store_gwwMBus(),
            config::get_store_meter_full(),
            config::get_store_cfg_set_meta()};
    }

    cyng::object convert_to_type(cyng::type_code tc, cyng::object &obj) {
        switch (tc) {
        case cyng::TC_UUID:
            return convert_to_uuid(obj);
        case cyng::TC_TIME_POINT:
            return convert_to_tp(obj);
        case cyng::TC_IP_ADDRESS:
            return convert_to_ip_address(obj);
        case cyng::TC_AES128:
            return convert_to_aes128(obj);
        case cyng::TC_AES192:
            return convert_to_aes192(obj);
        case cyng::TC_AES256:
            return convert_to_aes256(obj);

        case cyng::TC_UINT8:
            return convert_to_numeric<std::uint8_t>(obj);
        case cyng::TC_UINT16:
            return convert_to_numeric<std::uint16_t>(obj);
        case cyng::TC_UINT32:
            return convert_to_numeric<std::uint32_t>(obj);
        case cyng::TC_UINT64:
            return convert_to_numeric<std::uint64_t>(obj);
        case cyng::TC_INT8:
            return convert_to_numeric<std::int8_t>(obj);
        case cyng::TC_INT16:
            return convert_to_numeric<std::int16_t>(obj);
        case cyng::TC_INT32:
            return convert_to_numeric<std::int32_t>(obj);
        case cyng::TC_INT64:
            return convert_to_numeric<std::int64_t>(obj);

        case cyng::TC_NANO_SECOND:
            return convert_to_nanoseconds(obj);
        case cyng::TC_MICRO_SECOND:
            return convert_to_microseconds(obj);
        case cyng::TC_MILLI_SECOND:
            return convert_to_milliseconds(obj);
        case cyng::TC_SECOND:
            return convert_to_seconds(obj);
        case cyng::TC_MINUTE:
            return convert_to_minutes(obj);
        case cyng::TC_HOUR:
            return convert_to_hours(obj);

        default:
            // all other data types pass through
            break;
        }
        return obj;
    }

    cyng::object convert_to_uuid(cyng::object &obj) {
        switch (obj.tag()) {
        case cyng::TC_STRING: {
            auto const str = cyng::io::to_plain(obj);
            BOOST_ASSERT(str.size() == 36);
            return cyng::make_object(cyng::to_uuid(str));
        }
        case cyng::TC_UUID:
            return obj;
        default:
            BOOST_ASSERT(obj.tag() == cyng::TC_NULL);
            break;
        }
        return cyng::make_object(boost::uuids::nil_uuid());
    }

    cyng::object convert_to_tp(cyng::object &obj) {
        if (obj.tag() == cyng::TC_TIME_POINT)
            return obj;
        if (obj.tag() == cyng::TC_STRING) {
            auto const str = cyng::io::to_plain(obj);
            return cyng::make_object(cyng::to_tp_iso8601(str));
        }
        return cyng::make_object(std::chrono::system_clock::now());
    }

    cyng::object convert_to_ip_address(cyng::object &obj) {
        if (obj.tag() == cyng::TC_IP_ADDRESS)
            return obj;
        if (obj.tag() == cyng::TC_STRING) {
            auto const str = cyng::io::to_plain(obj);
            return cyng::make_object(cyng::to_ip_address(str));
        }
        return cyng::make_object(boost::asio::ip::make_address("0.0.0.0"));
    }

    cyng::object convert_to_aes128(cyng::object &obj) {
        if (obj.tag() == cyng::TC_AES128)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        BOOST_ASSERT_MSG(str.size() == cyng::crypto::aes_128_key::hex_size(), "aes128 key has wrong size");
        return (str.size() == cyng::crypto::aes_128_key::hex_size())
                   ? cyng::make_object(cyng::to_aes_key<cyng::crypto::aes128_size>(str))
                   : cyng::make_object(cyng::crypto::aes_128_key());
    }
    cyng::object convert_to_aes192(cyng::object &obj) {
        if (obj.tag() == cyng::TC_AES192)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        BOOST_ASSERT_MSG(str.size() == cyng::crypto::aes_192_key::hex_size(), "aes192 key has wrong size");
        return (str.size() == cyng::crypto::aes_192_key::hex_size())
                   ? cyng::make_object(cyng::to_aes_key<cyng::crypto::aes192_size>(str))
                   : cyng::make_object(cyng::crypto::aes_192_key());
    }
    cyng::object convert_to_aes256(cyng::object &obj) {
        if (obj.tag() == cyng::TC_AES256)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        BOOST_ASSERT_MSG(str.size() == cyng::crypto::aes_256_key::hex_size(), "aes256 key has wrong size");
        return (str.size() == cyng::crypto::aes_256_key::hex_size())
                   ? cyng::make_object(cyng::to_aes_key<cyng::crypto::aes256_size>(str))
                   : cyng::make_object(cyng::crypto::aes_256_key());
    }

    cyng::object convert_to_nanoseconds(cyng::object &obj) {
        if (obj.tag() == cyng::TC_NANO_SECOND)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        return obj;
    }

    cyng::object convert_to_microseconds(cyng::object &obj) {
        if (obj.tag() == cyng::TC_MICRO_SECOND)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        return cyng::make_object(cyng::to_microseconds(str));
    }

    cyng::object convert_to_milliseconds(cyng::object &obj) {
        if (obj.tag() == cyng::TC_MILLI_SECOND)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        return cyng::make_object(cyng::to_milliseconds(str));
    }

    cyng::object convert_to_seconds(cyng::object &obj) {
        if (obj.tag() == cyng::TC_SECOND)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        return cyng::make_object(cyng::to_seconds(str));
    }

    cyng::object convert_to_minutes(cyng::object &obj) {
        if (obj.tag() == cyng::TC_MINUTE)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        return cyng::make_object(cyng::to_minutes(str));
    }

    cyng::object convert_to_hours(cyng::object &obj) {
        if (obj.tag() == cyng::TC_HOUR)
            return obj;
        BOOST_ASSERT(obj.tag() == cyng::TC_STRING);
        auto const str = cyng::io::to_plain(obj);
        return cyng::make_object(cyng::to_hours(str));
    }

} // namespace smf
