/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_BROKER_H
#define SMF_SEGW_TASK_BROKER_H

#include <cfg.h>
#include <config/cfg_broker.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/task_fwd.h>

namespace smf {

    /**
     * connect to MDM system
     * https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/example/cpp03/timeouts/async_tcp_client.cpp
     * Data is buffered (Fifo)
     * Connection is established
     * If during the connection establishment further data come, they are buffered
     * If the connection is open, data are sent
     * If the connection cannot be opened, buffer is deleted
     * If the remote station closes the connection, buffer is deleted
     */
    class broker {
        template <typename T> friend class cyng::task;

        using signatures_t = std::
            tuple<std::function<void(cyng::buffer_t)>, std::function<void(std::chrono::seconds)>, std::function<void(cyng::eod)>>;

        enum class state {
            START,
            WAIT,
            CONNECTED,
            STOPPED,
        } state_;

      public:
        broker(cyng::channel_weak, cyng::controller &ctl, cyng::logger, target const &, bool login);

      private:
        void stop(cyng::eod);
        void start();

        /**
         * incoming raw data from serial interface
         */
        void receive(cyng::buffer_t);

        void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
        void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void
        handle_connect(const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void do_read();
        void handle_read(const boost::system::error_code &ec, std::size_t n);
        void do_write();
        void handle_write(const boost::system::error_code &ec);
        void check_status(std::chrono::seconds);

        constexpr bool is_stopped() const { return state_ == state::STOPPED; }
        constexpr bool is_connected() const { return state_ == state::CONNECTED; }

        void reset(state);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        target const target_;
        bool const login_;

        boost::asio::ip::tcp::resolver::results_type endpoints_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::io_context::strand dispatcher_;

        std::array<char, 2048> read_buffer_;
        std::deque<cyng::buffer_t> write_buffer_;
    };

    /**
     * plantuml:
        @startuml
        [*] --> OFFLINE
        OFFLINE--> [*]
        OFFLINE: disconnected
        OFFLINE: empty cache

        OFFLINE --> CONNECTING : incoming data
        CONNECTING: buffering data

        CONNECTING --> CONNECTED : TCP/IP connect

        CONNECTING --> OFFLINE : connect failed\n clear cache

        CONNECTED --> OFFLINE: disconnect\n clear cache
        CONNECTED: send data from cache
        CONNECTED: forward incoming data
        @enduml
     */
    class broker_on_demand {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<std::function<void(cyng::buffer_t)>, std::function<void(cyng::eod)>>;

        enum class state_value { OFFLINE, CONNECTING, CONNECTED, STOPPED };

        struct state : std::enable_shared_from_this<state> {
            state(boost::asio::ip::tcp::resolver::results_type &&);
            constexpr bool is_stopped() const { return has_state(state_value::STOPPED); }
            constexpr bool is_connected() const { return has_state(state_value::CONNECTED); }
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
        broker_on_demand(cyng::channel_weak, cyng::controller &ctl, cyng::logger, target const &, bool login);

      private:
        void stop(cyng::eod);
        void start();
        void reset(state_ptr sp, state_value);

        /**
         * incoming raw data from serial interface
         */
        void receive(cyng::buffer_t);

        void store(cyng::buffer_t);
        void send(state_ptr sp, cyng::buffer_t);

        void do_write(state_ptr sp);
        void handle_write(state_ptr sp, boost::system::error_code const &ec);
        void connect(state_ptr sp);
        void start_connect(state_ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void handle_connect(
            state_ptr sp,
            const boost::system::error_code &ec,
            boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void do_read(state_ptr sp);
        void handle_read(state_ptr sp, const boost::system::error_code &ec, std::size_t n);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        target const target_;
        bool const login_;

        // boost::asio::ip::tcp::resolver::results_type endpoints_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::io_context::strand dispatcher_;

        std::array<char, 2048> read_buffer_;
        std::deque<cyng::buffer_t> write_buffer_;
    };
} // namespace smf

#endif
