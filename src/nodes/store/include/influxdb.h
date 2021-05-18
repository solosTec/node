/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_STORE_INFLUXDB_H
#define SMF_STORE_INFLUXDB_H

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/object.h>

#include <boost/asio.hpp>
#include <iostream>

namespace smf {
    namespace influx {
        int create_db(
            boost::asio::io_context &,
            std::ostream &,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db);

        int show_db(
            boost::asio::io_context &,
            std::ostream &,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert);

        int drop_db(
            boost::asio::io_context &,
            std::ostream &,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db);

        std::string push_over_http(
            boost::asio::io_context &ctx,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db,
            std::string const &stmt);

        std::string to_line_protocol(cyng::object obj);
        std::string to_line_protocol(std::string s);
        std::string to_line_protocol(std::chrono::system_clock::time_point tp);

    } // namespace influx
} // namespace smf

#endif
