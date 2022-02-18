/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_DB_H
#define SMF_MAIN_DB_H

#include <smf/config/kv_store.h>

#include <smf/config/schemes.h>

#include <cyng/log/logger.h>

#include <boost/uuid/random_generator.hpp>

namespace smf {

    /**
     * To address a party (remote session) two components are required:
     *
     * <li>tag</li>
     * <li>peer</li>
     */
    using pty = std::pair<boost::uuids::uuid, boost::uuids::uuid>;

    /**
     * To address a push target two components are required:
     *
     * <li>pty</li>
     * <li>channel</li>
     */
    struct push_target {
        push_target();
        push_target(pty, std::uint32_t);
        pty pty_;
        std::uint32_t channel_;
    };

    /** @brief provide all functions around synchronized data access
     */
    class db {

      public:
        db(cyng::store &cache, cyng::logger, boost::uuids::uuid tag);

        inline kv_store &get_cfg() { return cfg_; }
        inline kv_store const &get_cfg() const { return cfg_; }

        inline cyng::store &get_store() { return cache_; }

        /**
         * fill store map and create all tables
         */
        void init(cyng::param_map_t const &, std::string const &country_code, std::string const &lang_code);

        /**
         * Add a new cluster node into table "cluster"
         */
        bool insert_cluster_member(
            boost::uuids::uuid tag,
            boost::uuids::uuid peer,
            std::string class_name,
            cyng::version,
            boost::asio::ip::tcp::endpoint,
            cyng::pid,
            bool);

        bool remove_cluster_member(boost::uuids::uuid);

        bool insert_pty(
            boost::uuids::uuid tag,
            boost::uuids::uuid peer,
            boost::uuids::uuid dev,
            std::string const &name,
            std::string const &pwd,
            boost::asio::ip::tcp::endpoint ep,
            std::string const &data_layer);

        /**
         * @return session key to an possible open connection
         */
        std::pair<cyng::key_t, bool> remove_pty(boost::uuids::uuid, boost::uuids::uuid);

        /**
         * @return rTag and peer of the specified session
         */
        pty get_access_params(cyng::key_t);

        /**
         * remove all sessions of this peer. "cluster" table
         * will be updated too.
         *
         * @return number of removed sessions
         */
        std::size_t remove_pty_by_peer(boost::uuids::uuid peer, boost::uuids::uuid remote_peer);

        /**
         * update pty counter in cluster table according to current
         * count of members in "session" table
         */
        std::size_t update_pty_counter(boost::uuids::uuid peer, boost::uuids::uuid remote_peer);

        /**
         * @return the tag of the device with the specified credentials. Returns a null-tag
         * if no matching device was found. The boolean signals if the device is enabled or not
         */
        std::pair<boost::uuids::uuid, bool> lookup_device(std::string const &name, std::string const &pwd);

        bool insert_device(boost::uuids::uuid tag, std::string const &name, std::string const &pwd, bool enabled);

        /**
         * search a session by msisdn
         * @return
         *	(1) session already connected,
         *	(2) remote session online and enabled,
         *	(3) remote session vm-tag
         *  (4) remote session dev-tag
         *  (5) caller name
         *  (6) callee name
         */
        std::tuple<bool, bool, boost::uuids::uuid, boost::uuids::uuid, std::string, std::string>
        lookup_msisdn(std::string msisdn, boost::uuids::uuid dev);

        /**
         * update session and connection table
         */
        void establish_connection(
            boost::uuids::uuid caller_tag,
            boost::uuids::uuid caller_vm,
            boost::uuids::uuid caller_dev,
            std::string caller_name,
            std::string callee_name,
            boost::uuids::uuid tag,    //	callee tag
            boost::uuids::uuid dev,    //	callee dev-tag
            boost::uuids::uuid callee, //	callee vm-tag
            bool local);

        /**
         * update session and connection table
         */
        [[nodiscard]] std::tuple<boost::uuids::uuid, boost::uuids::uuid, boost::uuids::uuid, bool>
        clear_connection(boost::uuids::uuid dev);

        /**
         * Search for a remote session (if connected)
         */
        [[nodiscard]] std::tuple<boost::uuids::uuid, boost::uuids::uuid, boost::uuids::uuid> get_remote(boost::uuids::uuid);

        /**
         * insert new system message
         */
        bool push_sys_msg(std::string msg, cyng::severity);

        /**
         * insert new system message
         */
        template <typename Head, typename... Args> bool sys_msg(cyng::severity level, Head &&v, Args &&...args) {
            std::stringstream ss;
            ss << v;
            ((ss << ' ' << args), ...);
            return push_sys_msg(ss.str(), level);
        }

        [[nodiscard]] std::pair<std::uint32_t, bool> register_target(
            boost::uuids::uuid tag,
            boost::uuids::uuid dev,
            boost::uuids::uuid peer,
            std::string name,
            std::uint16_t paket_size,
            std::uint8_t window_size);

        /**
         *  @param dev key in table "device"
         *  @param name target name
         *  @param account account (unused yet)
         *  @param number msisdn (unused yet)
         *  @param sv software version (unused yet)
         *  @param id device id / model (unused yet)
         *  @param timeout channel timeout
         *  @return [channel, source, packet_size, count]
         */
        [[nodiscard]] std::tuple<std::uint32_t, std::uint32_t, std::uint16_t, std::uint32_t, std::string> open_channel(
            boost::uuids::uuid dev,
            std::string name,
            std::string account,
            std::string number,
            std::string sv,
            std::string id,
            std::chrono::seconds timeout);

        /**
         * lookup "channel" table for matching channels and return all
         * targets connected to this channel.
         */
        [[nodiscard]] std::vector<push_target> get_matching_channels(std::uint32_t, std::size_t);

        /**
         * remove push channel
         */
        std::size_t close_channel(std::uint32_t);

        /**
         * update "throughput" in "connection" table
         */
        void update_connection_throughput(boost::uuids::uuid rtag, boost::uuids::uuid dev, std::uint64_t size);

        /**
         * Set "cfg" flag in "cluster" table
         */
        void update_ping_result(boost::uuids::uuid peer, std::chrono::microseconds delta);

      private:
        void set_start_values(cyng::param_map_t const &session_cfg, std::string const &country_code, std::string const &lang_code);
        void init_sys_msg();
        void init_LoRa_uplink();
        void init_iec_uplink();
        void init_wmbus_uplink();
        void init_demo_data();

        std::pair<cyng::key_list_t, std::uint16_t> get_matching_targets(
            cyng::table const *tbl,
            std::string name,
            std::string account,
            std::string number,
            std::string sv,
            std::string id,
            boost::uuids::uuid dev);

      private:
        cyng::store &cache_;
        cyng::logger logger_;
        kv_store cfg_;
        config::store_map store_map_;
        boost::uuids::random_generator uuid_gen_;
        std::uint32_t source_;
        std::uint32_t channel_target_;
        std::uint32_t channel_pty_;
    };

    /**
     * @return vector of all required meta data
     */
    std::vector<cyng::meta_store> get_store_meta_data();

} // namespace smf

#endif
