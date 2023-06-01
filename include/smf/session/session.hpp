/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_SESSION_HPP
#define SMF_SESSION_HPP

#include <smf/session/tasks/gatekeeper.hpp>

#include <smf/cluster/bus.h>

#include <cyng/obj/container_factory.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <array>
#include <cstdint>
#include <deque>
#include <memory>

#include <boost/uuid/nil_generator.hpp>

#if defined(_DEBUG_SESSION) || defined(_DEBUG_IPT)
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {

    /**
     * @tparam P parser
     * @tparam T implementation class
     * @tparam N receive buffer size
     */
    template <typename P, typename S, typename T, std::size_t N>
    //  generic base class for SMF sessions (client side)
    class session : public std::enable_shared_from_this<T> {
      public:
        using parser_t = P;
        using serializer_t = S;
        using impl_t = T;
        using gatekeeper_t = gatekeeper<impl_t>;

      public:
        session(
            boost::asio::ip::tcp::socket socket,
            bus &cluster_bus,
            cyng::mesh &fabric,
            cyng::logger logger,
            parser_t &&parser,
            serializer_t &&serializer)
            : read_buffer_{0}
            , write_buffer_{}
            , rx_{0}
            , sx_{0}
            , socket_(std::move(socket))
            , bus_(cluster_bus)
            , ctl_(fabric.get_ctl())
            , logger_(logger)
            , dev_(boost::uuids::nil_uuid())
            , vm_()
            , gatekeeper_()
            , parser_(std::move(parser))
            , serializer_(std::move(serializer)) {}

        virtual ~session() {
            if (gatekeeper_) {
                gatekeeper_->stop();
            }
        }

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

        void close_socket() {
            //	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
            socket_.close(ec);
        }

        /**
         * start reading from socket
         */
        void start(std::chrono::seconds timeout) {

            do_read();
            gatekeeper_ = ctl_.create_channel_with_ref<gatekeeper_t>(logger_, this->shared_from_this(), bus_).first;
            BOOST_ASSERT(gatekeeper_->is_open());
            CYNG_LOG_TRACE(logger_, "start gatekeeper with a timeout of " << timeout.count() << " seconds");
            gatekeeper_->suspend(
                timeout,
                "timeout",
                //  handle dispatch errors
                std::bind(&bus::log_dispatch_error, &bus_, std::placeholders::_1, std::placeholders::_2));
        }

        /**
         * Produce data that will be sent to device.
         * This function is thread safe regarding the write buffer and
         * the production of data.
         */
        void send(std::function<cyng::buffer_t()> f) {
            cyng::exec(vm_, [this, f]() {
                bool const b = write_buffer_.empty();
                write_buffer_.push_back(f());
                if (b) {
                    do_write();
                }
            });
        }

      private:
        void do_read() {
            auto self = this->shared_from_this();

            socket_.async_read_some(
                boost::asio::buffer(read_buffer_.data(), read_buffer_.size()),
                [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                    if (!ec) {
                        CYNG_LOG_DEBUG(
                            logger_,
                            "[session] " << vm_.get_tag() << " received " << bytes_transferred << " bytes from ["
                                         << socket_.remote_endpoint() << "]");

#if defined(_DEBUG_SESSION) || defined(_DEBUG_IPT)
                        {
                            std::stringstream ss;
                            cyng::io::hex_dump<8> hd;
                            hd(ss, read_buffer_.data(), read_buffer_.data() + bytes_transferred);
                            auto const dmp = ss.str();

                            CYNG_LOG_DEBUG(
                                logger_,
                                "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes:\n"
                                    << dmp);
                        }
#endif
                        //
                        //	let it parse
                        //
                        parser_.read(read_buffer_.data(), read_buffer_.data() + bytes_transferred);

                        //
                        //	update rx
                        //
                        rx_ += static_cast<std::uint64_t>(bytes_transferred);
                        if (!dev_.is_nil() && bus_.is_connected()) {

                            bus_.req_db_update("session", cyng::key_generator(dev_), cyng::param_map_factory()("rx", rx_));
                        }

                        //
                        //	continue reading
                        //
                        do_read();
                    } else {
                        CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " read: " << ec.message());
                        write_buffer_.clear();
                        close_socket();
                    }
                });
        }

        void do_write() {

            // Start an asynchronous operation to send a heartbeat message.
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(write_buffer_.front().data(), write_buffer_.front().size()),
                cyng::expose_dispatcher(vm_).wrap(
                    std::bind(&session::handle_write, this, std::placeholders::_1, std::placeholders::_2)));
        }

        void handle_write(const boost::system::error_code &ec, std::size_t n) {

            if (!ec) {

                //
                //	update sx
                //
                sx_ += static_cast<std::uint64_t>(n);
                //                sx_ += static_cast<std::uint64_t>(write_buffer_.front().size());
                if (!dev_.is_nil() && bus_.is_connected()) {

                    bus_.req_db_update("session", cyng::key_generator(dev_), cyng::param_map_factory()("sx", sx_));
                }

                write_buffer_.pop_front();
                if (!write_buffer_.empty()) {
                    do_write();
                }
            } else {
                CYNG_LOG_ERROR(logger_, "[session] " << vm_.get_tag() << " write: " << ec.message());
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
         * statistics
         */
        std::uint64_t rx_, sx_;

        /**
         * TCP/IP socket
         */
        boost::asio::ip::tcp::socket socket_;

      protected:
        /**
         * cluster bus
         */
        bus &bus_;

        cyng::controller &ctl_;
        cyng::logger logger_;

        /**
         * tag/pk of device
         */
        boost::uuids::uuid dev_;

        /**
         * session VM
         */
        cyng::vm_proxy vm_;

        /**
         * gatekeeper
         */
        cyng::channel_ptr gatekeeper_;

        /**
         * parser for session data
         */
        parser_t parser_;

        /**
         * serializer for session data
         */
        serializer_t serializer_;
    };
} // namespace smf

#endif
