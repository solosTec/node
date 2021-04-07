/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/cluster/bus.h>
#include <smf.h>

#include <cyng/log/record.h>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/generator.hpp>
#include <cyng/sys/process.h>
#include <cyng/io/serialize.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>
#include <cyng/obj/algorithm/add.hpp>

#include <boost/bind.hpp>

namespace smf {

	bus::bus(boost::asio::io_context& ctx
		, cyng::logger logger
		, toggle::server_vec_t&& tgl
		, std::string const& node_name
		, boost::uuids::uuid tag
		, bus_interface* bip)
	: state_(state::START)
		, ctx_(ctx)
		, logger_(logger)
		, tgl_(std::move(tgl))
		, node_name_(node_name)
		, tag_(tag)
		, bip_(bip)
		, endpoints_()
		, socket_(ctx_)
		, timer_(ctx_)
		, buffer_write_()
		, input_buffer_()
		, vm_()
		, parser_([this](cyng::object&& obj) {
			CYNG_LOG_DEBUG(logger_, "[cluster] parser: " << cyng::io::to_typed(obj));
			vm_.load(std::move(obj));
		})
	{
		vm_ = init_vm(bip);
		vm_.set_channel_name("cluster.res.login", 0);
		vm_.set_channel_name("cluster.disconnect", 1);
		vm_.set_channel_name("db.res.insert", 2);
		vm_.set_channel_name("db.res.trx", 3);
		vm_.set_channel_name("db.res.update", 4);
		vm_.set_channel_name("db.res.remove", 5);
		vm_.set_channel_name("db.res.clear", 6);
		vm_.set_channel_name("pty.res.login", 7);
	}

	cyng::vm_proxy bus::init_vm(bus_interface* bip) {

		return bip->get_fabric()->create_proxy(
			get_vm_func_on_login(bip),			//	"cluster.res.login"
			get_vm_func_on_disconnect(bip),		//	"cluster.disconnect"
			get_vm_func_db_res_insert(bip),		//	"db.res.insert"
			get_vm_func_db_res_trx(bip),		//	"db.res.trx"
			get_vm_func_db_res_update(bip),		//	"db.res.update"
			get_vm_func_db_res_remove(bip),		//	"db.res.remove"
			get_vm_func_db_res_clear(bip)		//	"db.res.clear"
		);
	}

	void bus::start() {

		auto const srv = tgl_.get();
		CYNG_LOG_INFO(logger_, "[cluster] connect(" << srv << ")");
		state_ = state::START;

		//
		//	connect to cluster
		//
		boost::asio::ip::tcp::resolver r(ctx_);
		connect(r.resolve(srv.host_, srv.service_));

	}

	void bus::stop() {
		CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " stop");

		reset();
		state_ = state::STOPPED;
	}

	boost::uuids::uuid bus::get_tag() const {
		return tag_;
	}

	void bus::reset() {

		boost::system::error_code ignored_ec;
		socket_.close(ignored_ec);
		state_ = state::START;
		timer_.cancel();

	}

	void bus::check_deadline(const boost::system::error_code& ec) {
		if (is_stopped())	return;
		CYNG_LOG_TRACE(logger_, "[cluster] check deadline " << ec);

		if (!ec) {
			switch (state_) {
			case state::START:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: start");
				start();
				break;
			case state::CONNECTED:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: connected");
				break;
			case state::WAIT:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: waiting");
				start();
				break;
			default:
				CYNG_LOG_TRACE(logger_, "[cluster] check deadline: other");
				BOOST_ASSERT_MSG(false, "invalid state");
				break;
			}
		}
		else {
			CYNG_LOG_TRACE(logger_, "[cluster] check deadline timer cancelled");
		}
	}

	void bus::connect(boost::asio::ip::tcp::resolver::results_type endpoints) {

		state_ = state::WAIT;

		// Start the connect actor.
		endpoints_ = endpoints;
		start_connect(endpoints_.begin());

		// Start the deadline actor. You will note that we're not setting any
		// particular deadline here. Instead, the connect and input actors will
		// update the deadline prior to each asynchronous operation.
		timer_.async_wait(boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error));

	}

	void bus::start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {
		if (endpoint_iter != endpoints_.end()) {
			CYNG_LOG_TRACE(logger_, "[cluster] trying " << endpoint_iter->endpoint() << "...");

			// Set a deadline for the connect operation.
			timer_.expires_after(std::chrono::seconds(60));

			// Start the asynchronous connect operation.
			socket_.async_connect(endpoint_iter->endpoint(),
				std::bind(&bus::handle_connect, this,
					std::placeholders::_1, endpoint_iter));
		}
		else {

			//
			// There are no more endpoints to try. Shut down the client.
			//
			reset();

			//
			//	alter connection endpoint
			//
			tgl_.changeover();
			CYNG_LOG_WARNING(logger_, "[cluster] connect failed - switch to " << tgl_.get());

			//
			//	reconnect after 20 seconds
			//
			timer_.expires_after(boost::asio::chrono::seconds(20));
			timer_.async_wait(boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error));

		}
	}

	void bus::handle_connect(const boost::system::error_code& ec,
		boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter) {

		if (is_stopped())	return;

		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation. If the socket is closed at this time then
		// the timeout handler must have run first.
		if (!socket_.is_open()) {

			CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect timed out");

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Check if the connect operation failed before the deadline expired.
		else if (ec)
		{
			CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " connect error " << ec.value() << ": " << ec.message());

			// We need to close the socket used in the previous connection attempt
			// before starting a new one.
			socket_.close();

			// Try the next available endpoint.
			start_connect(++endpoint_iter);
		}

		// Otherwise we have successfully established a connection.
		else
		{
			CYNG_LOG_INFO(logger_, "[cluster] " << tgl_.get() << " connected to " << endpoint_iter->endpoint());
			state_ = state::CONNECTED;

			//
			//	send login sequence
			//
			auto cfg = tgl_.get();
			add_msg(cyng::serialize_invoke("cluster.req.login"
				, cfg.account_
				, cfg.pwd_
				, cyng::sys::get_process_id()
				, node_name_
				, tag_
				, cyng::version(SMF_VERSION_MAJOR, SMF_VERSION_MINOR)));

			// Start the input actor.
			do_read();

			// Start the heartbeat actor.
			//do_write();
		}

	}

	void bus::do_read()
	{
		//
		//	connect was successful
		//

		// Start an asynchronous operation to read a newline-delimited message.
		socket_.async_read_some(boost::asio::buffer(input_buffer_), std::bind(&bus::handle_read, this,
			std::placeholders::_1, std::placeholders::_2));
	}

	void bus::do_write()
	{
		if (is_stopped())	return;

		BOOST_ASSERT(!buffer_write_.empty());

		//
		//	write actually data to socket
		//
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			cyng::expose_dispatcher(vm_).wrap(std::bind(&bus::handle_write, this, std::placeholders::_1)));
	}

	void bus::handle_read(const boost::system::error_code& ec, std::size_t bytes_transferred)
	{
		if (is_stopped())	return;

		if (!ec)
		{
			CYNG_LOG_DEBUG(logger_, "[cluster] " << tgl_.get() << " received " << bytes_transferred << " bytes");

			//
			//	let parse it
			//
			parser_.read(input_buffer_.begin(), input_buffer_.begin() + bytes_transferred);

			//
			//	continue reading
			//
			do_read();
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "[cluster] " << tgl_.get() << " read " << ec.value() << ": " << ec.message());
			reset();

			//
			//	call disconnect function
			//
			vm_.load(cyng::generate_invoke("cluster.disconnect", ec.message()));
			vm_.run();

			//
			//	reconnect after 10/20 seconds
			//
			timer_.expires_after((ec == boost::asio::error::connection_reset)
				? boost::asio::chrono::seconds(10)
				: boost::asio::chrono::seconds(20));
			timer_.async_wait(boost::bind(&bus::check_deadline, this, boost::asio::placeholders::error));

		}
	}

	void bus::handle_write(const boost::system::error_code& ec)
	{
		if (is_stopped())	return;

		if (!ec) {

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
			else {

				// Wait 10 seconds before sending the next heartbeat.
				//heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
				//heartbeat_timer_.async_wait(std::bind(&bus::do_write, this));
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[cluster] " << tgl_.get() << " on heartbeat: " << ec.message());

			reset();
			//auto sp = channel_.lock();
			//if (sp)	sp->suspend(std::chrono::seconds(12), "start", cyng::make_tuple());
		}
	}

	void bus::req_subscribe(std::string table_name) {

		add_msg(cyng::serialize_invoke("db.req.subscribe"
			, table_name
			, tag_));

	}

	void bus::req_db_insert(std::string const& table_name
		, cyng::key_t key
		, cyng::data_t  data
		, std::uint64_t generation) {

		add_msg(cyng::serialize_invoke("db.req.insert"
			, table_name
			, key
			, data
			, generation
			, tag_));
	}

	void bus::req_db_update(std::string const& table_name
		, cyng::key_t key
		, cyng::param_map_t data) {

		//
		//	triggers a merge() on the receiver side
		//
		add_msg(cyng::serialize_invoke("db.req.update"
			, table_name
			, key
			, data
			, tag_));

	}

	void bus::req_db_remove(std::string const& table_name
		, cyng::key_t key) {

		add_msg(cyng::serialize_invoke("db.req.remove"
			, table_name
			, key
			, tag_));

	}

	void bus::req_db_clear(std::string const& table_name) {

		add_msg(cyng::serialize_invoke("db.req.clear"
			, table_name
			, tag_));
	}

	void bus::pty_login(std::string name
		, std::string pwd
		, boost::uuids::uuid tag
		, std::string data_layer
		, boost::asio::ip::tcp::endpoint ep) {

		auto const srv = tgl_.get();
		add_msg(cyng::serialize_invoke("pty.req.login"
			, tag
			, name
			, pwd
			, ep
			, data_layer));

	}

	void bus::pty_connect(std::string msisdn, boost::uuids::uuid tag) {

		add_msg(cyng::serialize_invoke("pty.connect"
			, tag
			, msisdn));

	}

	void bus::pty_disconnect(boost::uuids::uuid tag) {

		add_msg(cyng::serialize_invoke("pty.disconnect"
			, tag));
	}

	void bus::pty_reg_target(std::string name
		, std::uint16_t paket_size
		, std::uint8_t window_size
		, boost::uuids::uuid dev
		, boost::uuids::uuid tag
		, cyng::param_map_t&& token) {

		add_msg(cyng::serialize_invoke("pty.register"
			, tag
			, dev
			, name
			, paket_size
			, window_size
			, token));
	}

	void bus::pty_dereg_target(std::string name
		, boost::uuids::uuid dev
		, boost::uuids::uuid tag
		, cyng::param_map_t&& token) {

		add_msg(cyng::serialize_invoke("pty.deregister"
			, tag
			, dev
			, name
			, token));
	}

	//	"pty.open.channel"
	void bus::pty_open_channel(std::string name
		, std::string account
		, std::string msisdn
		, std::string version
		, std::string id
		, std::chrono::seconds timeout
		, boost::uuids::uuid dev
		, boost::uuids::uuid tag
		, cyng::param_map_t&& token) {

		add_msg(cyng::serialize_invoke("pty.open.channel"
			, tag
			, dev
			, name
			, account
			, msisdn
			, version 
			, id 
			, timeout
			, token));
	}

	//	"pty.close.channel"
	void bus::pty_close_channel(std::uint32_t channel
		, boost::uuids::uuid dev
		, boost::uuids::uuid tag
		, cyng::param_map_t&& token) {

		add_msg(cyng::serialize_invoke("pty.close.channel"
			, tag
			, dev
			, channel
			, token));
	}


	void bus::push_sys_msg(std::string msg, cyng::severity level) {

		add_msg(cyng::serialize_invoke("sys.msg"
			, msg
			, level));

	}

	void bus::add_msg(std::deque<cyng::buffer_t>&& msg) {

		cyng::exec(vm_, [=, this]() {
			bool const b = buffer_write_.empty();
			cyng::add(buffer_write_, msg);
			if (b)	do_write();
			});

	}

	std::function<void(bool)>
	bus::get_vm_func_on_login(bus_interface* bip) {
		return std::bind(&bus_interface::on_login, bip, std::placeholders::_1);
	}

	std::function<void(std::string)>
	bus::get_vm_func_on_disconnect(bus_interface* bip) {
		return std::bind(&bus_interface::on_disconnect, bip, std::placeholders::_1);
	}

	std::function<void(std::string
		, cyng::key_t key
		, cyng::data_t data
		, std::uint64_t gen
		, boost::uuids::uuid tag)>
	bus::get_vm_func_db_res_insert(bus_interface* bip) {
		return std::bind(&bus_interface::db_res_insert, bip, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	}

	std::function<void(std::string
		, bool)>
	bus::get_vm_func_db_res_trx(bus_interface* bip) {
		return std::bind(&bus_interface::db_res_trx, bip, std::placeholders::_1, std::placeholders::_2);
	}

	std::function<void(std::string
		, cyng::key_t key
		, cyng::attr_t attr_t
		, std::uint64_t gen
		, boost::uuids::uuid tag)>
	bus::get_vm_func_db_res_update(bus_interface* bip) {
		return std::bind(&bus_interface::db_res_update, bip, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	}

	std::function<void(std::string
		, cyng::key_t key
		, boost::uuids::uuid tag)>
	bus::get_vm_func_db_res_remove(bus_interface* bip) {
		return std::bind(&bus_interface::db_res_remove, bip, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	std::function<void(std::string
		, boost::uuids::uuid tag)>
	bus::get_vm_func_db_res_clear(bus_interface* bip) {
		return std::bind(&bus_interface::db_res_clear, bip, std::placeholders::_1, std::placeholders::_2);
	}

}

