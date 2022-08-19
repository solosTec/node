/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_SESSION_H
#define SMF_MAIN_SESSION_H

#include <db.h>

#include <cyng/io/parser/parser.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/pid.h>
#include <cyng/store/slot_interface.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/proxy.h>

#include <array>
#include <memory>

namespace smf {

    class session : public std::enable_shared_from_this<session> {
        class slot : public cyng::slot_interface {
          public:
            slot(session *);

            /**
             * insert
             */
            virtual bool
            forward(cyng::table const *, cyng::key_t const &, cyng::data_t const &, std::uint64_t, boost::uuids::uuid) override;

            /**
             * update
             */
            virtual bool forward(
                cyng::table const *tbl,
                cyng::key_t const &key,
                cyng::attr_t const &attr,
                cyng::data_t const &data,
                std::uint64_t gen,
                boost::uuids::uuid tag) override;

            /**
             * remove
             */
            virtual bool
            forward(cyng::table const *tbl, cyng::key_t const &key, cyng::data_t const &data, boost::uuids::uuid tag) override;

            virtual bool forward(cyng::table const *, boost::uuids::uuid) override;

            virtual bool forward(cyng::table const *, bool) override;

          private:
            session *sp_;
        };

      public:
        session(boost::asio::ip::tcp::socket socket, db &, cyng::mesh &, cyng::logger);
        ~session() = default;

        void start();
        void stop();

        /**
         * @return tag of remote session
         */
        boost::uuids::uuid get_peer() const;
        boost::uuids::uuid get_remote_peer() const;

      private:
        cyng::vm_proxy init_vm(cyng::mesh &);
        void do_read();
        void do_write();
        void handle_write(const boost::system::error_code &ec);

        void cluster_login(
            std::string,
            std::string,
            cyng::pid,
            std::string node,
            boost::uuids::uuid tag,
            std::uint32_t,
            cyng::version v);
        void cluster_ping(std::chrono::system_clock::time_point);

        void db_req_subscribe(std::string, boost::uuids::uuid tag);

        void
        db_req_insert(std::string table_name, cyng::key_t key, cyng::data_t data, std::uint64_t generation, boost::uuids::uuid);

        void db_req_insert_auto(std::string table_name, cyng::data_t data, boost::uuids::uuid);

        /**
         * table "gwIEC" is depended from table "meterIEC"
         */
        void db_req_insert_gw_iec(cyng::key_t key, boost::uuids::uuid tag);

        /**
         * table merge()
         */
        void db_req_update(std::string table_name, cyng::key_t key, cyng::param_map_t, boost::uuids::uuid);
        void db_req_update_gw_iec(cyng::key_t key, cyng::param_map_t const &, boost::uuids::uuid);
        void db_req_update_meter_iec_host_or_port(cyng::key_t const &key, cyng::param_t const &, boost::uuids::uuid);
        void update_iec_interval(std::chrono::seconds, boost::uuids::uuid);

        void auto_insert_gateway(cyng::key_t key, std::string id, boost::uuids::uuid);

        /**
         * Changes in table "gwIEC" must be applied to table "meterIEC" too
         */
        void db_req_update_meter_iec(cyng::key_t key, cyng::param_map_t const &, boost::uuids::uuid);

        /**
         * table erase()
         */
        void db_req_remove(std::string table_name, cyng::key_t key, boost::uuids::uuid source);

        /**
         * remove "gwIEC" record if no gateway has no longer any meters
         */
        void db_req_remove_gw_iec(cyng::key_t key, boost::uuids::uuid source);

        /**
         * table clear()
         */
        void db_req_clear(std::string table_name, boost::uuids::uuid source);

        /**
         * pty login
         */
        void pty_login(boost::uuids::uuid, std::string, std::string, boost::asio::ip::tcp::endpoint, std::string);

        /**
         * @brief pty logout
         *
         * Client session was closed.
         */
        void pty_logout(boost::uuids::uuid tag, boost::uuids::uuid dev);

        /**
         * pty.open.connection
         */
        void pty_open_connection(boost::uuids::uuid tag, boost::uuids::uuid dev, std::string msisdn, cyng::param_map_t token);

        /** @brief "pty.forward.open.connection"
         *
         */
        void pty_forward_res_open_connection(boost::uuids::uuid caller_tag, bool success, cyng::param_map_t token);

        /** @brief SMF internal command
         *
         * smf node answer to the "pty.forward.open.connection" command
         */
        void pty_return_open_connection(
            bool success,
            boost::uuids::uuid dev,    //	callee dev-tag
            boost::uuids::uuid callee, //	callee vm-tag
            cyng::param_map_t token);

        /**
         * pty.transfer.data
         */
        void pty_transfer_data(boost::uuids::uuid tag, boost::uuids::uuid dev, cyng::buffer_t);

        /**
         * pty.close.connection
         */
        void pty_close_connection(boost::uuids::uuid tag, boost::uuids::uuid dev, cyng::param_map_t);

        void pty_register_target(
            boost::uuids::uuid tag,
            boost::uuids::uuid dev,
            std::string,
            std::uint16_t,
            std::uint8_t,
            cyng::param_map_t);

        /**
         * pty.register
         */
        void pty_deregister(boost::uuids::uuid, std::string);

        /**
         * pty.open.channel
         */
        void pty_open_channel(
            boost::uuids::uuid,
            boost::uuids::uuid,
            std::string,
            std::string,
            std::string,
            std::string,
            std::string,
            std::chrono::seconds,
            cyng::param_map_t);

        /**
         * pty.close.channel
         */
        void pty_close_channel(boost::uuids::uuid, boost::uuids::uuid, std::uint32_t, cyng::param_map_t);

        /**
         * pty.push.data.req
         */
        void pty_push_data_req(
            boost::uuids::uuid,
            boost::uuids::uuid,
            std::uint32_t,
            std::uint32_t,
            cyng::buffer_t,
            cyng::param_map_t);

        /**
         * pty.stop session
         */
        void pty_stop(std::string, cyng::key_t);

        /** @brief cfg.init.backup session
         *
         * initialize a backup process
         */
        void cfg_backup_init(std::string, cyng::key_t, std::chrono::system_clock::time_point);

        void
        cfg_backup_merge(boost::uuids::uuid tag, cyng::buffer_t gw, cyng::buffer_t meter, cyng::obis_path_t, cyng::object value);

        void cfg_backup_finish(boost::uuids::uuid tag, cyng::buffer_t gw, std::chrono::system_clock::time_point);

        void
        cfg_sml_channel(bool, cyng::vector_t, std::string, cyng::obis, cyng::param_map_t, boost::uuids::uuid, boost::uuids::uuid);

        /**
         * send data to cluster node
         */
        void cluster_send_msg(std::deque<cyng::buffer_t>);

        /**
         * update node status
         */
        void send_ping_request();

        /**
         * emit a test message
         */
        void cluster_test_msg(std::string);

#ifdef _DEBUG
        void send_test_msg_to_setup(std::string);
#endif

      private:
        boost::asio::ip::tcp::socket socket_;
        db &cache_;
        cyng::controller &ctl_;
        cyng::logger logger_;

        /**
         * Buffer for incoming data.
         */
        std::array<char, 2048> buffer_;

        /**
         * Buffer for outgoing data.
         */
        std::deque<cyng::buffer_t> buffer_write_;

        cyng::vm_proxy vm_;
        cyng::io::parser parser_;

        /**
         * database listener
         */
        cyng::slot_ptr slot_;

        /**
         * remote session tag
         */
        boost::uuids::uuid peer_;
        std::string protocol_layer_;

        /**
         * Generate dependend keys for table "gwIEC"
         */
        config::dependend_key dep_key_;

        /**
         * ping task to update node status
         */
        cyng::channel_ptr ping_;
    };

    /**
     *  Round up to 5 minutes
     */
    std::chrono::seconds smooth(std::chrono::seconds interval);

    /**
     * Try to detect the server ID
     */
    //std::string detect_server_id(std::string const &);

    std::pair <std::string, bool> is_custom_gateway(std::string const &);

} // namespace smf

#endif
