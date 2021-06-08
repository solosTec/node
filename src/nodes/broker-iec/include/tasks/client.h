/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_TASK_CLIENT_H
#define SMF_DASH_TASK_CLIENT_H

#include <db.h>

#include <smf/cluster/bus.h>
#include <smf/iec/parser.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/store/key.hpp>
#include <cyng/task/task_fwd.h>

namespace smf {

    class client {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::string, std::string, std::chrono::seconds)>,
            std::function<void(std::string, cyng::key_t)>, //  add
            std::function<void(cyng::eod)>>;

        enum class state {
            START,
            WAIT,
            CONNECTED,
            STOPPED,
        } state_;

        struct meter_state {
            meter_state(std::string id, cyng::key_t);
            std::string const id_;
            cyng::key_t const key_;
        };

        /**
         * Manage multiple meters at one gateway.
         * Collect some statistics
         */
        class meter_mgr {

          public:
            meter_mgr();

            /**
             * @return current meter id
             */
            std::string get_id() const;
            cyng::key_t get_key() const;

            /**
             * set index to start
             */
            void reset();

            /**
             * @return true if at last position
             */
            bool next();
            bool is_complete() const;
            void add(std::string, cyng::key_t key);
            std::uint32_t size() const;
            std::uint32_t index() const;
            void loop(std::function<void(meter_state const &)>);

          private:
            std::vector<meter_state> meters_;
            std::uint32_t index_;
        };

      public:
        client(
            cyng::channel_weak,
            cyng::controller &,
            bus &,
            cyng::logger,
            std::filesystem::path,
            cyng::key_t key, //  "gwIEC"
            std::uint32_t connect_counter,
            std::uint32_t failure_counter);

      private:
        void start(std::string, std::string, std::chrono::seconds);
        void add_meter(std::string, cyng::key_t);
        void stop(cyng::eod);

        void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
        void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void
        handle_connect(const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
        void do_read();
        void handle_read(const boost::system::error_code &ec, std::size_t n);
        void do_write();
        void handle_write(const boost::system::error_code &ec);

        /**
         * send ice query for
         *
         * @param id meter id
         */
        void send_query(std::string id);

        constexpr bool is_stopped() const { return state_ == state::STOPPED; }
        constexpr bool is_connected() const { return state_ == state::CONNECTED; }

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::controller &ctl_;
        bus &bus_;
        cyng::logger logger_;

        cyng::key_t key_gw_iec_; //  "gwIEC"
        std::uint32_t connect_counter_;
        std::uint32_t failure_counter_;

        meter_mgr mgr_;
        boost::asio::ip::tcp::resolver::results_type endpoints_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::io_context::strand dispatcher_;
        std::deque<cyng::buffer_t> buffer_write_;
        std::array<char, 2048> input_buffer_;

        iec::parser parser_;
        cyng::channel_ptr writer_;
        std::size_t entries_; //	per readount
    };

    /**
     * generate a query string (clearing list) in the format "/?METER!\r\n"
     *
     * "/2!\r\n" special list on elster Annnn meters
     * "/3!\r\n" table 3 on EMH LZQJ-XC meters
     * "/4!\r\n" service on EMH LZQJ-XC meters
     */
    cyng::buffer_t generate_query(std::string meter);
} // namespace smf

#endif
