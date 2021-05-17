/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#ifndef SMF_STORE_INFLUXDB_H
#define SMF_STORE_INFLUXDB_H

//#include <smf/controller_base.h>
//#include <smf/ipt/config.h>

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
    } // namespace influx
} // namespace smf

#endif
