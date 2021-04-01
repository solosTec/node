/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <session.h>
#include <tasks/server.h>

#include <cyng/log/record.h>
#include <cyng/io/serialize.h>
#include <cyng/io/ostream.h>
#include <cyng/vm/vm.h>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/generator.hpp>
#include <cyng/vm/mesh.h>
#include <cyng/obj/algorithm/add.hpp>

#include <iostream>

namespace smf {

	session::session(boost::asio::ip::tcp::socket socket, server* srv, cyng::logger logger)
	: socket_(std::move(socket))
		, srvp_(srv)
		, logger_(logger)
		, buffer_()
		, buffer_write_()
		, vm_()
		, parser_([this](cyng::object&& obj) {
			CYNG_LOG_DEBUG(logger_, "parser: " << cyng::io::to_typed(obj));
			vm_.load(std::move(obj));
		})
		, slot_(cyng::make_slot(new slot(this)))
		, peer_(boost::uuids::nil_uuid())
		, protocol_layer_("any")
	{
		vm_ = init_vm(srv);
		vm_.set_channel_name("cluster.req.login", 0);
		vm_.set_channel_name("db.req.subscribe", 1);
		vm_.set_channel_name("db.req.insert", 2);
		vm_.set_channel_name("db.req.update", 3);	//	table modify()
		vm_.set_channel_name("db.req.remove", 4);	//	table erase()
		vm_.set_channel_name("db.req.clear", 5);	//	table clear()
		vm_.set_channel_name("pty.req.login", 6);
		vm_.set_channel_name("pty.connect", 6);
	}

	session::~session()
	{
#ifdef _DEBUG_MAIN
		std::cout << "session(~)" << std::endl;
#endif
	}

	boost::uuids::uuid session::get_peer() const {
		return peer_;
	}

	void session::stop()
	{
		//	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
		CYNG_LOG_WARNING(logger_, "stop session");
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
		socket_.close(ec);

		vm_.stop();
	}

	void session::start()
	{
		do_read();
	}

	void session::do_read() {
		auto self = shared_from_this();

		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

				if (!ec) {
					CYNG_LOG_DEBUG(logger_, "session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes");

					//
					//	let parse it
					//
					parser_.read(buffer_.begin(), buffer_.begin() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "[session] read: " << ec.message());
				}

		});
	}

	cyng::vm_proxy session::init_vm(server* srv) {

		return srv->fabric_.create_proxy(get_vm_func_cluster_req_login(this)
			, get_vm_func_db_req_subscribe(this)
			, get_vm_func_db_req_insert(this)
			, get_vm_func_db_req_update(this)
			, get_vm_func_db_req_remove(this)
			, get_vm_func_db_req_clear(this)
			, get_vm_func_pty_login(this)
			, get_vm_func_pty_connect(srv));
	}

	void session::do_write()
	{
		BOOST_ASSERT(!buffer_write_.empty());

		//
		//	write actually data to socket
		//
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			cyng::expose_dispatcher(vm_).wrap(std::bind(&session::handle_write, this, std::placeholders::_1)));
	}

	void session::handle_write(const boost::system::error_code& ec)
	{
		if (!ec) {

			BOOST_ASSERT(!buffer_write_.empty());
			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[session] write: " << ec.message());

			//reset();
		}
	}

	void session::cluster_login(std::string name, std::string pwd, cyng::pid n, std::string node, boost::uuids::uuid tag, cyng::version v) {
		CYNG_LOG_INFO(logger_, "session [" 
			<< socket_.remote_endpoint() 
			<< "] cluster login " 
			<< name << ":" 
			<< pwd 
			<< "@" << node 
			<< " #" << n.get_internal_value()
			<< " v"
			<< v);

		//
		//	ToDo: check credentials
		//

		//
		//	insert into cluster table
		//
		BOOST_ASSERT(!tag.is_nil());
		peer_ = tag;
		protocol_layer_ = node;
		srvp_->cache_.insert_cluster_member(tag, node, v, socket_.remote_endpoint(), n);

		//
		//	send response
		//
		auto const deq = cyng::serialize_invoke("cluster.res.login", true);
		cyng::exec(vm_, [=, this]() {
			bool const b = buffer_write_.empty();
			cyng::add(buffer_write_, deq);
			if (b)	do_write();
		});

	}

	void session::db_req_subscribe(std::string table, boost::uuids::uuid tag) {

		CYNG_LOG_INFO(logger_, "session ["
			<< socket_.remote_endpoint()
			<< "] subscribe "
			<< table );

		srvp_->store_.connect(table, slot_);

	}
	void session::db_req_insert(std::string const& table_name
		, cyng::key_t key
		, cyng::data_t data
		, std::uint64_t generation
		, boost::uuids::uuid tag) {

		CYNG_LOG_INFO(logger_, "session ["
			<< socket_.remote_endpoint()
			<< "] insert "
			<< table_name
			<< " - "
			<< data);

		std::reverse(key.begin(), key.end());
		std::reverse(data.begin(), data.end());
		srvp_->store_.insert(table_name, key, data, generation, tag);

	}
	void session::db_req_update(std::string const& table_name
		, cyng::key_t key
		, cyng::param_map_t data
		, boost::uuids::uuid source) {

		//
		//	modify multiple 
		// 
		std::reverse(key.begin(), key.end());
		srvp_->store_.modify(table_name, key, data, source);


	}
	void session::db_req_remove(std::string const& table_name
		, cyng::key_t key
		, boost::uuids::uuid source) {

		std::reverse(key.begin(), key.end());
		srvp_->store_.erase(table_name, key, source);

	}
	void session::db_req_clear(std::string const& table_name
		, boost::uuids::uuid source) {

		srvp_->store_.clear(table_name, source);

	}

	//
	//	slot implementation
	//

	session::slot::slot(session* sp) 
		: sp_(sp)
	{}

	bool session::slot::forward(cyng::table const* tbl
		, cyng::key_t const& key
		, cyng::data_t const& data
		, std::uint64_t gen
		, boost::uuids::uuid source) {

		//
		//	send to subscriber
		//
		CYNG_LOG_TRACE(sp_->logger_, "forward insert ["
			<< tbl->meta().get_name()
			<< "]");

		auto const deq = cyng::serialize_invoke("db.res.insert"
			, tbl->meta().get_name()
			, key
			, data
			, gen
			, source);

		cyng::exec(sp_->vm_, [=, this]() {
			bool const b = sp_->buffer_write_.empty();
			cyng::add(sp_->buffer_write_, deq);
			if (b)	sp_->do_write();
		});


		return true;
	}

	bool session::slot::forward(cyng::table const* tbl
			, cyng::key_t const& key
			, cyng::attr_t const& attr
			, std::uint64_t gen
			, boost::uuids::uuid source) {

		//
		//	send update to subscriber
		//
		CYNG_LOG_TRACE(sp_->logger_, "forward update ["
			<< tbl->meta().get_name()
			<< "]");

		auto const deq = cyng::serialize_invoke("db.res.update"
			, tbl->meta().get_name()
			, key
			, attr
			, gen
			, source);

		cyng::exec(sp_->vm_, [=, this]() {
			bool const b = sp_->buffer_write_.empty();
			cyng::add(sp_->buffer_write_, deq);
			if (b)	sp_->do_write();
			});

		return true;
	}

	bool session::slot::forward(cyng::table const* tbl
			, cyng::key_t const& key
			, boost::uuids::uuid source) {
		//
		//	send remove to subscriber
		//
		CYNG_LOG_TRACE(sp_->logger_, "forward remove ["
			<< tbl->meta().get_name()
			<< "]");

		auto const deq = cyng::serialize_invoke("db.res.remove"
			, tbl->meta().get_name()
			, key
			, source);

		cyng::exec(sp_->vm_, [=, this]() {
			bool const b = sp_->buffer_write_.empty();
			cyng::add(sp_->buffer_write_, deq);
			if (b)	sp_->do_write();
			});

		return true;
	}

	bool session::slot::forward(cyng::table const* tbl
			, boost::uuids::uuid source) {

		//
		//	send clear to subscriber
		//

		CYNG_LOG_TRACE(sp_->logger_, "forward clear ["
			<< tbl->meta().get_name()
			<< "]");

		auto const deq = cyng::serialize_invoke("db.res.clear"
			, tbl->meta().get_name()
			, source);

		cyng::exec(sp_->vm_, [=, this]() {
			bool const b = sp_->buffer_write_.empty();
			cyng::add(sp_->buffer_write_, deq);
			if (b)	sp_->do_write();
			});

		return true;
	}

	bool session::slot::forward(cyng::table const* tbl
		, bool trx) {

		CYNG_LOG_TRACE(sp_->logger_, "forward trx ["
			<< tbl->meta().get_name()
			<< "] "
			<< (trx ? "start" : "commit"));

		auto const deq = cyng::serialize_invoke("db.res.trx"
			, tbl->meta().get_name()
			, trx);

		cyng::exec(sp_->vm_, [=, this]() {
			bool const b = sp_->buffer_write_.empty();
			cyng::add(sp_->buffer_write_, deq);
			if (b)	sp_->do_write();
		});

		return true;
	}

	void session::pty_login(boost::uuids::uuid tag
		, std::string name
		, std::string pwd
		, boost::asio::ip::tcp::endpoint ep
		, std::string data_layer) {

		CYNG_LOG_INFO(logger_, "pty login " << name << ':' << pwd << '@' << ep);


		//
		//	check credentials and get associated device
		// 
		//auto const dev = srvp_->cache_.lookup_device(name, pwd);
		auto const[dev, enabled] = srvp_->cache_.lookup_device(name, pwd);
		if (!dev.is_nil()) {

			//
			//	ToDo: check if there is already a session of this pty
			// 

			//
			//	insert into session table
			//
			srvp_->cache_.insert_pty(dev, peer_, tag, name, pwd, ep, data_layer);

			//
			//	send response
			// 
			auto const deq = cyng::serialize_forward("pty.res.login"
				, tag
				, true
				, dev);

			cyng::exec(vm_, [=, this]() {
				bool const b = buffer_write_.empty();
				cyng::add(buffer_write_, deq);
				if (b)	do_write();
				});

			//
			//	update cluster table (pty counter)
			// 
			srvp_->cache_.update_pty_counter(peer_);

		}
		else {
			CYNG_LOG_WARNING(logger_, "pty login [" << name << "] failed");

			//
			//	send response
			// 
			auto const deq = cyng::serialize_forward("pty.res.login"
				, tag
				, false
				, boost::uuids::nil_uuid());	//	no device

			cyng::exec(vm_, [=, this]() {
				bool const b = buffer_write_.empty();
				cyng::add(buffer_write_, deq);
				if (b)	do_write();
				});

			//	check auto insert
			auto const auto_enabled = srvp_->cache_.get_cfg().get_value("auto-enabled", false);
			if (auto_enabled) {
				CYNG_LOG_INFO(logger_, "auto-enabled is ON - insert [" << name << "] into device table");
				srvp_->cache_.insert_device(tag, name, pwd, true);
			}
		}

	}

	std::function<void(std::string
		, std::string
		, cyng::pid
		, std::string
		, boost::uuids::uuid
		, cyng::version)> 
	session::get_vm_func_cluster_req_login(session* ptr) {
		return std::bind(&session::cluster_login, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
	}

	std::function<void(std::string
		, boost::uuids::uuid tag)>  
	session::get_vm_func_db_req_subscribe(session* ptr) {
		return std::bind(&session::db_req_subscribe, ptr, std::placeholders::_1, std::placeholders::_2);
	}

	std::function<void(std::string
		, cyng::key_t
		, cyng::data_t
		, std::uint64_t
		, boost::uuids::uuid)>
	session::get_vm_func_db_req_insert(session* ptr) {
		return std::bind(&session::db_req_insert, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	}

	//	"db.req.update" aka merge()
	std::function<void(std::string
		, cyng::key_t
		, cyng::param_map_t
		, boost::uuids::uuid)>
	session::get_vm_func_db_req_update(session* ptr) {
		return std::bind(&session::db_req_update, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	}

	//	"db.req.remove"
	std::function<void(std::string
		, cyng::key_t
		, boost::uuids::uuid)>
	session::get_vm_func_db_req_remove(session* ptr) {
		return std::bind(&session::db_req_remove, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	//	"db.req.clear"
	std::function<void(std::string
		, boost::uuids::uuid)>
	session::get_vm_func_db_req_clear(session* ptr) {
		return std::bind(&session::db_req_clear, ptr, std::placeholders::_1, std::placeholders::_2);
	}

	std::function<void(boost::uuids::uuid
		, std::string
		, std::string
		, boost::asio::ip::tcp::endpoint
		, std::string)>
	session::get_vm_func_pty_login(session* ptr) {
		return std::bind(&session::pty_login, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	}

	std::function<void(boost::uuids::uuid
		, std::string)>
	session::get_vm_func_pty_connect(server* ptr) {
		return std::bind(&server::pty_connect, ptr, std::placeholders::_1, std::placeholders::_2);
	}



}


