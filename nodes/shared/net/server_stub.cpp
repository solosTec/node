/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include <smf/cluster/server_stub.h>
#include <smf/cluster/generator.h>
#include <cyng/vm/generator.h>
#include <cyng/tuple_cast.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	server_stub::server_stub(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, std::chrono::seconds timeout)
	: mux_(mux)
		, logger_(logger)
		, bus_(bus)
		, timeout_(timeout)
		, acceptor_(mux.get_io_service())
#if (BOOST_VERSION < 106600)
        , socket_(mux_.get_io_service())
#endif
		, rnd_()
		, client_map_()
		, connection_map_()
		, cv_acceptor_closed_()
		, cv_sessions_closed_()
		, mutex_()
	{
		//
		//	connection management
		//
		bus_->vm_.register_function("push.connection", 1, std::bind(&server_stub::push_connection, this, std::placeholders::_1));
		bus_->vm_.register_function("server.insert.client", 2, std::bind(&server_stub::insert_client, this, std::placeholders::_1));
		bus_->vm_.register_function("server.remove.client", 1, std::bind(&server_stub::remove_client, this, std::placeholders::_1));
		bus_->vm_.register_function("server.close.client", 2, std::bind(&server_stub::close_client, this, std::placeholders::_1));
		bus_->vm_.register_function("server.shutdown.clients", 0, std::bind(&server_stub::shutdown_clients, this, std::placeholders::_1));
		bus_->vm_.register_function("server.clear.connection.map", 1, std::bind(&server_stub::clear_connection_map, this, std::placeholders::_1));
		bus_->vm_.register_function("server.transmit.data", 2, std::bind(&server_stub::transmit_data, this, std::placeholders::_1));


		//
		//	client responses
		//
		bus_->vm_.register_function("client.res.login", 7, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.res.close", 3, std::bind(&server_stub::client_res_close_impl, this, std::placeholders::_1));
		bus_->vm_.register_function("client.req.close", 4, std::bind(&server_stub::client_req_close_impl, this, std::placeholders::_1));

		bus_->vm_.register_function("client.res.open.push.channel", 7, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.res.register.push.target", 1, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.res.deregister.push.target", 6, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));

		bus_->vm_.register_function("client.res.open.connection", 6, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.req.open.connection.forward", 6, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.res.open.connection.forward", 6, std::bind(&server_stub::client_res_open_connection_forward, this, std::placeholders::_1));
		bus_->vm_.register_function("client.req.transmit.data.forward", 5, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.res.transfer.pushdata", 7, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.req.transfer.pushdata.forward", 7, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.res.close.push.channel", 6, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.req.close.connection.forward", 7, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));
		bus_->vm_.register_function("client.res.close.connection.forward", 6, std::bind(&server_stub::client_propagate, this, std::placeholders::_1));

		//
		//	statistical data
		//
		bus_->vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));
	}

	void server_stub::run(std::string const& address, std::string const& service)
	{
		CYNG_LOG_TRACE(logger_, "resolve address "
			<< address
			<< ':'
			<< service);

		try {
			// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
			boost::asio::ip::tcp::resolver resolver(mux_.get_io_service());
#if (BOOST_VERSION >= 106600)
			boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, service).begin();
#else
			boost::asio::ip::tcp::resolver::query query(address, service);
			boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
#endif
			acceptor_.open(endpoint.protocol());
			acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor_.bind(endpoint);
			acceptor_.listen();

			CYNG_LOG_INFO(logger_, "listen "
				<< endpoint.address().to_string()
				<< ':'
				<< endpoint.port());

			do_accept();
		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger_, "resolve address "
				<< address
				<< ':'
				<< service
				<< " failed: "
				<< ex.what());
		}
	}

	void server_stub::create_client(boost::asio::ip::tcp::socket socket)
	{
		if (bus_->is_online()) {

			const auto tag = rnd_();

			CYNG_LOG_TRACE(logger_, "accept "
				<< socket.remote_endpoint()
				<< " - "
				<< tag);

			//
			//	create new connection/session
			//	bus is synchronizing access to client_map_
			//
			bus_->vm_.async_run(cyng::generate_invoke("server.insert.client", tag, make_client(tag, std::move(socket))));
		}
		else {

			CYNG_LOG_WARNING(logger_, "do not accept incoming IP-T connection from "
				<< socket.remote_endpoint()
				<< " because there is no access to SMF cluster");

			boost::system::error_code ec;
			socket.close(ec);
		}
	}

	void server_stub::do_accept()
	{
		static_assert(BOOST_VERSION >= 106600, "boost library 1.66 or higher required");
		acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {

			if (!ec)	{

				//
				//	create a new session and insert into client_map_
				//
				create_client(std::move(socket));

				//
				//
				//	continue accepting
				//
				if (acceptor_.is_open()) {
					do_accept();
				}
			}
			else {

				//	 system:995 - The I/O operation has been aborted because of either a thread exit or an application request
				CYNG_LOG_WARNING(logger_, "accept: "
					<< ec
					<< " - "
					<< ec.message());

				//
				//	notify
				//
				cyng::async::lock_guard<cyng::async::mutex> lk(mutex_);
				cv_acceptor_closed_.notify_all();
			}

		});
	}

	void server_stub::close()
	{
		//
		//	close acceptor
		//	no more incoming connections
		//
		close_acceptor();

		//
		//	close existing connection
		//
		close_clients();
	}

	void server_stub::close_clients()
	{
		CYNG_LOG_INFO(logger_, "close "
			<< client_map_.size()
			<< " clients");

		//
		// close all clients
		//
		cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);
		bus_->vm_.async_run(cyng::generate_invoke("server.shutdown.clients"));

		//
		//	wait for pending ipt connections
		//
		CYNG_LOG_TRACE(logger_, "server_stub is waiting for "
			<< client_map_.size()
			<< " clients to shutdown");
		if (cv_sessions_closed_.wait_for(lock, timeout_, [this] {
			return client_map_.empty();
		})) {

			//
			//	all clients stopped
			//
			CYNG_LOG_INFO(logger_, "server_stub shutdown complete");
		}
		else {
			CYNG_LOG_ERROR(logger_, "server_stub shutdown timeout. "
				<< client_map_.size()
				<< " clients still running");
		}
	}

	void server_stub::close_acceptor()
	{
		CYNG_LOG_INFO(logger_, "close acceptor");

		// The server_stub is stopped by cancelling all outstanding asynchronous
		// operations. Once all operations have finished the io_context::run()
		// call will exit.
		cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);
		acceptor_.cancel();
		acceptor_.close();

		//
		//	wait for cancellation of accept operation
		//
		CYNG_LOG_TRACE(logger_, "server_stub is waiting for acceptor cancellation");
		cv_acceptor_closed_.wait(lock, [this] {
			return !acceptor_.is_open();
		});

		//
		//	no more incoming connections
		//
		CYNG_LOG_INFO(logger_, "acceptor cancellation complete");
	}

	void server_stub::shutdown_clients(cyng::context& ctx)
	{
		CYNG_LOG_INFO(logger_, "close "
			<< client_map_.size()
			<< " ipt client(s)");

		if (!client_map_.empty()) {
			for(auto& conn : client_map_)     {

				//
				//	close sockets
				//
				close_connection(conn.first, conn.second);
				//const_cast<connection*>(cyng::object_cast<connection>(conn.second))->close();
			}
		}
		else {

			//
			//	notify
			//
			cyng::async::lock_guard<cyng::async::mutex> lk(mutex_);
			cv_sessions_closed_.notify_all();
		}
	}


	void server_stub::insert_client(cyng::context& ctx)
	{
		BOOST_ASSERT(bus_->vm_.tag() == ctx.tag());

		//	[a95f46e9-eccd-4b47-b02c-17d5172218af,<!260:ipt::connection>]
		const cyng::vector_t frame = ctx.get_frame();

		auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		if (!insert_client_impl(tag, frame.at(1))) {
			ctx.attach(cyng::generate_invoke("log.msg.error", "server.insert.connection - failed", frame));
		}
	}

	bool server_stub::insert_client_impl(boost::uuids::uuid tag, cyng::object obj)
	{
		auto r = client_map_.emplace(tag, obj);
		if (r.second) {

			//
			//	start new IP-T session
			//
			start_client((*r.first).second);
			//const_cast<connection*>(cyng::object_cast<connection>((*r.first).second))->start();

			CYNG_LOG_TRACE(logger_, client_map_.size()
				<< " ipt sessions open with "
				<< (connection_map_.size() / 2)
				<< " connections");

			return true;
		}
		return false;
	}

	void server_stub::remove_client(cyng::context& ctx)
	{
		BOOST_ASSERT(bus_->vm_.tag() == ctx.tag());

		//	[a95f46e9-eccd-4b47-b02c-17d5172218af]
		const cyng::vector_t frame = ctx.get_frame();

		const auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		if (!remove_client_impl(tag)) {
			ctx.attach(cyng::generate_invoke("log.msg.error", "server.remove.client - failed", tag));
		}
	}

	void server_stub::clear_connection_map(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		const auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		clear_connection_map_impl(tag);
	}

	bool server_stub::clear_connection_map_impl(boost::uuids::uuid tag)
	{
		auto pos = connection_map_.find(tag);
		if (pos != connection_map_.end())
		{
			auto receiver = pos->second;

			CYNG_LOG_INFO(logger_, "remove "
				<< tag
				<< " <==> "
				<< receiver
				<< " from (local) connection map with "
				<< connection_map_.size()
				<< " entries");

			//
			//	remove both entries
			//
			connection_map_.erase(pos);
			connection_map_.erase(receiver);

			BOOST_ASSERT((connection_map_.size() % 2) == 0);

			return true;
		}

		return false;
	}

	void server_stub::transmit_data(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[8113a678-903f-4975-8f11-49c1bf63a3c6,1B1B1B1B010101017681063138303430...000001B1B1B1B1A039946]
		//
		//	* sender
		//	* data
		//
		auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		auto pos = connection_map_.find(tag);
		if (pos != connection_map_.end())
		{
			auto receiver = pos->second;

			CYNG_LOG_TRACE(logger_, "transfer data local "
				<< tag
				<< " ==> "
				<< receiver);

			propagate("ipt.transfer.data", receiver, cyng::vector_t({ frame.at(1) }));
			propagate("stream.flush", receiver, cyng::vector_t({}));

			//
			//	update meta data
			//
			auto dp = cyng::object_cast<cyng::buffer_t>(frame.at(1));
			if (dp != nullptr)
			{
				ctx.attach(client_inc_throughput(tag
					, receiver
					, dp->size()));
			}
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "transmit data failed: "
				<< tag
				<< " is not member of connection map with "
				<< connection_map_.size()
				<< " entrie(s)");

			for (auto const& m : connection_map_) {

				CYNG_LOG_TRACE(logger_, m.first
					<< " <==> "
					<< m.second);
			}
		}
	}

	void server_stub::push_connection(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		auto pos = client_map_.find(tag);
		if (pos != client_map_.end())
		{
			CYNG_LOG_TRACE(logger_, "push.connection "
				<< tag
				<< " on stack");
			ctx.push(pos->second);
		}
		else
		{
			CYNG_LOG_FATAL(logger_, "push.connection "
				<< tag
				<< " not found");
			ctx.push(cyng::make_object());
		}
	}

	void server_stub::client_propagate(cyng::context& ctx)
	{
		propagate(ctx.get_name(), ctx.get_frame());
	}

	void server_stub::client_res_close_impl(cyng::context& ctx)
	{
		//	[9f6585e8-c7c6-4f93-9b99-e21986dec2bb,3ed440a2-958f-4863-9f86-ff8b1a0dc2f1,19]
		//
		//	* client tag
		//	* peer
		//	* sequence
		//
		const cyng::vector_t frame = ctx.get_frame();
		ctx.attach(cyng::generate_invoke("log.msg.info", "client.res.close", frame));

		const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

		try {
			if (!remove_client_impl(tag)) {
				ctx.attach(cyng::generate_invoke("log.msg.error", "client.res.close - failed", frame));
			}
		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger_, "client.res.close "
				<< tag
				<< " failed: "
				<< ex.what());
		}
	}

	bool server_stub::remove_client_impl(boost::uuids::uuid tag)
	{
		auto pos = client_map_.find(tag);
		if (pos != client_map_.end())
		{
			//
			//	remove from client list
			//
			client_map_.erase(pos);
			clear_connection_map_impl(tag);

			CYNG_LOG_TRACE(logger_, client_map_.size()
				<< " ipt sessions open with "
				<< (connection_map_.size() / 2)
				<< " connections");

			//
			//	shutdown mode
			//
			if (!acceptor_.is_open() && client_map_.empty()) {

				CYNG_LOG_DEBUG(logger_, "send <empty-client-map> signal");

				cyng::async::lock_guard<cyng::async::mutex> lk(mutex_);
				cv_sessions_closed_.notify_all();
			}

			return true;
		}
		return false;
	}

	void server_stub::close_client(cyng::context& ctx)
	{
		BOOST_ASSERT(bus_->vm_.tag() == ctx.tag());

		//	[a95f46e9-eccd-4b47-b02c-17d5172218af]
		const cyng::vector_t frame = ctx.get_frame();

		const auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		if (!close_client_impl(tag)) {
			ctx.attach(cyng::generate_invoke("log.msg.error", "server.close.client - failed", tag));
		}
	}

	bool server_stub::close_client_impl(boost::uuids::uuid tag)
	{
		auto pos = client_map_.find(tag);
		if (pos != client_map_.end())
		{
			BOOST_ASSERT(tag == pos->first);
			return close_connection(tag, pos->second);
		}
		return false;
	}

	void server_stub::client_req_close_impl(cyng::context& ctx)
	{
		//
		//	SMF master requests to close a specific session. Typical reason is a login with an account
		//	that is already online. In supersede mode the running session will be canceled in favor  
		//	the new session.
		//

		//	[1d166271-ca0b-4875-93a3-0cec92dbe34d,ef013e5f-50b0-4afe-ae7b-4a2f1c83f287,1,0]
		//
		//	* client tag
		//	* peer
		//	* cluster sequence
		//
		const cyng::vector_t frame = ctx.get_frame();
		const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
		const auto seq = cyng::value_cast<std::uint64_t>(frame.at(2), 0u);

		//
		//	Remove connection/client from connection list and call stop()
		//	method
		//
		if (close_client_impl(tag)) {
			//
			//	acknowledge that session was closed
			//
			ctx.attach(client_res_close(tag, seq, true));
		}
		else {
			ctx.attach(cyng::generate_invoke("log.msg.error", "client.req.close - failed", frame));
			ctx.attach(client_res_close(tag, seq, false));
		}
	}

	void server_stub::propagate(std::string fun, cyng::vector_t const& msg)
	{
		if (!msg.empty())
		{
			auto pos = msg.begin();
			BOOST_ASSERT(pos != msg.end());
			auto tag = cyng::value_cast(*pos, boost::uuids::nil_uuid());
			BOOST_ASSERT_MSG(!tag.is_nil(), "invalid propagation protocol");

			//
			//	remove duplicate information
			//
			propagate(fun, tag, cyng::vector_t(++pos, msg.end()));
		}
		else	{
			CYNG_LOG_FATAL(logger_, "empty function " << fun);
		}
	}

	void server_stub::propagate(std::string fun, boost::uuids::uuid tag, cyng::vector_t&& msg)
	{
		auto pos = client_map_.find(tag);
		if (pos != client_map_.end())
		{
			cyng::vector_t prg;
			prg
				<< cyng::code::ESBA
				<< cyng::unwinder(std::move(msg))
				<< cyng::invoke(fun)
				<< cyng::code::REBA
				;
			propagate((*pos).second, std::move(prg));
		}
		else
		{
			if (boost::algorithm::equals(fun, "client.req.close.connection.forward"))
			{
				CYNG_LOG_WARNING(logger_, "session "
					<< tag
					<< " not found - clean up connection map");

				//
				//	clean up connection_map_
				//
				clear_connection_map_impl(tag);
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "session "
					<< tag
					<< " not found");

#ifdef _DEBUG
				for (auto client : client_map_)
				{
					CYNG_LOG_TRACE(logger_, client.first);
				}
#endif
			}
		}
	}

	void server_stub::client_res_open_connection_forward(cyng::context& ctx)
	{
		cyng::vector_t frame = ctx.get_frame();

		//
		//	[bff22fb4-f410-4239-83ae-40027fb2609e,f07f47e1-7f4d-4398-8fb5-ae1d8942a50e,16,true,
		//	%(("device-name":LSMTest4),("local-connect":true),("local-peer":3d5994f1-60df-4410-b6ba-c2934f8bfd5e),("origin-tag":bff22fb4-f410-4239-83ae-40027fb2609e),("remote-peer":3d5994f1-60df-4410-b6ba-c2934f8bfd5e),("remote-tag":3f972487-e9a9-4e03-92d4-e2df8f6d30c5)),
		//	%(("seq":1),("tp-layer":ipt))]
		//
		//	* session tag
		//	* peer
		//	* cluster sequence
		//	* success
		//	* options
		//	* bag - original data
		//ctx.run(cyng::generate_invoke("log.msg.trace", "client.res.open.connection.forward", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] tag
			boost::uuids::uuid,		//	[1] peer
			std::uint64_t,			//	[2] cluster sequence
			bool,					//	[3] success
			cyng::param_map_t,		//	[4] options
			cyng::param_map_t		//	[5] bag
		>(frame);


		const bool success = std::get<3>(tpl);

		//
		//	dom reader
		//
		auto dom = cyng::make_reader(std::get<4>(tpl));
		auto local_connect = cyng::value_cast(dom.get("local-connect"), false);
		if (success && local_connect)
		{
			auto origin_tag = cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());
			auto remote_tag = cyng::value_cast(dom.get("remote-tag"), boost::uuids::nil_uuid());
			BOOST_ASSERT(origin_tag == std::get<0>(tpl));
			BOOST_ASSERT(origin_tag != remote_tag);

			//
			//	create entry in local connection list
			//
			CYNG_LOG_INFO(logger_, "server_stub "
				<< ctx.tag()
				<< " establish a local connection "
				<< connection_map_.size()
				<< ": "
				<< origin_tag
				<< " <==> " 
				<< remote_tag);


			//
			//	insert both directions
			//
			auto pos = connection_map_.find(origin_tag);
			if (pos != connection_map_.end()) {
				CYNG_LOG_WARNING(logger_, "server_stub "
					<< ctx.tag()
					<< " has to remove "
					<< origin_tag
					<< " from connection map");
				auto idx = connection_map_.erase(pos);
				connection_map_.emplace_hint(idx, origin_tag, remote_tag);
			}
			else {
				connection_map_.emplace(origin_tag, remote_tag);
			}

			pos = connection_map_.find(remote_tag);
			if (pos != connection_map_.end()) {
				CYNG_LOG_WARNING(logger_, "server_stub "
					<< ctx.tag()
					<< " has to remove "
					<< remote_tag
					<< " from connection map");
				auto idx = connection_map_.erase(pos);
				connection_map_.emplace_hint(idx, remote_tag, origin_tag);
			}
			else {
				connection_map_.emplace(remote_tag, origin_tag);
			}

			//
			//	serious condition
			//
			if ((connection_map_.size() % 2) != 0) {
				CYNG_LOG_ERROR(logger_, "server_stub "
					<< ctx.tag()
					<< " has uneven number of entries "
					<< connection_map_.size());
			}
			BOOST_ASSERT((connection_map_.size() % 2) == 0);

			//
			//	update connection state of remote session
			//
			BOOST_ASSERT(local_connect);
			cyng::vector_t prg;
			prg
				<< origin_tag
				<< local_connect	//	"true" is the only logical value
				;

			//
			//	manage local connections
			//
			propagate("session.update.connection.state", remote_tag, std::move(prg));
		}

		//
		//	forward to session: "client.res.open.connection.forward"
		//
		propagate(ctx.get_name(), std::move(frame));
	}
}



