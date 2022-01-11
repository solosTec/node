/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/client.h>
#include <tasks/reconnect.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/task/channel.h>
#include <cyng/task/controller.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

#ifdef _DEBUG_BROKER_IEC
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

namespace smf {

    client::client(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        bus &cluster_bus,
        cyng::logger logger,
        cyng::key_t key, //  "gwIEC"
        std::uint32_t connect_counter,
        std::uint32_t failure_counter,
        std::string host,
        std::string service,
        std::chrono::seconds reconnect_timeout)
        : state_holder_(),
          sigs_{
              std::bind(&client::init, this, std::placeholders::_1),
              std::bind(&client::start, this),
              std::bind(&client::connect, this),
              std::bind(&client::add_meter, this, std::placeholders::_1, std::placeholders::_2),
              std::bind(&client::remove_meter, this, std::placeholders::_1),
              std::bind(&client::shutdown, this),
              std::bind(
                    &client::on_channel_open,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    std::placeholders::_4),
              std::bind(
                    &client::on_channel_close,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2),
              std::bind(&client::stop, this, std::placeholders::_1),
          },
          channel_(wp),
          ctl_(ctl),
          bus_(cluster_bus),
          logger_(logger),
          key_gw_iec_(key), //  "gwIEC"
          connect_counter_(connect_counter),
          failure_counter_(failure_counter),
          host_(host),
          service_(service),
          reconnect_timeout_(reconnect_timeout),
          mgr_(),
          socket_(ctl.get_ctx()),
          dispatcher_(ctl.get_ctx()),
          buffer_write_(),
          input_buffer_(),
          interval_(60 * 60),   //  1h
          next_readout_(std::chrono::steady_clock::now()),  
          scanner_([this](iec::scanner::state s, std::size_t line, std::string id) {
                CYNG_LOG_TRACE(logger_, "[iec] readout " << to_string(s) << " #" << line);
                switch (s) { 
                case iec::scanner::state::VERSION:
                    state_version(id);
                    break;
                case iec::scanner::state::DATA:
                    state_data(line);
                    break;
                case iec::scanner::state::BBC:
                    state_bbc();
                    break;
                default:
                    break;
                }
            }),
          reconnect_(ctl_.create_channel_with_ref<reconnect>(logger_, channel_)),
          retry_counter_{3} {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_names(
                {"init", "start", "connect", "add.meter", "remove.meter", "shutdown", "channel.open", "channel.close"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }

        //
        //	check writer and reconnect task
        //
        // BOOST_ASSERT(writer_->is_open());
        BOOST_ASSERT(reconnect_->is_open());
    }

    void client::state_version(std::string id) {
        //
        //  update model/device ID
        //
        auto const pos = id.find('\\', 0);
        if (pos == std::string::npos) {
            bus_.req_db_update("device", mgr_.get_key(), cyng::param_map_factory("id", id));
        } else {
            auto const model = id.substr(0, pos);
            bus_.req_db_update("device", mgr_.get_key(), cyng::param_map_factory("id", model));
            if (id.size() > pos) {
                auto const version = id.substr(pos + 1);
                bus_.req_db_update("device", mgr_.get_key(), cyng::param_map_factory("vFirmware", version));
            }
        }
    }

    void client::state_data(std::size_t line) {
        std::stringstream ss;
        ss << mgr_.get_id() << " #" << line;
        auto const msg = ss.str();
        bus_.req_db_update(
            "gwIEC", key_gw_iec_, cyng::param_map_factory()("meter", msg) //  current meter id
        );
    }
    void client::state_bbc() {
        //
        //  next meter
        //
        next_meter();
    }

    void client::stop(cyng::eod) {
        if (state_holder_) {
            reset(state_holder_, state_value::STOPPED);
        }
    }

    void client::shutdown() {
        auto sp = channel_.lock();
        std::string const task_name = (sp) ? sp->get_name() : "no:task";
        CYNG_LOG_WARNING(logger_, "[" << task_name << "] shutdown");

        // BOOST_ASSERT(writer_->is_open());
        BOOST_ASSERT(reconnect_->is_open());
        /*       if (writer_)
                   writer_->stop();*/
        if (reconnect_)
            reconnect_->stop();

        //
        //  stop push task(s)
        //
        mgr_.loop([&, this](meter_state const &state) {
            auto cps = ctl_.get_registry().lookup(state.id_);
            for (auto cp : cps) {
                CYNG_LOG_WARNING(logger_, task_name << " stops " << cp->get_name() << " #" << cp->get_id());
                cp->stop();
            }
        });
    }

    std::uint32_t client::get_remaining_retries() const {
        BOOST_ASSERT(retry_counter_ <= mgr_.get_retry_number());
        return mgr_.get_retry_number() - retry_counter_;
    }

    void client::reset(state_ptr sp, state_value s) {
        if (!sp->is_stopped()) {
            sp->value_ = s;

            boost::system::error_code ignored_ec;

            switch (s) {
            case state_value::START:
                //
                //  make sure that socket is closed
                //
                socket_.close(ignored_ec);
                boost::ignore_unused(std::move(socket_)); //  socket will be new constructed
                retry_counter_ = mgr_.get_retry_number(); //  retry at least 3 times
                break;
            case state_value::WAIT:
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
                break;
            case state_value::CONNECTED:
                break;
            case state_value::RETRY: //  connection got lost, try to reconnect
                //  required to get a proper error code: connection_aborted
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                socket_.close(ignored_ec);
                buffer_write_.clear();
                BOOST_ASSERT(retry_counter_ != 0);
                retry_counter_--;
                break;
            case state_value::STOPPED:
                socket_.close(ignored_ec);
                break;
            default:
                break;
            }
        }
    }

    void client::add_meter(std::string name, cyng::key_t key) {
        auto sp = channel_.lock();
        if (sp) {
            mgr_.add(name, key);
            CYNG_LOG_INFO(logger_, "[" << sp->get_name() << "] add meter #" << mgr_.size() << ": " << name);
        }
    }

    void client::remove_meter(std::string name) {

        //
        //  push task will be stopped by cluster task
        //

        auto sp = channel_.lock();
        if (sp) {
            mgr_.remove(name);
            CYNG_LOG_INFO(logger_, "[" << sp->get_name() << "] remove meter #" << mgr_.size() << ": " << name);
        }
    }
    void client::init(std::chrono::seconds interval) {
        BOOST_ASSERT(interval > std::chrono::seconds(60 * 5));
        interval_ = interval;
    }
    void client::start() {
        auto sp = channel_.lock();
        if (sp) {

            //
            //  calculate next readout time
            //  this is an absolute time
            //
            next_readout_ += interval_;
            if (sp->suspend(next_readout_, "start")) {

                CYNG_LOG_INFO(
                    logger_, "start client: " << host_ << ':' << service_ << " #" << mgr_.size() << " @" << next_readout_);
                connect();
            } else {
                CYNG_LOG_FATAL(logger_, "cannot start client: " << host_ << ':' << service_);
            }
        }
    }

    void client::connect() {

        try {

            // CYNG_LOG_TRACE(logger_, "[client] " << host_ << ':' << service_ << " - use count: " << channel_.use_count());

            //
            // Start the connect actor.
            //
            boost::asio::ip::tcp::resolver r(ctl_.get_ctx());
            state_holder_ = std::make_shared<state>(r.resolve(host_, service_));

            //  update gateway status
            reset(state_holder_, state_value::WAIT);

            start_connect(state_holder_, state_holder_->endpoints_.begin());
        } catch (boost::system::system_error const &ex) {
            CYNG_LOG_WARNING(logger_, "[client] start: " << host_ << ':' << service_ << " failed: " << ex.what());
        }
    }

    void client::start_connect(state_ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (endpoint_iter != sp->endpoints_.end()) {
                CYNG_LOG_TRACE(logger_, "[client] trying " << endpoint_iter->endpoint() << "...");

                // Start the asynchronous connect operation.
                socket_.async_connect(
                    endpoint_iter->endpoint(), std::bind(&client::handle_connect, this, sp, std::placeholders::_1, endpoint_iter));
            } else {

                //
                //  connect failed
                //  extend lifetime of channel
                //
                auto ch_ptr = channel_.lock();
                if (ch_ptr) {

                    //
                    //  There are no more endpoints to try. Shutdown the client.
                    //  Full reset.
                    //
                    bus_.req_db_insert_auto(
                        "iecUplink",
                        cyng::data_generator(
                            std::chrono::system_clock::now(),
                            "connect " + ch_ptr->get_name() + " failed: " + mgr_.get_id() + "#" + std::to_string(mgr_.index()),
                            boost::asio::ip::tcp::endpoint(),
                            boost::uuids::nil_uuid()));

                    ++failure_counter_;

                    if ((retry_counter_ != 0) && is_sufficient_time(std::chrono::minutes(5))) {
                        bus_.req_db_update(
                            "gwIEC",
                            key_gw_iec_,
                            cyng::param_map_factory()("failureCounter", failure_counter_) // failed connects
                            ("state", static_cast<std::uint16_t>(3))                      //  reconnect
                            ("meter",
                             std::string("[reconnect #") + std::to_string(get_remaining_retries()) +
                                 std::string("]")) //  reconnect state
                        );
                        retry_counter_--;
                        //  reconnect after a specified "reconnect.timeout"
                        reconnect_->suspend(reconnect_timeout_, "run");
                    } else {
                        bus_.req_db_update(
                            "gwIEC",
                            key_gw_iec_,
                            cyng::param_map_factory()("failureCounter", failure_counter_) // failed connects
                            ("state", static_cast<std::uint16_t>(0))                      //  offline
                            ("meter", mgr_.get_id())                                      //  current meter id
                        );
                        reset(sp, state_value::START);
                    }
                }
            }
        }
    }

    void client::handle_connect(
        state_ptr sp,
        const boost::system::error_code &ec,
        boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            // The async_connect() function automatically opens the socket at the start
            // of the asynchronous operation. If the socket is closed at this time then
            // the timeout handler must have run first.
            if (!socket_.is_open()) {

                CYNG_LOG_WARNING(logger_, "[client] connect timed out - " << mgr_.get_id());

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Check if the connect operation failed before the deadline expired.
            else if (ec) {
                CYNG_LOG_WARNING(
                    logger_, "[client] " << mgr_.get_id() << " connect error - " << ec.value() << ": " << ec.message());

                // We need to close the socket used in the previous connection attempt
                // before starting a new one.
                boost::system::error_code ignored_ec;
                socket_.close(ignored_ec);

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Otherwise we have successfully established a connection.
            else {
                CYNG_LOG_INFO(logger_, "[client] connected to " << endpoint_iter->endpoint());
                reset(sp, state_value::CONNECTED);

                //
                //  get current meter id
                //  and open push channel
                //  The name of the meter is also the name of the task
                //
                auto const name = mgr_.get_id();

                //
                //	update "gwIEC"
                //
                bus_.req_db_update(
                    "gwIEC",
                    key_gw_iec_,
                    cyng::param_map_factory()("state", static_cast<std::uint16_t>(2)) //  state: online/connected
                    ("index", mgr_.index())                                           //  current meter index
                    ("meter", name)                                                   //  current meter id
                );

                //
                //	update configuration
                //
                mgr_.loop([this](client::meter_state const &state) -> void {
                    bus_.req_db_update(
                        "meterIEC", state.key_, cyng::param_map_factory()("lastSeen", std::chrono::system_clock::now()));
                });

                //
                // Start the input actor.
                //
                do_read(sp);

                //
                //  open push channel
                //
                ctl_.get_registry().dispatch(name, "open");
            }
        }
    }

    void client::send_query(std::string id) {
        //
        //	send query
        //
        auto sp = state_holder_->shared_from_this();
        boost::asio::post(dispatcher_, [this, sp, id]() {
            BOOST_ASSERT(buffer_write_.empty());
            //
            //	send query to meter
            //
            buffer_write_.push_back(generate_query(id));
            do_write(sp);
        });
    }

    void client::do_read(state_ptr sp) {
        //
        //	connect was successful
        //

        // Start an asynchronous operation to read
        socket_.async_read_some(
            boost::asio::buffer(input_buffer_),
            std::bind(&client::handle_read, this, sp, std::placeholders::_1, std::placeholders::_2));
    }

    void client::handle_read(state_ptr sp, const boost::system::error_code &ec, std::size_t bytes_transferred) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (!ec) {
                CYNG_LOG_DEBUG(logger_, "[client] received " << bytes_transferred << " bytes");

                //
                //	parse message
                //
                std::stringstream ss;
                ss << "[client] received " << bytes_transferred << " bytes";
#ifdef _DEBUG_BROKER_IEC
                {
                    std::stringstream ss;
                    cyng::io::hex_dump<8> hd;
                    hd(ss, input_buffer_.data(), input_buffer_.data() + bytes_transferred);
                    auto const dmp = ss.str();
                    CYNG_LOG_TRACE(logger_, "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes:\n" << dmp);
                }
#endif

                //
                //  send to push target
                //
                ctl_.get_registry().dispatch(
                    mgr_.get_id(), "push", cyng::buffer_t(input_buffer_.data(), input_buffer_.data() + bytes_transferred));
                //
                //  start parser
                //
                scanner_.read(input_buffer_.data(), input_buffer_.data() + bytes_transferred);

                do_read(sp);
            } else if (ec != boost::asio::error::connection_aborted) {

                CYNG_LOG_WARNING(logger_, "[client] " << mgr_.get_id() << " read " << ec.value() << ": " << ec.message());

                if (mgr_.is_complete()) {

                    bus_.req_db_update(
                        "gwIEC",
                        key_gw_iec_,
                        cyng::param_map_factory()("state", static_cast<std::uint16_t>(0)) //  offline
                        ("index", mgr_.index())                                           //  current meter index
                        ("meter", "[complete]")                                           //  flag incomplete readout
                    );

                    reset(sp, state_value::START);

                } else {
                    //
                    //  goto RETRY state
                    //
                    bus_.req_db_update(
                        "gwIEC",
                        key_gw_iec_,
                        cyng::param_map_factory()("state", static_cast<std::uint16_t>(0)) //  offline
                        ("index", mgr_.index())                                           //  current meter index
                        ("meter", "[retry#" + std::to_string(retry_counter_) + "]")       //  flag incomplete readout
                    );

                    //  skip failed meter
                    mgr_.next();

                    auto ch_ptr = channel_.lock();
                    if (ch_ptr) {
                        if ((retry_counter_ != 0) && is_sufficient_time(std::chrono::minutes(5))) {
                            reset(sp, state_value::RETRY);
                            //  reconnect after a specified "reconnect.timeout"
                            reconnect_->suspend(reconnect_timeout_, "run");
                        } else {
                            reset(sp, state_value::START);
                        }
                    }
                }
            }
        }
    }

    void client::do_write(state_ptr sp) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            BOOST_ASSERT(!buffer_write_.empty());

            // Start an asynchronous operation to send a heartbeat message.
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
                dispatcher_.wrap(std::bind(&client::handle_write, this, sp, std::placeholders::_1)));
        }
    }

    void client::handle_write(state_ptr sp, const boost::system::error_code &ec) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (!ec) {

                buffer_write_.pop_front();
                if (!buffer_write_.empty()) {
                    do_write(sp);
                }
            } else if (ec != boost::asio::error::connection_aborted) {
                CYNG_LOG_ERROR(logger_, "[client]  write failed: " << ec.message());

                boost::system::error_code ignored_ec;
                //  required to get a proper error code: connection_aborted
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
                socket_.close(ignored_ec);
                buffer_write_.clear();
            }
        }
    }

    void client::on_channel_open(bool success, std::string meter, std::uint32_t channel, std::uint32_t source) {

        BOOST_ASSERT(boost::algorithm::equals(meter, mgr_.get_id()));
        if (success) {
            CYNG_LOG_INFO(logger_, "[client]  send query: " << meter << " #" << mgr_.index() << "/" << mgr_.size());
            send_query(meter);
        } else {

            //
            //  open push channel failed
            //  next meter
            //
            CYNG_LOG_WARNING(logger_, "[client]  open push channel \"" << meter << "\" failed");
            next_meter();
        }
    }
    void client::on_channel_close(std::string, std::uint32_t) {}

    void client::next_meter() {

        //
        // close open channel
        //
        ctl_.get_registry().dispatch(mgr_.get_id(), "close");

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
                //    next meters
                //
                auto const name = mgr_.get_id();

                //
                //  open push channel
                //
                ctl_.get_registry().dispatch(name, "open");

                //} else {

                //    std::stringstream ss;
                //    ss << "readout is incomplete - socket closed " << mgr_.get_id() << ": " << mgr_.index() << "/" << mgr_.size();
                //    auto const msg = ss.str();

                //    bus_.req_db_insert_auto(
                //        "iecUplink",
                //        cyng::data_generator(
                //            std::chrono::system_clock::now(), msg, boost::asio::ip::tcp::endpoint(), boost::uuids::nil_uuid()));
            }

        } else {

            //
            //    no more meters
            //
            CYNG_LOG_INFO(logger_, "[iec] close connection #" << mgr_.index() << "/" << mgr_.size());
            boost::system::error_code ignored_ec;
            //  required to get a proper error code: connection_aborted
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
            socket_.close(ignored_ec);
        }
    }

    //
    //  -- client meter_mgr --
    //
    client::meter_mgr::meter_mgr()
        : meters_()
        , index_{0} {}

    std::string client::meter_mgr::get_id() const { return meters_.empty() ? "[no-meters]" : meters_.at(index_).id_; }
    cyng::key_t client::meter_mgr::get_key() const { return meters_.empty() ? cyng::key_t() : meters_.at(index_).key_; }

    void client::meter_mgr::reset() { index_ = 0; }

    bool client::meter_mgr::next() {
        ++index_;
        if (index_ == size()) {
            reset(); //  index
            return true;
        }
        return false;
    }

    bool client::meter_mgr::is_complete() const { return (index_ == 0) || ((index_ + 1) == size()); }

    void client::meter_mgr::add(std::string name, cyng::key_t key) { meters_.push_back(meter_state(name, key)); }

    void client::meter_mgr::remove(std::string name) {
        meters_.erase(
            std::remove_if(
                meters_.begin(),
                meters_.end(),
                [&](meter_state const &state) { return boost::algorithm::equals(name, state.id_); }),
            meters_.end());
    }
    std::uint32_t client::meter_mgr::size() const { return static_cast<std::uint32_t>(meters_.size()); }
    std::uint32_t client::meter_mgr::index() const { return index_; }

    void client::meter_mgr::loop(std::function<void(client::meter_state const &)> cb) {
        for (auto const &state : meters_) {
            cb(state);
        }
    }

    std::uint32_t client::meter_mgr::get_retry_number() const { return 3u + size(); }

    //
    //  -- client state --
    //
    client::state::state(boost::asio::ip::tcp::resolver::results_type &&res)
        : value_(state_value::START)
        , endpoints_(std::move(res)) {}

    //
    //  -- client meter_state --
    //
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
