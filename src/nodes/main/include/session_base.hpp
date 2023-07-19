/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_SESSION_BASE_HPP
#define SMF_MAIN_SESSION_BASE_HPP

#include <tasks/ping.hpp>

#include <cyng/log/logger.h> // log_dispatch_error()
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/add.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/proxy.h>

#include <array>
#include <memory>

namespace smf {

    /**
     * @tparam P parser
     * @tparam T implementation class
     * @tparam N receive buffer size
     */
    template <typename P, typename T, std::size_t N>
    //  generic base class for SMF sessions (server side)
    class session_base : public std::enable_shared_from_this<T> {

      public:
        using parser_t = P;
        using impl_t = T;
        using ping_t = ping<T>;

      public:
        session_base(boost::asio::ip::tcp::socket socket, cyng::mesh &fabric, cyng::logger logger, parser_t &&parser)
            : read_buffer_{0}
            , write_buffer_{}
            , socket_(std::move(socket))
            , ctl_(fabric.get_ctl())
            , logger_(logger)
            , vm_()
            , ping_()
            , parser_(std::move(parser)) {}

        ~session_base() = default;

        /**
         * @return remote endpoint
         * @throw if socket is closed
         */
        boost::asio::ip::tcp::endpoint get_remote_endpoint() const {
            try {
                return socket_.remote_endpoint();
            } catch (...) {
            }
            return boost::asio::ip::tcp::endpoint{};
        }

        boost::asio::ip::tcp::endpoint get_local_endpoint() const {
            try {
                return socket_.local_endpoint();
            } catch (...) {
            }
            return boost::asio::ip::tcp::endpoint{};
        }

        /**
         * @return tag of remote session
         */
        boost::uuids::uuid get_peer() const { return vm_.get_tag(); }

        void close_socket() {
            //	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
            socket_.close(ec);
        }

        /**
         * start reading from socket
         */
        void start() {

            do_read();

            //
            //	start ping
            //
            ping_ = ctl_.create_channel_with_ref<ping_t>(logger_, this->shared_from_this()).first;
            BOOST_ASSERT(ping_->is_open());
            ping_->suspend(
                std::chrono::minutes(1),
                "update",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));
        }

        void stop() {
            //
            //  stop depended ping task
            //
            ping_->stop();

            //
            //  close TCP/IP socket
            //
            close_socket();

            //
            //	stop VM
            //
            vm_.stop();
        }

        void send(std::deque<cyng::buffer_t> msg) {
            BOOST_ASSERT(!msg.empty());
            if (!msg.empty()) {
                cyng::exec(vm_, [=, this, data = std::move(msg)]() {
                    bool const b = write_buffer_.empty();
                    CYNG_LOG_DEBUG(logger_, "send " << data.size() << " bytes cluster message to peer " << vm_.get_tag());
                    cyng::add(write_buffer_, data);
                    if (b) {
                        do_write();
                    }
                });
            }
        }

      private:
        void do_read() {
            auto self = this->shared_from_this();

            socket_.async_read_some(
                boost::asio::buffer(read_buffer_.data(), read_buffer_.size()),
                [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                    if (!ec) {
                        CYNG_LOG_DEBUG(
                            logger_, "session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes");

                        //
                        //	let it parse
                        //
                        parser_.read(read_buffer_.begin(), read_buffer_.begin() + bytes_transferred);

                        //
                        //	continue reading
                        //
                        do_read();
                    } else {
                        CYNG_LOG_WARNING(logger_, "[session] read: " << ec.message());
                        //  ping holds a reference of this session and must be terminated.
                        if (ping_ && ping_->is_open()) {
                            ping_->stop();
                        }
                        CYNG_LOG_TRACE(logger_, "[session] use count: " << self.use_count());
                        write_buffer_.clear();
                        close_socket();
                    }
                });
        }

        void do_write() {
            BOOST_ASSERT(!write_buffer_.empty());

            //
            //	write actually data to socket
            //
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(write_buffer_.front().data(), write_buffer_.front().size()),
                cyng::expose_dispatcher(vm_).wrap(std::bind(&session_base::handle_write, this, std::placeholders::_1)));
        }

        void handle_write(const boost::system::error_code &ec) {
            if (!ec) {

                BOOST_ASSERT(!write_buffer_.empty());
                write_buffer_.pop_front();
                if (!write_buffer_.empty()) {
                    do_write();
                }
            } else {
                CYNG_LOG_ERROR(logger_, "[session] write: " << ec.message());
                write_buffer_.clear();
                close_socket();
            }
        }

      private:
        /**
         * Buffer for incoming data.
         */
        std::array<char, N> read_buffer_;

        /**
         * Buffer for outgoing data.
         */
        std::deque<cyng::buffer_t> write_buffer_;

        /**
         * TCP/IP socket
         */
        boost::asio::ip::tcp::socket socket_;

      protected:
        cyng::controller &ctl_;
        cyng::logger logger_;

        cyng::vm_proxy vm_;

        /**
         * ping task to update node status
         */
        cyng::channel_ptr ping_;

        /**
         * parser for session data
         */
        parser_t parser_;
    };

    /**
     *  Round up to 5 minutes
     */
    std::chrono::seconds smooth(std::chrono::seconds interval);

    /**
     * Try to detect the server ID
     */
    std::pair<std::string, bool> is_custom_gateway(std::string const &);

} // namespace smf

#endif
