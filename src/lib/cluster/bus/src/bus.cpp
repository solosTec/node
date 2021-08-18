/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf.h>
#include <smf/cluster/bus.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/add.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/sys/process.h>
#include <cyng/vm/generator.hpp>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

#include <boost/bind.hpp>

namespace smf {

    bus::bus(
        boost::asio::io_context &ctx,
        cyng::logger logger,
        toggle::server_vec_t &&tgl,
        std::string const &node_name,
        boost::uuids::uuid tag,
        bus_interface *bip)
        : state_holder_()
        , ctx_(ctx)
        , logger_(logger)
        , tgl_(std::move(tgl))
        , node_name_(node_name)
        , tag_(tag)
        , bip_(bip)
        , socket_(ctx_)
        , timer_(ctx_)
        , buffer_write_()
        , input_buffer_()
        , vm_()
        , parser_([this](cyng::object &&obj) {
            //  un-comment this line to debug problems with transferring data over
            //  the cluster bus.
            // CYNG_LOG_DEBUG(logger_, "[cluster] parser: " << cyng::io::to_typed(obj));
            vm_.load(std::move(obj));
        }) {
        vm_ = init_vm(bip);
        std::size_t slot{0};
        vm_.set_channel_name("cluster.res.login", slot++);
        vm_.set_channel_name("cluster.req.ping", slot++);
        vm_.set_channel_name("cluster.disconnect", slot++);
        vm_.set_channel_name("db.res.insert", slot++);
        vm_.set_channel_name("db.res.trx", slot++);
        vm_.set_channel_name("db.res.update", slot++);
        vm_.set_channel_name("db.res.remove", slot++);
        vm_.set_channel_name("db.res.clear", slot++);
        vm_.set_channel_name("pty.res.login", slot++);
    }

    cyng::vm_proxy bus::init_vm(bus_interface *bip) {

        return bip->get_fabric()->create_proxy(
            get_vm_func_on_login(bip),      //	"cluster.res.login"
            get_vm_func_on_ping(this),      //	"cluster.req.ping"
            get_vm_func_on_disconnect(bip), //	"cluster.disconnect"
            get_vm_func_db_res_insert(bip), //	"db.res.insert"
            get_vm_func_db_res_trx(bip),    //	"db.res.trx"
            get_vm_func_db_res_update(bip), //	"db.res.update"
            get_vm_func_db_res_remove(bip), //	"db.res.remove"
            get_vm_func_db_res_clear(bip)   //	"db.res.clear"
        );
    }

    void bus::start() {

        auto const srv = tgl_.get();
        CYNG_LOG_INFO(logger_, "[cluster] connect(" << srv << ")");
        state_holder_.reset();

        //
        //	connect to cluster
        //
        try {
            boost::asio::ip::tcp::resolver r(ctx_);
            state_holder_ = std::make_shared<state>(r.resolve(tgl_.get().host_, tgl_.get().service_));
            connect(state_holder_->shared_from_this());
        } catch (std::exception const &ex) {
            CYNG_LOG_ERROR(logger_, "[cluster] connect: " << ex.what());

            tgl_.changeover();
            CYNG_LOG_WARNING(logger_, "[cluster] connect failed - switch to " << tgl_.get());
        }
    }

    void bus::stop() {
        CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " stop");
        vm_.stop();
        if (state_holder_) {
            reset(state_holder_, state_value::STOPPED);
        }
    }

    boost::uuids::uuid bus::get_tag() const { return tag_; }
    bool bus::is_connected() const { return (state_holder_) ? state_holder_->is_connected() : false; }

    void bus::reset(state_ptr sp, state_value s) {
        if (sp && !sp->is_stopped()) {

            sp->value_ = s;
            boost::system::error_code ignored_ec;
            //  required to get a proper error code: bad_descriptor (EBADF) instead of connection_aborted
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ignored_ec);
            socket_.close(ignored_ec);
            timer_.cancel();
            if (s != state_value::STOPPED) {
                buffer_write_.clear();
            }
        }
    }

    void bus::reconnect_timeout(state_ptr sp, const boost::system::error_code &ec) {
        if (sp && !sp->is_stopped()) {

            if (!ec) {
                CYNG_LOG_TRACE(logger_, "[cluster] reconnect timeout " << ec);
                if (!sp->is_connected()) {
                    start();
                }
            } else if (ec != boost::asio::error::operation_aborted) {
                CYNG_LOG_WARNING(logger_, "[cluster] reconnect timer: " << ec.message());
            }
        }
    }

    void bus::connect(state_ptr sp) {

        sp->value_ = state_value::WAIT;

        // Start the connect actor.
        start_connect(sp, sp->endpoints_.begin());
    }

    void bus::start_connect(state_ptr sp, boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (endpoint_iter != sp->endpoints_.end()) {

                CYNG_LOG_TRACE(logger_, "[cluster] trying " << endpoint_iter->endpoint() << "...");

                // Start the asynchronous connect operation.
                socket_.async_connect(
                    endpoint_iter->endpoint(), std::bind(&bus::handle_connect, this, sp, std::placeholders::_1, endpoint_iter));
            } else {

                //
                //  full reset
                //
                reset(sp, state_value::START);

                //
                //	alter connection endpoint
                //
                tgl_.changeover();
                CYNG_LOG_WARNING(logger_, "[cluster] connect failed - switch to " << tgl_.get());

                //
                //	reconnect after 20 seconds
                //
                set_reconnect_timer(std::chrono::seconds(20));
            }
        }
    }

    void bus::set_reconnect_timer(std::chrono::seconds delay) {

        timer_.expires_after(delay);
        timer_.async_wait(boost::asio::bind_executor(
            cyng::expose_dispatcher(vm_),
            boost::bind(&bus::reconnect_timeout, this, state_holder_, boost::asio::placeholders::error)));
    }

    void bus::handle_connect(
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

                CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect timed out: " << ec.message());

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Check if the connect operation failed before the deadline expired.
            else if (ec) {
                CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect error " << ec.value() << ": " << ec.message());

                // We need to close the socket used in the previous connection attempt
                // before starting a new one.
                socket_.close();

                // Try the next available endpoint.
                start_connect(sp, ++endpoint_iter);
            }

            // Otherwise we have successfully established a connection.
            else {
                CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " connected to " << endpoint_iter->endpoint());
                sp->value_ = state_value::CONNECTED;

                //
                //	send login sequence
                //
                auto cfg = tgl_.get();
                add_msg(
                    sp,
                    cyng::serialize_invoke(
                        "cluster.req.login",
                        cfg.account_,
                        cfg.pwd_,
                        cyng::sys::get_process_id(),
                        node_name_,
                        tag_,
                        cyng::version(SMF_VERSION_MAJOR, SMF_VERSION_MINOR)));

                // Start the input actor.
                do_read(sp);
            }
        }
    }

    void bus::do_read(state_ptr sp) {
        //
        //	connect was successful
        //

        // Start an asynchronous operation to read
        socket_.async_read_some(
            boost::asio::buffer(input_buffer_),
            std::bind(&bus::handle_read, this, sp, std::placeholders::_1, std::placeholders::_2));
    }

    void bus::do_write(state_ptr sp) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            BOOST_ASSERT(!buffer_write_.empty());

            //
            //	write actually data to socket
            //
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
                cyng::expose_dispatcher(vm_).wrap(std::bind(&bus::handle_write, this, sp, std::placeholders::_1)));
        }
    }

    void bus::handle_read(state_ptr sp, const boost::system::error_code &ec, std::size_t bytes_transferred) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (!ec) {
                CYNG_LOG_DEBUG(logger_, "[cluster] " << tgl_.get() << " received " << bytes_transferred << " bytes");

                //
                //	let parse it
                //
                parser_.read(input_buffer_.begin(), input_buffer_.begin() + bytes_transferred);

                //
                //	continue reading
                //
                do_read(sp);
            } else {

                CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " read " << ec.value() << ": " << ec.message());
                //
                //  cleanup
                //
                reset(sp, state_value::START);

                //
                //	call disconnect function
                //
                vm_.load(cyng::generate_invoke("cluster.disconnect", ec.message()));
                vm_.run();

                //
                //	reconnect after 10/20 seconds
                //
                set_reconnect_timer(
                    (ec == boost::asio::error::connection_reset) ? boost::asio::chrono::seconds(10)
                                                                 : boost::asio::chrono::seconds(20));
            }
        }
    }

    void bus::handle_write(state_ptr sp, const boost::system::error_code &ec) {
        //
        //  test if bus was stopped
        //
        if (!sp->is_stopped()) {

            if (!ec) {

                buffer_write_.pop_front();
                if (!buffer_write_.empty()) {
                    do_write(sp);
                }
            } else {
                CYNG_LOG_ERROR(logger_, "[cluster] " << tgl_.get() << " on write: " << ec.message());
                reset(sp, state_value::START);
            }
        }
    }

    void bus::req_subscribe(std::string table_name) {
        add_msg(state_holder_, cyng::serialize_invoke("db.req.subscribe", table_name, tag_));
    }

    void bus::req_db_insert(std::string const &table_name, cyng::key_t key, cyng::data_t data, std::uint64_t generation) {

        add_msg(state_holder_, cyng::serialize_invoke("db.req.insert", table_name, key, data, generation, tag_));
    }

    void bus::req_db_insert_auto(std::string const &table_name, cyng::data_t data) {

        add_msg(state_holder_, cyng::serialize_invoke("db.req.insert.auto", table_name, data, tag_));
    }

    void bus::req_db_update(std::string const &table_name, cyng::key_t key, cyng::param_map_t data) {

        //
        //	triggers a merge() on the receiver side
        //
        add_msg(state_holder_, cyng::serialize_invoke("db.req.update", table_name, key, data, tag_));
    }

    void bus::req_db_remove(std::string const &table_name, cyng::key_t key) {

        add_msg(state_holder_, cyng::serialize_invoke("db.req.remove", table_name, key, tag_));
    }

    void bus::req_db_clear(std::string const &table_name) {
        add_msg(state_holder_, cyng::serialize_invoke("db.req.clear", table_name, tag_));
    }

    void bus::pty_login(
        std::string name,
        std::string pwd,
        boost::uuids::uuid tag,
        std::string data_layer,
        boost::asio::ip::tcp::endpoint ep) {

        auto const srv = tgl_.get();
        add_msg(state_holder_, cyng::serialize_invoke("pty.req.login", tag, name, pwd, ep, data_layer));
    }

    void bus::pty_logout(boost::uuids::uuid dev, boost::uuids::uuid tag) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.req.logout", tag, dev));
    }

    void bus::pty_open_connection(std::string msisdn, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.open.connection", tag, dev, msisdn, token));
    }

    void bus::pty_res_open_connection(
        bool success,
        boost::uuids::uuid peer,   //	caller_vm
        boost::uuids::uuid dev,    //	callee dev-tag
        boost::uuids::uuid callee, //	callee vm-tag
        cyng::param_map_t &&token) {

        add_msg(
            state_holder_,
            cyng::serialize_forward(
                "pty.return.open.connection",
                peer, //	caller_vm
                success,
                dev,
                callee,
                token));
    }
    void bus::pty_close_connection(boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.close.connection", tag, dev, token));
    }

    void bus::pty_transfer_data(boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::buffer_t &&data) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.transfer.data", tag, dev, std::move(data)));
    }

    void bus::pty_reg_target(
        std::string name,
        std::uint16_t paket_size,
        std::uint8_t window_size,
        boost::uuids::uuid dev,
        boost::uuids::uuid tag,
        cyng::param_map_t &&token) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.register.target", tag, dev, name, paket_size, window_size, token));
    }

    void bus::pty_dereg_target(std::string name, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.deregister", tag, dev, name, token));
    }

    //	"pty.open.channel"
    void bus::pty_open_channel(
        std::string name,
        std::string account,
        std::string msisdn,
        std::string version,
        std::string id,
        std::chrono::seconds timeout,
        boost::uuids::uuid dev,
        boost::uuids::uuid tag,
        cyng::param_map_t &&token) {

        add_msg(
            state_holder_,
            cyng::serialize_invoke("pty.open.channel", tag, dev, name, account, msisdn, version, id, timeout, token));
    }

    //	"pty.close.channel"
    void bus::pty_close_channel(std::uint32_t channel, boost::uuids::uuid dev, boost::uuids::uuid tag, cyng::param_map_t &&token) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.close.channel", tag, dev, channel, token));
    }

    //	"pty.push.data.req"
    void bus::pty_push_data(
        std::uint32_t channel,
        std::uint32_t source,
        cyng::buffer_t data,
        boost::uuids::uuid dev,
        boost::uuids::uuid tag,
        cyng::param_map_t &&token) {

        add_msg(state_holder_, cyng::serialize_invoke("pty.push.data.req", tag, dev, channel, source, data, token));
    }

    void bus::push_sys_msg(std::string msg, cyng::severity level) {
        add_msg(state_holder_, cyng::serialize_invoke("sys.msg", msg, level));
    }

    void bus::add_msg(state_ptr sp, std::deque<cyng::buffer_t> &&msg) {

        cyng::exec(vm_, [=, this]() {
            bool const b = buffer_write_.empty();
            cyng::add(buffer_write_, msg);
            if (b && sp) {
                do_write(sp);
            }
        });
    }

    void bus::update_pty_counter(std::uint64_t count) {

        req_db_update("cluster", cyng::key_generator(tag_), cyng::param_map_factory("clients", count));
    }

    void bus::on_ping(std::chrono::system_clock::time_point tp) {
        CYNG_LOG_TRACE(logger_, "ping: " << tp);
        //
        //  echo
        //
        // send_cluster_response(cyng::serialize_invoke("cluster.res.ping", tp));
        add_msg(state_holder_, cyng::serialize_invoke("cluster.res.ping", tp));
    }

    std::function<void(bool)> bus::get_vm_func_on_login(bus_interface *bip) {
        return std::bind(&bus_interface::on_login, bip, std::placeholders::_1);
    }

    std::function<void(std::chrono::system_clock::time_point)> bus::get_vm_func_on_ping(bus *bp) {
        return std::bind(&bus::on_ping, bp, std::placeholders::_1);
    }

    std::function<void(std::string)> bus::get_vm_func_on_disconnect(bus_interface *bip) {
        return std::bind(&bus_interface::on_disconnect, bip, std::placeholders::_1);
    }

    std::function<void(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag)>
    bus::get_vm_func_db_res_insert(bus_interface *bip) {
        return std::bind(
            &bus_interface::db_res_insert,
            bip,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5);
    }

    std::function<void(std::string, bool)> bus::get_vm_func_db_res_trx(bus_interface *bip) {
        return std::bind(&bus_interface::db_res_trx, bip, std::placeholders::_1, std::placeholders::_2);
    }

    std::function<void(std::string, cyng::key_t key, cyng::attr_t attr_t, std::uint64_t gen, boost::uuids::uuid tag)>
    bus::get_vm_func_db_res_update(bus_interface *bip) {
        return std::bind(
            &bus_interface::db_res_update,
            bip,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4,
            std::placeholders::_5);
    }

    std::function<void(std::string, cyng::key_t key, boost::uuids::uuid tag)> bus::get_vm_func_db_res_remove(bus_interface *bip) {
        return std::bind(&bus_interface::db_res_remove, bip, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    std::function<void(std::string, boost::uuids::uuid tag)> bus::get_vm_func_db_res_clear(bus_interface *bip) {
        return std::bind(&bus_interface::db_res_clear, bip, std::placeholders::_1, std::placeholders::_2);
    }

    bus::state::state(boost::asio::ip::tcp::resolver::results_type &&res)
        : value_(state_value::START)
        , endpoints_(std::move(res)) {}

} // namespace smf
