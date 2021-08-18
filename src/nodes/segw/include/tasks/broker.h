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

    namespace state {
        enum class value { OFFLINE, CONNECTING, CONNECTED, STOPPED };

        struct mgr : std::enable_shared_from_this<mgr> {
            mgr(boost::asio::ip::tcp::resolver::results_type &&);
            constexpr bool is_stopped() const { return has_state(value::STOPPED); }
            constexpr bool is_connected() const { return has_state(value::CONNECTED); }
            constexpr bool has_state(value s) const { return s == value_; }

            value value_;
            boost::asio::ip::tcp::resolver::results_type endpoints_;
        };

        using ptr = std::shared_ptr<mgr>;

    } // namespace state

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

        /**
         * helps to control state from the outside without
         * to establish an additional reference to the state object.
         */
        state::ptr state_holder_;

      public:
        broker(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cfg &, lmn_type type, std::size_t idx);

      private:
        void stop(cyng::eod);
        void start();

        /**
         * incoming raw data from serial interface
         */
        void receive(cyng::buffer_t);

        void connect(state::ptr sp);
        void start_connect(state::ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void handle_connect(
            state::ptr sp,
            const boost::system::error_code &ec,
            boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void do_read(state::ptr sp);
        void handle_read(state::ptr sp, const boost::system::error_code &ec, std::size_t n);
        void do_write(state::ptr sp);
        void handle_write(state::ptr sp, const boost::system::error_code &ec);
        void check_status(std::chrono::seconds);

        void reset(state::ptr sp, state::value);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        cfg_broker cfg_;
        std::size_t const index_;

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

        using signatures_t = std::tuple<
            std::function<void(cyng::buffer_t)>, // receive()
            std::function<void()>,               // close_connection()
            std::function<void(cyng::eod)>       // stop()
            >;

        /**
         * helps to control state from the outside without
         * to establish an additional reference to the state object.
         */
        state::ptr state_holder_;

      public:
        broker_on_demand(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cfg &, lmn_type type, std::size_t idx);

      private:
        void stop(cyng::eod);
        void start();
        void reset(state::ptr sp, state::value);

        /**
         * incoming raw data from serial interface
         */
        void receive(cyng::buffer_t);

        /**
         * If write buffer is empty an open connection
         * will be closed.
         */
        void close_connection();

        void store(cyng::buffer_t);
        void send(state::ptr sp, cyng::buffer_t);

        void do_write(state::ptr sp);
        void handle_write(state::ptr sp, boost::system::error_code const &ec, std::size_t n);
        void connect(state::ptr sp);
        void start_connect(state::ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void handle_connect(
            state::ptr sp,
            const boost::system::error_code &ec,
            boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void do_read(state::ptr sp);
        void handle_read(state::ptr sp, const boost::system::error_code &ec, std::size_t n);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        cyng::logger logger_;
        cfg_broker cfg_;
        std::size_t const index_;

        boost::asio::ip::tcp::socket socket_;
        boost::asio::io_context::strand dispatcher_;

        std::array<char, 2048> read_buffer_;
        std::deque<cyng::buffer_t> write_buffer_;
    };
} // namespace smf

#endif
