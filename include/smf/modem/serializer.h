/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_MODEM_SERIALIZER_H
#define SMF_MODEM_SERIALIZER_H

#include <smf.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/intrinsics/buffer.h>

#include <boost/asio/ip/tcp.hpp>

#include <type_traits>

namespace smf {
    namespace modem {

        /**
         * Currently the serializer is more a namespace than a class.
         * But it's possible to add scrambling or some more sophisticated
         * way to encrypt data.
         */
        class serializer {

          public:
            // serializer();
            [[nodiscard]] static cyng::buffer_t ok();
            [[nodiscard]] static cyng::buffer_t error();
            [[nodiscard]] static cyng::buffer_t busy();
            [[nodiscard]] static cyng::buffer_t no_dialtone();
            [[nodiscard]] static cyng::buffer_t no_carrier();
            [[nodiscard]] static cyng::buffer_t no_answer();

            [[nodiscard]] static cyng::buffer_t ring();
            [[nodiscard]] static cyng::buffer_t connect();

            [[nodiscard]] static cyng::buffer_t ep(boost::asio::ip::tcp::endpoint);

          private:
            [[nodiscard]] static cyng::buffer_t write(std::string const &);
        };
    } // namespace modem
} // namespace smf

#endif
