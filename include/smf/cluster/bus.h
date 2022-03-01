/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CLUSTER_BUS_H
#define SMF_CLUSTER_BUS_H

#include <smf/cluster/config.h>
#include <smf/cluster/features.h>

#include <cyng/io/parser/parser.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/store/key.hpp>
#include <cyng/vm/proxy.h>
#include <cyng/vm/vm_fwd.h>

#include <chrono>

namespace smf {

    class cfg_interface {
      public:
        /** @brief configuration management:
         *
         * Backup/update a gw/meter configuration record
         */
        virtual void
        cfg_merge(boost::uuids::uuid tag, cyng::buffer_t gw, cyng::buffer_t meter, cyng::obis_path_t, cyng::object value) = 0;

        /** @brief configuration management:
         *
         * Backup congig data is complete. Write meta data
         */
        virtual void
        cfg_finish(boost::uuids::uuid tag, cyng::buffer_t gw, std::chrono::system_clock::time_point) = 0;
    };

    /**
     * The bus interface defines the requirements of any kind of cluster
     * member that acts as a client.
     */
    class bus_client_interface {
      public:
        virtual auto get_fabric() -> cyng::mesh * = 0;

        virtual void on_login(bool) = 0;

        /**
         * connection to cluster lost
         *
         * @param msg system message
         */
        virtual void on_disconnect(std::string msg) = 0;

        virtual void db_res_insert(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) = 0;

        virtual void db_res_trx(std::string, bool) = 0;

        virtual void db_res_update(std::string, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) = 0;

        virtual void db_res_remove(std::string, cyng::key_t key, boost::uuids::uuid tag) = 0;

        virtual void db_res_clear(std::string, boost::uuids::uuid tag) = 0;

        /**
         * To return a null pointer is allowed.
         */
        virtual cfg_interface *get_cfg_interface() = 0;
    };

    /**
     * The TCP/IP behaviour is modelled after the async_tcp_client.cpp" example
     * in the asio C++11 example folder (libs/asio/example/cpp11/timeouts/async_tcp_client.cpp)
     */
    class bus {
        enum class state_value {
            START,
            WAIT,
            CONNECTED,
            STOPPED,
        };

        struct state : std::enable_shared_from_this<state> {
            state(boost::asio::ip::tcp::resolver::results_type &&);
            constexpr auto is_connected() const -> bool { return has_state(state_value::CONNECTED); }
            constexpr auto is_stopped() const -> bool { return has_state(state_value::STOPPED); }
            constexpr bool has_state(state_value s) const { return s == value_; }

            state_value value_;
            boost::asio::ip::tcp::resolver::results_type endpoints_;
        };
        using state_ptr = std::shared_ptr<state>;
        /**
         * helps to control state from the outside without
         * to establish an additional reference to the state object.
         */
        state_ptr state_holder_;

      public:
        bus(boost::asio::io_context &ctx,
            cyng::logger,
            toggle::server_vec_t &&cfg,
            std::string const &node_name,
            boost::uuids::uuid tag,
            std::uint32_t features,
            bus_client_interface *bip);

        void start();
        void stop();

        auto get_tag() const -> boost::uuids::uuid;

        auto is_connected() const -> bool;

        //
        //	cluster client functions
        //
        void req_subscribe(std::string table_name);
        void req_db_insert(std::string const &table_name, cyng::key_t key, cyng::data_t data, std::uint64_t generation);

        /**
         * insert data in table with auto-increment
         */
        void req_db_insert_auto(std::string const &table_name, cyng::data_t data);

        /**
         * triggers a merge() on the receiver side
         */
        void req_db_update(std::string const &, cyng::key_t key, cyng::param_map_t data);

        void req_db_remove(std::string const &, cyng::key_t);

        void req_db_clear(std::string const &);

        //	"pty.req.login"
        void pty_login(
            std::string name,
            std::string pwd,
            boost::uuids::uuid tag,
            std::string data_layer,
            boost::asio::ip::tcp::endpoint ep);

        //	"pty.req.logout"
        void pty_logout(boost::uuids::uuid dev, boost::uuids::uuid tag);

        //	"pty.open.connection"
        void pty_open_connection(std::string msisdn, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token);

        //	"pty.res.open.connection"
        void pty_res_open_connection(
            bool,
            // boost::uuids::uuid peer,
            boost::uuids::uuid dev,    //	callee dev-tag
            boost::uuids::uuid callee, //	callee vm-tag
            cyng::param_map_t &&token);

        //	"pty.close.connection"
        void pty_close_connection(boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token);

        //	"pty.transfer.data"
        void pty_transfer_data(boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::buffer_t &&);

        //	"pty.register.target"
        void pty_reg_target(
            std::string name,
            std::uint16_t,
            std::uint8_t,
            boost::uuids::uuid dev,
            boost::uuids::uuid tag,
            cyng::param_map_t &&token);
        //	"pty.deregister"
        void pty_dereg_target(std::string name, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token);

        //	"pty.open.channel"
        void pty_open_channel(
            std::string name,
            std::string account,
            std::string msisdn,
            std::string version,
            std::string id,
            std::chrono::seconds timeout,
            boost::uuids::uuid dev,
            boost::uuids::uuid tag,
            cyng::param_map_t &&token);

        //	"pty.close.channel"
        void pty_close_channel(std::uint32_t channel, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token);

        //	"pty.push.data"
        void pty_push_data(
            std::uint32_t channel,
            std::uint32_t source,
            cyng::buffer_t data,
            boost::uuids::uuid dev,
            boost::uuids::uuid tag,
            cyng::param_map_t &&token);

        //	"pty.stop"
        void pty_stop(std::string const &, cyng::key_t);

        /**
         * push system message
         * "sys.msg"
         */
        template <typename Head, typename... Args> void sys_msg(cyng::severity level, Head &&v, Args &&...args) {
            std::stringstream ss;
            ss << v;
            ((ss << ' ' << args), ...);
            push_sys_msg(ss.str(), level);
        }
        void push_sys_msg(std::string, cyng::severity level);

        /**
         * send an update request for "cluster" table to main node
         */
        void update_pty_counter(std::uint64_t);

        //	"cfg.init.backup"
        void cfg_init_backup(std::string const &, cyng::key_t, std::chrono::system_clock::time_point);

        /** @brief configuration management
         *
         * @return true if configuration management is available
         */
        bool has_cfg_management() const;

        /** @brief configuration management
         *
         * Backup/update a gw/meter configuration record
         */
        void
        cfg_merge_backup(boost::uuids::uuid tag, cyng::buffer_t gw, cyng::buffer_t meter, cyng::obis_path_t, cyng::object value);

        /** @brief configuration management
         *
         *  Finish a set of configuration data
         */
        void cfg_finish_backup(boost::uuids::uuid tag, cyng::buffer_t gw, std::chrono::system_clock::time_point now);

      private:
        void reset(state_ptr sp, state_value);
        void connect(state_ptr sp);
        void start_connect(state_ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void handle_connect(
            state_ptr sp,
            boost::system::error_code const &,
            boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void do_read(state_ptr sp);
        void do_write(state_ptr sp);
        void handle_read(state_ptr sp, boost::system::error_code const &, std::size_t n);
        void handle_write(state_ptr sp, boost::system::error_code const &);
        void reconnect_timeout(state_ptr sp, boost::system::error_code const &);

        void set_reconnect_timer(std::chrono::seconds);

        void add_msg(state_ptr, std::deque<cyng::buffer_t> &&);

        /**
         * insert external functions
         */
        auto init_vm(bus_client_interface *) -> cyng::vm_proxy;

        void on_ping(std::chrono::system_clock::time_point);

        void on_test_msg(std::string);

        // void cfg_merge_backup(boost::uuids::uuid);

      private:
        boost::asio::io_context &ctx_;
        cyng::logger logger_;
        toggle const tgl_;
        std::string const node_name_;
        boost::uuids::uuid const tag_;
        std::uint32_t const features_;
        bus_client_interface *bip_;

        boost::asio::ip::tcp::socket socket_;
        boost::asio::steady_timer timer_;
        std::deque<cyng::buffer_t> buffer_write_;
        std::array<char, 2048> input_buffer_;
        cyng::vm_proxy vm_;
        cyng::io::parser parser_;
    };
} // namespace smf

#endif
