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
            serializer();
            [[nodiscard]] cyng::buffer_t ok() const;
            [[nodiscard]] cyng::buffer_t error() const;
            [[nodiscard]] cyng::buffer_t busy() const;
            [[nodiscard]] cyng::buffer_t no_dialtone() const;
            [[nodiscard]] cyng::buffer_t no_carrier() const;
            [[nodiscard]] cyng::buffer_t no_answer() const;

            [[nodiscard]] cyng::buffer_t ring() const;
            [[nodiscard]] cyng::buffer_t connect() const;

          private:
            [[nodiscard]] cyng::buffer_t write(std::string const &) const;
        };
    } // namespace modem
} // namespace smf

#endif
