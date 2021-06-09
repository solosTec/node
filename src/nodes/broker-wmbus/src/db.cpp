/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>
#include <smf/cluster/bus.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    db::db(cyng::store &cache, cyng::logger logger, boost::uuids::uuid tag)
        : cache_(cache)
        , logger_(logger)
        , tag_(tag)
        , store_map_()
        , dep_key_() {}

    void db::init(cyng::slot_ptr slot) {
        //
        //	initialize cache
        //
        auto const vec = get_store_meta_data();
        for (auto const m : vec) {
            CYNG_LOG_INFO(logger_, "init table [" << m.get_name() << "]");
            store_map_.emplace(m.get_name(), m);
            cache_.create_table(m);
            cache_.connect(m.get_name(), slot);
        }
        BOOST_ASSERT(vec.size() == cache_.size());
    }

    void db::disconnect(cyng::slot_ptr slot) { cache_.disconnect(slot); }

    std::string db::get_next_table(std::string const &name) const {

        auto const vec = get_store_meta_data();
        auto pos = std::find_if(
            vec.begin(), vec.end(), [name](cyng::meta_store const &m) { return boost::algorithm::equals(name, m.get_name()); });
        if (pos != vec.end()) {
            ++pos; //  next table
            if (pos != vec.end()) {
                return pos->get_name();
            }
        }
        return ""; //  no more tables
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

    bool db::forward(cyng::table const *tbl, cyng::key_t const &, cyng::data_t const &, std::uint64_t, boost::uuids::uuid) {

        CYNG_LOG_TRACE(logger_, "[db] insert: " << tbl->meta().get_name());

        return true;
    }

    /**
     * update
     */
    bool db::forward(
        cyng::table const *tbl,
        cyng::key_t const &key,
        cyng::attr_t const &attr,
        std::uint64_t gen,
        boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[db] update: " << tbl->meta().get_name());
        return true;
    }

    /**
     * remove
     */
    bool db::forward(cyng::table const *tbl, cyng::key_t const &key, boost::uuids::uuid tag) {

        CYNG_LOG_TRACE(logger_, "[db] remove: " << tbl->meta().get_name());
        return true;
    }

    /**
     * clear
     */
    bool db::forward(cyng::table const *tbl, boost::uuids::uuid) {

        CYNG_LOG_TRACE(logger_, "[db] clear: " << tbl->meta().get_name());
        return true;
    }

    /**
     * transaction
     */
    bool db::forward(cyng::table const *tbl, bool trx) {

        CYNG_LOG_TRACE(logger_, "[db] trx: " << (trx ? " start" : " commit"));
        return true;
    }

    std::pair<cyng::crypto::aes_128_key, boost::uuids::uuid> db::lookup_meter(std::string id) {

        cyng::crypto::aes_128_key key;
        boost::uuids::uuid tag = boost::uuids::nil_uuid();

        cache_.access(
            [&](cyng::table const *tbl_meter, cyng::table const *tbl_wmbus) -> void {
                tbl_meter->loop([&](cyng::record &&rec, std::size_t idx) -> bool {
                    // CYNG_LOG_TRACE(logger_, "[db] lookup meter #" << idx << " - " << id << " / " << rec.value("meter", ""));
                    if (boost::algorithm::equals(rec.value("meter", ""), id)) {
                        CYNG_LOG_TRACE(logger_, "[db] meter " << id << " found: " << rec.value("tag", boost::uuids::nil_uuid()));
                        tag = rec.value("tag", boost::uuids::nil_uuid());
                        return false;
                    }
                    return true;
                });

                auto const rec = tbl_wmbus->lookup(cyng::key_generator(tag));
                if (!rec.empty()) {

                    //
                    //	get aes key
                    //
                    key = rec.value("aes", cyng::crypto::aes_128_key());
                    CYNG_LOG_TRACE(logger_, "[db] meter " << id << " has AES key: " << key);
                } else {
                    CYNG_LOG_WARNING(logger_, "[db] meter " << id << " is not configured as wM-Bus {" << tag << "}");
                    tag = boost::uuids::nil_uuid(); //	zero out UUID
                }
            },
            cyng::access::read("meter"),
            cyng::access::read("meterwMBus"));

        //
        //	return result
        //
        return {key, tag};
    }

    cyng::key_t db::update_gw_status(boost::asio::ip::tcp::endpoint ep, bus &cluster_bus) {

        cyng::key_t key;
        cache_.access(
            [&](cyng::table *tbl_gw) {
                tbl_gw->loop([&](cyng::record &&rec, std::size_t idx) -> bool {
                    auto const addr = rec.value("host", boost::asio::ip::address());

                    if (addr == ep.address()) {

                        //  calculate next connect counter
                        auto const connect_counter = rec.value("connectCounter", static_cast<std::uint32_t>(0)) + 1u;

                        //
                        //  get key and update port
                        //
                        key = rec.key();
                        cluster_bus.req_db_update(
                            tbl_gw->meta().get_name(),
                            key,
                            cyng::param_map_factory()("port", ep.port()) //  tcp/ip port
                            ("status", static_cast<std::uint16_t>(1))    //  status: connecting
                            ("connectCounter", connect_counter));

                        return false; //  complete
                    }
                    return true;
                });

                if (key.empty()) {
                    //
                    //  create new key
                    //
                    key = dep_key_(ep.address());
                    cluster_bus.req_db_insert(
                        tbl_gw->meta().get_name(),
                        key,
                        cyng::data_generator(
                            ep.address(),                  //   host
                            static_cast<std::uint32_t>(1), //  connectCounter
                            static_cast<std::uint16_t>(1), //  state connecting
                            std::string("[initial]"),      //  current meter id/name
                            ep.port()                      //  current TCP/IP port
                            ),
                        1);
                }
            },
            cyng::access::write("gwwMBus"));

        return key;
    }

    std::vector<cyng::meta_store> get_store_meta_data() {
        return {
            config::get_store_meter(),      //  all meters
            config::get_store_meterwMBus(), //  all wireless M-Bus meters
            config::get_store_gwwMBus(),    //	"status"

        };
    }

} // namespace smf
