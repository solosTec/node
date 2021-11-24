/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MODEM_SESSION_H
#define SMF_MODEM_SESSION_H

#include <smf/cluster/bus.h>
#include <smf/modem/parser.h>
#include <smf/modem/serializer.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/task/controller.h>
#include <cyng/vm/proxy.h>
#include <cyng/vm/vm_fwd.h>

#include <array>
#include <memory>

namespace smf {

    class modem_session : public std::enable_shared_from_this<modem_session> {
      public:
        modem_session(
            boost::asio::ip::tcp::socket socket,
            bus &cluster_bus,
            cyng::mesh &fabric,
            bool auto_answer,
            std::chrono::milliseconds guard,
            cyng::logger logger);
        ~modem_session();

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
        // void ipt_cmd(ipt::header const &h, cyng::buffer_t &&body);
        // void ipt_stream(cyng::buffer_t &&data);

        /**
         * start an async write
         * @param f to provide a function that produces a buffer instead of the buffer itself
         * guaranties that scrambled content is has the correct index in the scramble key.
         */
      private:
        cyng::controller &ctl_;
        boost::asio::ip::tcp::socket socket_;
        cyng::logger logger_;

        bus &cluster_bus_;

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
         * parser for modem data
         */
        modem::parser parser_;

        // modem::serializer serializer_;

        cyng::vm_proxy vm_;

        /**
         * tag/pk of device
         */
        // boost::uuids::uuid dev_;

        /**
         * gatekeeper
         */
        cyng::channel_ptr gatekeeper_;
    };

} // namespace smf

#endif
