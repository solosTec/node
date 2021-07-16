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
            enum class state_value {
                START,
                CONNECTED,
                AUTHORIZED,
                LINKED,
                STOPPED,
            };

            /**
             * state information has to be available even if the bus object
             * is destroyed.
             */
            struct state : std::enable_shared_from_this<state> {
                state(boost::asio::ip::tcp::resolver::results_type &&);
                constexpr bool is_authorized() const {
                    return (value_ == state_value::AUTHORIZED) || (value_ == state_value::LINKED);
                }
                constexpr bool is_stopped() const { return value_ == state_value::STOPPED; }
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
            bus(boost::asio::io_context &ctx,
                cyng::logger,
                toggle::server_vec_t &&,
                std::string model,
                parser::command_cb,
                parser::data_cb,
                auth_cb);

            ~bus();

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
            void transfer(cyng::buffer_t const &);

          private:
            void reset(state_ptr, state_value);
            void connect(state_ptr sp);
            void start_connect(state_ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
            void handle_connect(
                state_ptr sp,
                boost::system::error_code const &ec,
                boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
            void reconnect_timeout(state_ptr sp, boost::system::error_code const &);
            void do_read(state_ptr);
            void do_write(state_ptr);
            void handle_read(state_ptr, boost::system::error_code const &ec, std::size_t n);
            void handle_write(state_ptr, boost::system::error_code const &ec);

            /**
             * start an async write
             */
            void send(state_ptr, std::function<cyng::buffer_t()>);

            void cmd_complete(header const &, cyng::buffer_t &&);

            void res_login(cyng::buffer_t &&);
            void res_register_target(header const &, cyng::buffer_t &&);

            void pushdata_transfer(header const &, cyng::buffer_t &&);

            void req_watchdog(header const &, cyng::buffer_t &&);

            void open_connection(std::string, sequence_t);
            void close_connection(sequence_t);

            void set_reconnect_timer(std::chrono::seconds);

            void res_open_push_channel(header const &, cyng::buffer_t &&);
            void res_close_push_channel(header const &, cyng::buffer_t &&);

          private:
            boost::asio::io_context &ctx_;
            cyng::logger logger_;
            toggle tgl_;
            std::string const model_;
            parser::command_cb cb_cmd_;
            auth_cb cb_auth_;

            // boost::asio::ip::tcp::resolver::results_type endpoints_;
            boost::asio::ip::tcp::socket socket_;
            boost::asio::steady_timer timer_;
            boost::asio::io_context::strand dispatcher_;
            serializer serializer_;
            parser parser_;
            std::deque<cyng::buffer_t> buffer_write_;
            std::array<char, 2048> input_buffer_;

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
