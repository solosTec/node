/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_SESSION_H
#define SMF_IPT_SESSION_H

#include <proxy.h>
#include <smf/cluster/bus.h>
#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>
#ifdef _DEBUG_IPT
#include <smf/sml/unpack.h>
#endif

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/task/controller.h>
#include <cyng/vm/proxy.h>
#include <cyng/vm/vm_fwd.h>

#include <array>
#include <memory>

namespace smf {

    class proxy;
    class ipt_session : public std::enable_shared_from_this<ipt_session> {
        friend class proxy;

      public:
        ipt_session(
            boost::asio::ip::tcp::socket socket,
            cyng::logger logger,
            bus &cluster_bus,
            cyng::mesh &fabric,
            ipt::scramble_key const &sk,
            std::uint32_t query,
            cyng::mac48 client_id);
        ~ipt_session();

        void start(std::chrono::seconds timeout);
        void stop();
        void logout();

        boost::asio::ip::tcp::endpoint get_remote_endpoint() const;

      private:
        void do_read();
        void do_write();
        void handle_write(const boost::system::error_code &ec);

        //
        //	bus interface
        //
        void ipt_cmd(ipt::header const &h, cyng::buffer_t &&body);
        void ipt_stream(cyng::buffer_t &&data);

        /**
         * start an async write
         * @param f to provide a function that produces a buffer instead of the buffer itself
         * guaranties that scrambled content is has the correct index in the scramble key.
         */
        void ipt_send(std::function<cyng::buffer_t()> f);

        void pty_res_login(bool success, boost::uuids::uuid dev);
        void pty_res_register(bool success, std::uint32_t channel, cyng::param_map_t token);
        void pty_res_open_channel(
            bool success,
            std::uint32_t channel,
            std::uint32_t source,
            std::uint16_t packet_size,
            std::uint8_t window_size,
            std::uint8_t status,
            std::uint32_t count,
            cyng::param_map_t token);

        void pty_req_push_data(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data);

        void pty_res_push_data(bool success, std::uint32_t channel, std::uint32_t source, cyng::param_map_t token);

        void pty_res_close_channel(bool success, std::uint32_t channel, std::size_t count, cyng::param_map_t /*token*/);

        void pty_res_open_connection(bool success, cyng::param_map_t token);

        void pty_res_close_connection(bool success, cyng::param_map_t token);

        void pty_req_open_connection(std::string msisdn, bool local, cyng::param_map_t token);

        void pty_req_close_connection();

        void pty_transfer_data(cyng::buffer_t data);

        void pty_stop();

        void cfg_backup(std::string, std::string, cyng::buffer_t, std::chrono::system_clock::time_point tp);

        /**
         * query some device data
         */
        void query();

        void update_software_version(std::string str);
        void update_device_identifier(std::string str);
        void register_target(std::string name, std::uint16_t paket_size, std::uint8_t window_size, ipt::sequence_t seq);

        void deregister_target(std::string name, ipt::sequence_t seq);

        void open_push_channel(
            std::string name,
            std::string account,
            std::string msisdn,
            std::string version,
            std::string id,
            std::uint16_t timeout,
            ipt::sequence_t /*seq*/);

        void close_push_channel(std::uint32_t channel, ipt::sequence_t seq);

        void pushdata_transfer(
            std::uint32_t channel,
            std::uint32_t source,
            std::uint8_t status,
            std::uint8_t block,
            cyng::buffer_t data,
            ipt::sequence_t seq);

        void open_connection(std::string msisdn, ipt::sequence_t seq);

        //#ifdef _DEBUG
        //        void debug_get_profile_list();
        //#endif

        static auto get_vm_func_pty_res_login(ipt_session *p) -> std::function<void(bool success, boost::uuids::uuid)>;

        static auto get_vm_func_pty_res_register(ipt_session *p)
            -> std::function<void(bool success, std::uint32_t, cyng::param_map_t)>;

        static auto get_vm_func_pty_res_open_channel(ipt_session *p) -> std::function<
            void(bool, std::uint32_t, std::uint32_t, std::uint16_t, std::uint8_t, std::uint8_t, std::uint32_t, cyng::param_map_t)>;

        static auto get_vm_func_pty_req_push_data(ipt_session *p)
            -> std::function<void(std::uint32_t, std::uint32_t, cyng::buffer_t)>;

        static auto get_vm_func_pty_res_push_data(ipt_session *p)
            -> std::function<void(bool, std::uint32_t, std::uint32_t, cyng::param_map_t)>;

        static auto get_vm_func_pty_res_close_channel(ipt_session *p)
            -> std::function<void(bool success, std::uint32_t channel, std::size_t count, cyng::param_map_t)>;

        static auto get_vm_func_pty_res_open_connection(ipt_session *p) -> std::function<void(bool success, cyng::param_map_t)>;

        static auto get_vm_func_pty_transfer_data(ipt_session *p) -> std::function<void(cyng::buffer_t)>;

        static auto get_vm_func_pty_res_close_connection(ipt_session *p) -> std::function<void(bool success, cyng::param_map_t)>;

        static auto get_vm_func_pty_req_open_connection(ipt_session *p)
            -> std::function<void(std::string, bool, cyng::param_map_t)>;

        static auto get_vm_func_pty_req_close_connection(ipt_session *p) -> std::function<void()>;

        static auto get_vm_func_pty_stop(ipt_session *p) -> std::function<void()>;

        static auto get_vm_func_cfg_backup(ipt_session *p)
            -> std::function<void(std::string, std::string, cyng::buffer_t, std::chrono::system_clock::time_point tp)>;

      private:
        cyng::controller &ctl_;
        boost::asio::ip::tcp::socket socket_;
        cyng::logger logger_;

        bus &cluster_bus_;
        std::uint32_t const query_;

        /**
         * Buffer for incoming data.
         */
        std::array<char, 2048> buffer_;
        std::uint64_t rx_, sx_, px_;

        /**
         * Buffer for outgoing data.
         */
        std::deque<cyng::buffer_t> buffer_write_;

        /**
         * parser for ip-t data
         */
        ipt::parser parser_;

        /**
         * serializer for ip-t data
         */
        ipt::serializer serializer_;

        /**
         * process cluster commands
         */
        cyng::vm_proxy vm_;

        /**
         * tag/pk of device
         */
        boost::uuids::uuid dev_;

        /**
         * store temporary data during connection establishment
         */
        std::map<ipt::sequence_t, cyng::param_map_t> oce_map_;

        /**
         * gatekeeper
         */
        cyng::channel_ptr gatekeeper_;

        /**
         * proxy
         */
        proxy proxy_;

#ifdef _DEBUG_IPT
        sml::unpack sml_parser_;
#endif
    };

} // namespace smf

#endif
