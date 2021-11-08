/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/lmn.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>

namespace smf {

    lmn::lmn(cyng::channel_weak wp
		, cyng::controller& ctl
		, cyng::logger logger
		, cfg& c
		, lmn_type type)
		: sigs_{
			std::bind(&lmn::open, this),
			std::bind(&lmn::do_write, this, std::placeholders::_1),
			std::bind(&lmn::reset_data_sinks, this),
			std::bind(&lmn::add_data_sink, this, std::placeholders::_1),
			std::bind(&lmn::remove_data_sink, this, std::placeholders::_1),
			std::bind(&lmn::set_baud_rate, this, std::placeholders::_1),	
			std::bind(&lmn::set_parity, this, std::placeholders::_1),		
			std::bind(&lmn::set_flow_control, this, std::placeholders::_1),	
			std::bind(&lmn::set_stopbits, this, std::placeholders::_1),		
			std::bind(&lmn::set_databits, this, std::placeholders::_1),		
			std::bind(&lmn::update_statistics, this),	//	9
			std::bind(&lmn::stop, this, std::placeholders::_1)
		}	
		, channel_(wp)
		, ctl_(ctl)
		, logger_(logger)
		, cfg_(c, type)
		, gpio_cfg_(c)
		, port_(ctl.get_ctx())
		, buffer_{ 0 }
		, data_sinks_()
		, accumulated_bytes_{0}
	{
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_names(
                {"open",
                 "write",
                 "reset-data-sinks",
                 "add-data-sink",
                 "remove-data-sink",
                 "set-baud-rate",
                 "set-parity",
                 "set-flow-control",
                 "set-stopbits",
                 "set-databits",
                 "update-statistics"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void lmn::stop(cyng::eod) {
        CYNG_LOG_INFO(logger_, "LMN stop");
        data_sinks_.clear();
        boost::system::error_code ec;
        port_.close(ec);
    }

    void lmn::open() {
        auto const port = cfg_.get_port();

        boost::system::error_code ec;
        if (!port_.is_open()) {
            port_.open(port, ec);
            if (!ec) {

                CYNG_LOG_INFO(logger_, "[" << port << "] is open");

                //
                //	set options
                //
                set_options(port);

                //
                //	ToDo: update status.word
                //

                //
                //	start reading
                //
                do_read();

            } else {
                CYNG_LOG_ERROR(logger_, "[" << port << "] cannot open: " << ec.message());
            }
        } else {
            CYNG_LOG_ERROR(logger_, "[" << port << "] already open");
        }
    }

    void lmn::set_options(std::string const &port) {

        try {
            auto const parity = cfg_.get_parity();
            port_.set_option(parity); // default none
            CYNG_LOG_INFO(logger_, "[" << port << "] parity: " << serial::to_str(parity));

            auto const databits = cfg_.get_databits();
            port_.set_option(databits);
            CYNG_LOG_INFO(logger_, "[" << port << "] databits: " << +databits.value());

            auto const stopbits = cfg_.get_stopbits();
            port_.set_option(stopbits);
            CYNG_LOG_INFO(logger_, "[" << port << "] stopbits: " << serial::to_str(stopbits));

            auto const baud_rate = cfg_.get_baud_rate();
            port_.set_option(baud_rate);
            CYNG_LOG_INFO(logger_, "[" << port << "] baud-rate: " << baud_rate.value());

            auto const flow_control = cfg_.get_flow_control();
            port_.set_option(flow_control);
            CYNG_LOG_INFO(logger_, "[" << port << "] flow-control: " << serial::to_str(flow_control));
        } catch (std::exception const &ex) {

            CYNG_LOG_ERROR(logger_, "set options [" << port << "] failed - " << ex.what());
        }
    }

    void lmn::reset_data_sinks() {
        CYNG_LOG_TRACE(logger_, "[" << cfg_.get_port() << "] clear " << data_sinks_.size() << " data sink(s)");
        data_sinks_.clear();
    }

    void lmn::add_data_sink(std::size_t id) {
        data_sinks_.push_back(id);
        CYNG_LOG_TRACE(logger_, "[" << cfg_.get_port() << "] add data sink #" << id << " +" << data_sinks_.size());
    }

    void lmn::remove_data_sink(std::size_t id) {
        data_sinks_.erase(std::remove(data_sinks_.begin(), data_sinks_.end(), id), data_sinks_.end());
        CYNG_LOG_TRACE(logger_, "[" << cfg_.get_port() << "] remove data sink #" << id << " +" << data_sinks_.size());
    }

    void lmn::update_statistics() {
        if (accumulated_bytes_ > 0) {
            CYNG_LOG_TRACE(logger_, "[" << cfg_.get_port() << "] update statistics: " << accumulated_bytes_ << " bytes/sec");
            if (accumulated_bytes_ < 128) {
                flash_led(std::chrono::milliseconds(500), 2);
            } else if (accumulated_bytes_ < 512) {
                flash_led(std::chrono::milliseconds(250), 4);
            } else {
                flash_led(std::chrono::milliseconds(125), 8);
            }
            accumulated_bytes_ = 0;
        }
        auto sp = channel_.lock();
        if (sp)
            sp->suspend(std::chrono::seconds(1), "update-statistics", cyng::make_tuple());
    }

    void lmn::flash_led(std::chrono::milliseconds ms, std::size_t count) {
        auto const gpio_channel = cfg_gpio::get_name(cfg_.get_lmn_type());
        BOOST_ASSERT(!gpio_channel.empty());
        ctl_.get_registry().dispatch(gpio_channel, "flashing", ms, count);
    }

    void lmn::do_read() {

        port_.async_read_some(boost::asio::buffer(buffer_), [this](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                accumulated_bytes_ += bytes_transferred;

                if (cfg_.is_hex_dump()) {
                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, std::begin(buffer_), std::begin(buffer_) + bytes_transferred);
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[" << cfg_.get_port() << "] received " << bytes_transferred << " bytes:\n" << dmp);
                } else {
                    CYNG_LOG_TRACE(
                        logger_,
                        "[" << cfg_.get_port() << "] received " << bytes_transferred << " / " << accumulated_bytes_ << " bytes");
                }

                //
                //	post data to receiver
                //
                CYNG_LOG_TRACE(
                    logger_,
                    "[" << cfg_.get_port() << "] dispatch " << bytes_transferred << " bytes => " << data_sinks_.size()
                        << " data sink(s)");

                //	copy data
                ctl_.get_registry().dispatch(
                    data_sinks_,
                    "receive",
                    cyng::make_tuple(cyng::buffer_t(std::begin(buffer_), std::begin(buffer_) + bytes_transferred)));

                //
                //	continue reading
                //
                do_read();
            } else {
                CYNG_LOG_ERROR(logger_, "[" << cfg_.get_port() << "] connection closed: " << ec.message());
            }
        });
    }

    void lmn::do_write(cyng::buffer_t data) {

        boost::system::error_code ec;
        boost::asio::write(port_, boost::asio::buffer(data, data.size()), ec);
        if (!ec) {
            CYNG_LOG_TRACE(logger_, "[" << cfg_.get_port() << "] sent " << data.size() << " bytes");
        } else {
            CYNG_LOG_ERROR(logger_, "[" << cfg_.get_port() << "] write failed: " << ec.message());
        }
    }

    void lmn::set_baud_rate(std::uint32_t val) {

        //
        //	set value
        //
        boost::system::error_code ec;
        port_.set_option(boost::asio::serial_port_base::baud_rate(val), ec);
        if (!ec) {

            //
            //	get value and compare
            //
            boost::asio::serial_port_base::baud_rate baud_rate;
            port_.get_option(baud_rate, ec);
            CYNG_LOG_INFO(logger_, "[" << cfg_.get_port() << "] new baudrate " << val << "/" << baud_rate.value());
        } else {
            CYNG_LOG_ERROR(logger_, "[" << cfg_.get_port() << "] set baudrate " << val << " failed: " << ec.message());
        }
    }

    void lmn::set_parity(std::string val) {

        //
        //	set value
        //
        boost::system::error_code ec;
        port_.set_option(serial::to_parity(val), ec);
        if (!ec) {

            //
            //	get value and compare
            //
            boost::asio::serial_port_base::parity parity;
            port_.get_option(parity, ec);
            CYNG_LOG_INFO(logger_, "[" << cfg_.get_port() << "] new parity " << val << "/" << serial::to_str(parity));
        } else {
            CYNG_LOG_ERROR(logger_, "[" << cfg_.get_port() << "] set parity " << val << " failed: " << ec.message());
        }
    }

    void lmn::set_flow_control(std::string val) {

        //
        //	set value
        //
        boost::system::error_code ec;
        port_.set_option(serial::to_flow_control(val), ec);
        if (!ec) {

            //
            //	get value and compare
            //
            boost::asio::serial_port_base::flow_control flow_control;
            port_.get_option(flow_control, ec);

            CYNG_LOG_INFO(logger_, "[" << cfg_.get_port() << "] new flow control " << val << "/" << serial::to_str(flow_control));
        } else {
            CYNG_LOG_ERROR(logger_, "[" << cfg_.get_port() << "] set flow control " << val << " failed: " << ec.message());
        }
    }

    void lmn::set_stopbits(std::string val) {

        //
        //	set value
        //
        boost::system::error_code ec;
        port_.set_option(serial::to_stopbits(val), ec);
        if (!ec) {

            //
            //	get value and compare
            //
            boost::asio::serial_port_base::stop_bits stopbits;
            port_.get_option(stopbits);

            CYNG_LOG_INFO(logger_, "[" << cfg_.get_port() << "] new stopbits " << val << "/" << serial::to_str(stopbits));
        } else {
            CYNG_LOG_ERROR(logger_, "[" << cfg_.get_port() << "] set stopbits " << val << " failed: " << ec.message());
        }
    }
    void lmn::set_databits(std::uint8_t val) {
        //
        //	set value
        //
        boost::system::error_code ec;
        port_.set_option(boost::asio::serial_port_base::character_size(val), ec);
        if (!ec) {

            //
            //	get value and compare
            //
            boost::asio::serial_port_base::character_size databits;
            port_.get_option(databits, ec);
            CYNG_LOG_INFO(logger_, "[" << cfg_.get_port() << "] new databits " << +val << "/" << +databits.value());
        } else {
            CYNG_LOG_ERROR(logger_, "[" << cfg_.get_port() << "] set databits " << +val << " failed: " << ec.message());
        }
    }

} // namespace smf
