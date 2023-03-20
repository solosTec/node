/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_SESSION_H
#define SMF_IPT_SESSION_H

#include <proxy.h>
#include <smf/session/session.hpp>
// #include <smf/cluster/bus.h>
#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>
#ifdef _DEBUG_IPT
#include <smf/sml/unpack.h>
#endif

namespace smf {

    class proxy;
    class ipt_session : public session<ipt::parser, ipt::serializer, ipt_session, 2048> {

        //  base class
        using base_t = session<ipt::parser, ipt::serializer, ipt_session, 2048>;

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
        virtual ~ipt_session();

        void stop();
        void logout();

      private:
        //
        //	bus interface
        //
        void ipt_cmd(ipt::header const &h, cyng::buffer_t &&body);
        void ipt_stream(cyng::buffer_t &&data);

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

        void cfg_req_backup(
            std::string,
            std::string,
            boost::uuids::uuid,
            cyng::buffer_t,
            std::string fw,
            std::chrono::system_clock::time_point tp);

        void cfg_sml_channel(
            std::string,        //  operator_name,
            std::string,        //  operator_pwd,
            boost::uuids::uuid, //  tag (device/session/gateway)
            cyng::buffer_t,     //  id
            std::string,        // channel
            cyng::obis,         // section
            cyng::param_map_t,  // params
            boost::uuids::uuid, // source
            boost::uuids::uuid  // cluster tag
        );

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

        void update_connect_state(bool connected);

      private:
        /**
         * @see ipt::query
         */
        std::uint32_t const query_;

        /**
         * push bytes counter
         */
        std::uint64_t px_;

        std::string name_; //  login name

        /**
         * store temporary data during connection establishment
         */
        std::map<ipt::sequence_t, cyng::param_map_t> oce_map_;

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
