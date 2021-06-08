/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/client.h>
#include <tasks/writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/task/channel.h>
#include <cyng/task/controller.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

//#ifdef _DEBUG_BROKER_IEC
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
//#endif

namespace smf {

    client::client(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        bus &cluster_bus,
        cyng::logger logger,
        std::filesystem::path out,
        cyng::key_t key, //  "gwIEC"
        std::uint32_t connect_counter,
        std::uint32_t failure_counter)
        : state_(state::START),
          sigs_{
              std::bind(&client::start, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
              std::bind(&client::add_meter, this, std::placeholders::_1, std::placeholders::_2),
              std::bind(&client::stop, this, std::placeholders::_1),
          },
          channel_(wp),
          ctl_(ctl),
          bus_(cluster_bus),
          logger_(logger),
          key_gw_iec_(key), //  "gwIEC"
          connect_counter_(connect_counter),
          failure_counter_(failure_counter),
          mgr_(),
          endpoints_(),
          socket_(ctl.get_ctx()),
          dispatcher_(ctl.get_ctx()),
          buffer_write_(),
          input_buffer_(),
          parser_(
              [this](cyng::obis code, std::string value, std::string unit) {
                  auto const name = mgr_.get_id();
                  CYNG_LOG_TRACE(logger_, "[iec] data " << name << " - " << code << ": " << value << " " << unit);

                  //
                  //	store data to csv file
                  //
                  writer_->dispatch("store", cyng::make_tuple(code, value, unit));

                  //
                  //	increment counter
                  //
                  ++entries_;

                  std::stringstream ss;
                  ss << mgr_.get_id() << "#" << entries_;
                  auto const msg = ss.str();
                  bus_.req_db_update(
                      "gwIEC",
                      key_gw_iec_,
                      cyng::param_map_factory()("meter", msg) //  current meter id
                  );


              },
              [this](std::string dev, bool crc) {

                    //
                    //    get current meter id
                    //
                    auto const name = mgr_.get_id();
                    CYNG_LOG_INFO(logger_, "[iec] " << name << " readout " << mgr_.index() << "/" << mgr_.size() << " complete " << dev);

                    //
                    //	close CSV file
                    //
                    writer_->dispatch("commit", cyng::make_tuple());

                    //
                    //    message for IEC uplink table
                    //
                    std::stringstream ss;
                    ss << name << " readout " << mgr_.index() << "/" << mgr_.size() << " complete (" << dev << ") #" << entries_;
                    auto const msg = ss.str();

                    //
                    //	reset
                    //
                    entries_ = 0;

                    try {
                        bus_.req_db_insert_auto(
                            "iecUplink",
                            cyng::data_generator(
                                std::chrono::system_clock::now(), msg, socket_.remote_endpoint(), boost::uuids::nil_uuid()));
                    } catch (std::exception const &ex) {
                        //  socket already closed
                    }

                    //
                    //	next meter
                    //
                    if (!mgr_.next()) {

                        if (socket_.is_open()) {
                            //
                            //    update index in gwIEC table
                            //
                            bus_.req_db_update(
                                "gwIEC",
                                key_gw_iec_,
                                cyng::param_map_factory()("index", mgr_.index()) //  current meter index
                                ("meter", mgr_.get_id())                         //  current meter id
                            );

                            //
                            //    more meters
                            //
                            auto const name = mgr_.get_id();
                            writer_->dispatch("open", cyng::make_tuple(name));
                            CYNG_LOG_INFO(logger_, "[client] query " << name << " #" << mgr_.index() << "/" << mgr_.size());
                            send_query(name);
                        } else {
                            //bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[iec]");
                            std::stringstream ss;
                            ss << "readout is incomplete - socket closed " << mgr_.get_id() << ": " << mgr_.index() << "/" << mgr_.size();
                            auto const msg = ss.str();

                            bus_.req_db_insert_auto(
                                "iecUplink",
                                cyng::data_generator(
                                    std::chrono::system_clock::now(),
                                    msg,
                                    boost::asio::ip::tcp::endpoint(),
                                    boost::uuids::nil_uuid()));

                        }

                    } else {

                        //
                        //    no more meters
                        //
                        CYNG_LOG_INFO(logger_, "[iec] close connection " << name << " #" << mgr_.index() << "/" << mgr_.size());
                        boost::system::error_code ignored_ec;
                        socket_.close(ignored_ec);
                    }
              },
              1u),
          writer_(ctl_.create_channel_with_ref<writer>(logger_, out)),
          entries_{0} {
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("start", slot++);
            sp->set_channel_name("add.meter", slot++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }

        //
        //	check writer
        //
        BOOST_ASSERT(writer_->is_open());
    }

    void client::stop(cyng::eod) { state_ = state::STOPPED; }

    void client::add_meter(std::string name, cyng::key_t key) {
        auto sp = channel_.lock();
        if (sp) {
            mgr_.add(name, key);
            CYNG_LOG_INFO(logger_, "[" << sp->get_name() << "] add meter #" << mgr_.size() << ": " << name);
        }
    }

    void client::start(std::string address, std::string service, std::chrono::seconds interval) {
        auto sp = channel_.lock();
        if (sp) {

            CYNG_LOG_INFO(logger_, "start client: " << address << ':' << service << " #" << mgr_.size());

            //
            //  restart timer
            //
            sp->suspend(interval, "start", cyng::make_tuple(std::string(address), std::string(service), interval));

            try {
                state_ = state::START; //	update client state

                //
                // update "gwIEC"
                //
                ++connect_counter_;
                bus_.req_db_update(
                    "gwIEC",
                    key_gw_iec_,
                    cyng::param_map_factory()("connectCounter", connect_counter_) //  increased connect counter
                    ("state", static_cast<std::uint16_t>(1))                      //  waiting
                    ("index", mgr_.size())                                        //  show max. meter index
                    ("meter", mgr_.get_id())                                      //  current meter id/name
                );

                //
                //  make sure that socket is closed
                //
                boost::system::error_code ignored_ec;
                socket_.close(ignored_ec);
                std::move(socket_); //  socket will be new constructed

                boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
                connect(r.resolve(address, service));
            } catch (boost::system::system_error const &ex) {
                CYNG_LOG_WARNING(logger_, "[client] start: " << address << ':' << service << " failed: " << ex.what());
            }
        }
    }

    void client::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

        state_ = state::WAIT;

        // Start the connect actor.
        endpoints_ = endpoints;
        if (!endpoints_.empty()) {
            start_connect(endpoints_.begin());
        } else {
            bus_.req_db_insert_auto(
                "iecUplink",
                cyng::data_generator(
                    std::chrono::system_clock::now(),
                    "invalid tcp/ip configuration: " + mgr_.get_id(),
                    boost::asio::ip::tcp::endpoint(),
                    boost::uuids::nil_uuid()));
        }
    }

    void client::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

        if (endpoint_iter != endpoints_.end()) {
            CYNG_LOG_TRACE(logger_, "[client] trying " << endpoint_iter->endpoint() << "...");

            // Start the asynchronous connect operation.
            socket_.async_connect(
                endpoint_iter->endpoint(), std::bind(&client::handle_connect, this, std::placeholders::_1, endpoint_iter));
        } else {

            //
            //  extend lifetime
            //
            auto sp = channel_.lock();
            if (sp) {

                //
                // There are no more endpoints to try. Shut down the client.
                //
                bus_.req_db_insert_auto(
                    "iecUplink",
                    cyng::data_generator(
                        std::chrono::system_clock::now(),
                        "connect " + sp->get_name() + " failed: " + mgr_.get_id() + "#" + std::to_string(mgr_.index()),
                        boost::asio::ip::tcp::endpoint(),
                        boost::uuids::nil_uuid()));

                ++failure_counter_;
                bus_.req_db_update(
                    "gwIEC",
                    key_gw_iec_,
                    cyng::param_map_factory()("failureCounter", failure_counter_) // failed connects
                    ("state", static_cast<std::uint16_t>(0))                      //  offline
                    ("meter", mgr_.get_id())                                      //  current meter id
                );

                //
                //  reset meter index
                //
                // mgr_.reset();
            }
        }
    }

    void client::handle_connect(
        const boost::system::error_code &ec,
        boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

        if (is_stopped())
            return;

        // The async_connect() function automatically opens the socket at the start
        // of the asynchronous operation. If the socket is closed at this time then
        // the timeout handler must have run first.
        if (!socket_.is_open()) {

            try {
                CYNG_LOG_WARNING(logger_, "[client] connect timed out - " << endpoint_iter->endpoint());
            } catch (std::exception const &) {
                CYNG_LOG_WARNING(logger_, "[client] connect timed out - " << mgr_.get_id());
            }

            // Try the next available endpoint.
            start_connect(++endpoint_iter);
        }

        // Check if the connect operation failed before the deadline expired.
        else if (ec) {
            CYNG_LOG_WARNING(logger_, "[client] " << mgr_.get_id() << " connect error - " << ec.value() << ": " << ec.message());

            // We need to close the socket used in the previous connection attempt
            // before starting a new one.
            boost::system::error_code ignored_ec;
            socket_.close(ignored_ec);

            // Try the next available endpoint.
            start_connect(++endpoint_iter);
        }

        // Otherwise we have successfully established a connection.
        else {
            CYNG_LOG_INFO(logger_, "[client] connected to " << endpoint_iter->endpoint());
            state_ = state::CONNECTED;

            //
            //  get current meter id
            //  and open writer
            //
            auto const name = mgr_.get_id();

            //
            //	update "gwIEC"
            //
            bus_.req_db_update(
                "gwIEC",
                key_gw_iec_,
                cyng::param_map_factory()("state", static_cast<std::uint16_t>(2)) //  state: online
                ("index", mgr_.index())                                           //  current meter index
                ("meter", mgr_.get_id())                                          //  current meter id
            );

            //
            //	update configuration
            //
            mgr_.loop([this](client::meter_state const &state) -> void {
                bus_.req_db_update("meterIEC", state.key_, cyng::param_map_factory()("lastSeen", std::chrono::system_clock::now()));
            });

            // Start the input actor.
            do_read();

            writer_->dispatch("open", cyng::make_tuple(name));
            CYNG_LOG_INFO(logger_, "[client] query " << name << " #" << mgr_.index() << "/" << mgr_.size());
            send_query(name);
        }
    }

    void client::send_query(std::string id) {
        //
        //	send query
        //
        boost::asio::post(dispatcher_, [this, id]() {
            //
            //	send query to meter
            //
            buffer_write_.push_back(generate_query(id));

            if (!buffer_write_.empty())
                do_write();
        });
    }

    void client::do_read() {
        //
        //	connect was successful
        //

        // Start an asynchronous operation to read
        socket_.async_read_some(
            boost::asio::buffer(input_buffer_),
            std::bind(&client::handle_read, this, std::placeholders::_1, std::placeholders::_2));
    }

    void client::handle_read(const boost::system::error_code &ec, std::size_t bytes_transferred) {
        if (is_stopped())
            return;

        if (!ec) {
            CYNG_LOG_DEBUG(logger_, "[client] received " << bytes_transferred << " bytes");

            //
            //	parse message
            //
            std::stringstream ss;
            ss << "[client] received " << bytes_transferred << " bytes";
            //#ifdef _DEBUG_BROKER_IEC
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, input_buffer_.data(), input_buffer_.data() + bytes_transferred);
                auto const dmp = ss.str();
                CYNG_LOG_TRACE(logger_, "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes:\n" << dmp);
            }
            //#endif

            parser_.read(input_buffer_.data(), input_buffer_.data() + bytes_transferred);

            do_read();
        } else if (ec != boost::asio::error::connection_aborted) {

            CYNG_LOG_WARNING(logger_, "[client] " << mgr_.get_id() << " read " << ec.value() << ": " << ec.message());

            boost::system::error_code ignored_ec;
            socket_.close(ignored_ec);
            buffer_write_.clear();

            if (mgr_.is_complete()) {
                bus_.req_db_update(
                    "gwIEC",
                    key_gw_iec_,
                    cyng::param_map_factory()("state", static_cast<std::uint16_t>(0)) //  offline
                    ("index", mgr_.index())                                           //  current meter index
                    ("meter", "[incomplete]")                                         //  flag incomplete readout
                );
            }
        }
    }

    void client::do_write() {
        if (is_stopped())
            return;

        BOOST_ASSERT(!buffer_write_.empty());

        // Start an asynchronous operation to send a heartbeat message.
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
            dispatcher_.wrap(std::bind(&client::handle_write, this, std::placeholders::_1)));
    }

    void client::handle_write(const boost::system::error_code &ec) {
        if (is_stopped())
            return;

        if (!ec) {

            buffer_write_.pop_front();
            if (!buffer_write_.empty()) {
                do_write();
            }
        } else if (ec != boost::asio::error::connection_aborted) {
            CYNG_LOG_ERROR(logger_, "[client]  write failed: " << ec.message());

            boost::system::error_code ignored_ec;
            socket_.close(ignored_ec);
            buffer_write_.clear();
        }
    }

    client::meter_mgr::meter_mgr()
        : meters_()
        , index_{0} {}

    std::string client::meter_mgr::get_id() const { return meters_.empty() ? "" : meters_.at(index_).id_; }
    cyng::key_t client::meter_mgr::get_key() const { return meters_.empty() ? cyng::key_t() : meters_.at(index_).key_; }

    void client::meter_mgr::reset() { index_ = 0; }

    bool client::meter_mgr::next() {
        ++index_;
        if (index_ == size()) {
            reset();
            return true;
        }
        return false;
    }

    bool client::meter_mgr::is_complete() const { return (index_ == 0) || ((index_ + 1) == size()); }

    void client::meter_mgr::add(std::string name, cyng::key_t key) { meters_.push_back(meter_state(name, key)); }

    std::uint32_t client::meter_mgr::size() const { return static_cast<std::uint32_t>(meters_.size()); }
    std::uint32_t client::meter_mgr::index() const { return index_; }

    void client::meter_mgr::loop(std::function<void(client::meter_state const &)> cb) {
        for (auto const &state : meters_) {
            cb(state);
        }
    }

    client::meter_state::meter_state(std::string id, cyng::key_t key)
        : id_(id)
        , key_(key) {}

    cyng::buffer_t generate_query(std::string meter) {
        std::stringstream ss;
        ss << "/?" << meter << "!\r\n";
        auto const query = ss.str();
        return cyng::to_buffer(query);
    }

} // namespace smf
