/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_H
#define SMF_IPT_BUS_H

#include <smf/ipt/config.h>
#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>

#include <cyng/log/logger.h>
#include <cyng/net/client_proxy.h>

namespace smf {
    namespace ipt {

        //  channel + source
        using channel_id = std::pair<std::uint32_t, std::uint32_t>;

        /**
         * @brief implementation of an ip-t client.
         *
         * Some effort is made to manage the internal state. It's important that
         * the state can be controlled from the outside and that the state is available
         * even after the bus object is expired. When an asynchronous socket operation
         * was cancelled the callback function has to figure out, what's next. And here
         * is a valid state information necessary.
         */
        class bus {
            /**
             * managing state of the bus
             */
            enum class state {
                INITIAL,
                CONNECTED,
                AUTHORIZED,
                LINKED,
                STOPPED,
            } state_;

            /**
             * Use this to store a (target) name - channel relation
             */
            using name_target_rel_t = std::pair<std::string, cyng::channel_weak>;
            using name_channel_rel_t = std::pair<push_channel, cyng::channel_weak>;
            using id_channel_rel_t = std::pair<std::uint32_t, cyng::channel_weak>;

          public:
            //	call back to signal changed IP-T authorization state
            using auth_cb = std::function<void(bool, boost::asio::ip::tcp::endpoint, boost::asio::ip::tcp::endpoint)>;

          public:
            bus(cyng::controller &ctl,
                cyng::logger,
                toggle::server_vec_t &&,
                std::string model,
                parser::command_cb,
                parser::data_cb,
                auth_cb);

            ~bus() = default;

            void start();
            void stop();

            /**
             * @return true if bus is authorized or in link state
             */
            bool is_authorized() const;

            /**
             * Initiate a sequence to register a push target.
             * If registration fails this task will be closed.
             *
             * @return true if process was started
             */
            bool register_target(std::string, cyng::channel_weak);

            /**
             * open a push channel (TP_RES_OPEN_CONNECTION)
             * Requires a methode "on.channel.open" to receive the response
             *
             * @return true if process was started
             */
            bool open_channel(push_channel, cyng::channel_weak);

            /**
             * close push channel (TP_RES_CLOSE_PUSH_CHANNEL)
             */
            bool close_channel(std::uint32_t channel, cyng::channel_weak);

            /**
             * send push data over specified channel
             */
            void transmit(std::pair<std::uint32_t, std::uint32_t> id, cyng::buffer_t data);

            /**
             * send data over an open connection
             */
            void transfer(cyng::buffer_t &&);

            /**
             * Send a watchdog reqponse (without a request)
             * This function is threadsafe.
             */
            bool send_watchdog();

          private:
            void reset();

            void cmd_complete(header const &, cyng::buffer_t &&);

            void res_login(cyng::buffer_t &&);
            void res_register_target(header const &, cyng::buffer_t &&);

            void pushdata_transfer(header const &, cyng::buffer_t &&);

            void req_watchdog(header const &, cyng::buffer_t &&);

            void open_connection(std::string, sequence_t);
            void close_connection(sequence_t);

            void res_open_push_channel(header const &, cyng::buffer_t &&);
            void res_close_push_channel(header const &, cyng::buffer_t &&);

          private:
            cyng::controller &ctl_;
            cyng::logger logger_;
            toggle tgl_;
            cyng::net::client_proxy client_;
            std::string const model_;
            parser::command_cb cb_cmd_;
            auth_cb cb_auth_;

            serializer serializer_;
            parser parser_;

            /**
             * Temporary table to hold a "register target" request.
             * After receiving a response the stored callback will be moved
             * to the channel => callback map.
             */
            std::map<ipt::sequence_t, name_target_rel_t> pending_targets_;
            std::map<std::uint32_t, name_target_rel_t> targets_;

            /**
             * Maps the sequence id to the channel/target name
             */
            std::map<ipt::sequence_t, name_channel_rel_t> opening_channel_;
            std::map<std::uint32_t, name_channel_rel_t> channels_;
            std::map<ipt::sequence_t, id_channel_rel_t> closing_channels_;

            boost::asio::ip::tcp::endpoint local_endpoint_;
            boost::asio::ip::tcp::endpoint remote_endpoint_;

            /**
             * watchdog task
             */
            cyng::channel_ptr wd_;
        };

        /**
         * test if channel id is null
         */
        bool is_null(channel_id const &);
        void init(channel_id &);

        constexpr std::uint32_t get_channel(channel_id const &id) { return id.first; }
        constexpr std::uint32_t get_source(channel_id const &id) { return id.second; }

        /**
         * combine two u32 integers to onw u64 integer with the channel as most significant word
         * and source as least significant word.
         */
        constexpr std::uint64_t combine(std::uint32_t channel, std::uint32_t source) {
            return static_cast<uint64_t>(channel) << 32 | source;
        }
        constexpr std::uint64_t combine(channel_id const &id) { return combine(get_channel(id), get_source(id)); }

    } // namespace ipt
} // namespace smf

#endif
