/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "client.h"
#include "session.h"
#include "db.h"
#include <NODE_project_info.h>

#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>

#include <cyng/value_cast.hpp>
#include <cyng/object_cast.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/dom/reader.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/assert.hpp>
#include <boost/functional/hash.hpp>
#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node 
{
	client::client(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, std::atomic<std::uint64_t>& global_configuration
		, boost::uuids::uuid stag
		, boost::filesystem::path stat_dir)
	: mux_(mux)
		, logger_(logger)
		, db_(db)
		, global_configuration_(global_configuration)
		, stat_dir_(stat_dir)
		, stag_(stag)
		, node_class_("undefined")
		, rng_(std::numeric_limits<std::uint32_t>::min(), std::numeric_limits<std::uint32_t>::max())
		, uuid_gen_()
	{}

	void client::register_this(cyng::context& ctx)
	{
		ctx.queue(cyng::register_function("client.req.login", 8, std::bind(&client::req_login, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.close", 4, std::bind(&client::req_close, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.res.close", 4, std::bind(&client::res_close, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.open.push.channel", 10, std::bind(&client::req_open_push_channel, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.res.open.push.channel", 9, std::bind(&client::res_open_push_channel, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.close.push.channel", 5, std::bind(&client::req_close_push_channel, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.register.push.target", 5, std::bind(&client::req_register_push_target, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.deregister.push.target", 5, std::bind(&client::req_deregister_push_target, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.open.connection", 6, std::bind(&client::req_open_connection, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.res.open.connection", 4, std::bind(&client::res_open_connection, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.close.connection", 5, std::bind(&client::req_close_connection, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.res.close.connection", 6, std::bind(&client::res_close_connection, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.transfer.pushdata", 6, std::bind(&client::req_transfer_pushdata, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.req.transmit.data", 5, std::bind(&client::req_transmit_data, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.inc.throughput", 3, std::bind(&client::inc_throughput, this, std::placeholders::_1)));
		ctx.queue(cyng::register_function("client.update.attr", 6, std::bind(&client::update_attr, this, std::placeholders::_1)));
	}

	bool client::set_connection_auto_login(cyng::object obj)
	{
		return node::set_connection_auto_login(global_configuration_, cyng::value_cast(obj, false));
	}

	bool client::set_connection_auto_enabled(cyng::object obj)
	{
		return node::set_connection_auto_enabled(global_configuration_, cyng::value_cast(obj, true));
	}

	bool client::set_connection_superseed(cyng::object obj)
	{
		return node::set_connection_superseed(global_configuration_, cyng::value_cast(obj, true));
	}

	bool client::set_catch_meters(cyng::object obj)
	{
		return node::set_catch_meters(global_configuration_, cyng::value_cast(obj, true));
	}

	bool client::set_catch_lora(cyng::object obj)
	{
		return node::set_catch_lora(global_configuration_, cyng::value_cast(obj, true));
	}

	bool client::set_generate_time_series(cyng::object obj)
	{
		return node::set_generate_time_series(global_configuration_, cyng::value_cast(obj, false));
	}

	bool client::is_generate_time_series() const
	{
		return node::is_generate_time_series(global_configuration_.load());
	}

	void client::set_class(std::string const& node_class)
	{
		node_class_ = node_class;
	}

	std::string client::get_class()
	{
		return node_class_;
	}

	void client::req_login(cyng::context& ctx)
	{
		//	[1cea6ddf-5044-478b-9fba-67fa23997ba6,65d9eb67-2187-481b-8770-5ce41801eaa6,1,data-store,secret,plain,%(("security":scrambled),("tp-layer":ipt)),<!259:session>]
		//
		//	* remote client tag
		//	* remote peer tag
		//	* sequence number (should be continually incremented)
		//	* name
		//	* pwd/credentials
		//	* authorization scheme
		//	* bag (to send back)
		//	* session object
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "client.req.login " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] name
			std::string,			//	[4] pwd/credential
			std::string,			//	[5] authorization scheme
			cyng::param_map_t		//	[6] bag
		>(frame);

		req_login_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, std::get<6>(tpl)
			, frame.at(7));
	}

	void client::req_login_impl(cyng::context& ctx,
		boost::uuids::uuid tag,
		boost::uuids::uuid cluster_tag,	//	peer
		std::uint64_t seq,
		std::string account,
		std::string pwd,
		std::string scheme,
		cyng::param_map_t const& bag,
		cyng::object self)
	{
		//
		//	test credentials
		//
		bool found{ false };
		bool wrong_pwd{ false };
		db_.access([&](const cyng::store::table* tbl_device, cyng::store::table* tbl_session, cyng::store::table* tbl_cluster, cyng::store::table* tbl_tsdb)->void {

			//
			// check if session is already authorized
			//
			const bool auth = check_auth_state(tbl_session, tag);
			if (auth)
			{
				ctx.queue(cyng::generate_invoke("log.msg.warning", "[" + account + "] already authorized", tag));
				ctx.queue(client_res_login(tag
					, seq
					, false
					, account
					, "already authorized"
					, 0
					, bag));
			}
			else
			{

				//
				// check for running session(s) with the same name
				//
				const bool online = check_online_state(ctx, tbl_session, account);

				if (!online)
				{
					//
					//	test credentials
					//
					boost::uuids::uuid dev_tag{ boost::uuids::nil_uuid() };
					std::uint32_t query{ 6 };
					std::tie(found, wrong_pwd, dev_tag, query) = test_credential(ctx, tbl_device, account, pwd);

					if (found && !wrong_pwd)
					{
						//	bag
						//	%(("security":scrambled),("tp-layer":ipt))
						auto dom = cyng::make_reader(bag);

						if (tbl_session->insert(cyng::table::key_generator(tag)
							, cyng::table::data_generator(self	//	local
								, cyng::make_object()			//	remote
								, cluster_tag					//	peer
								, dev_tag						//	device
								, account						//	name
								, rng_()			//	source id
								, std::chrono::system_clock::now()
								, boost::uuids::nil_uuid()
								, cyng::value_cast<std::string>(dom.get("tp-layer"), "tcp/ip")
								, 0u, 0u, 0u)
							, 1
							, tag))
						{
							ctx.queue(client_res_login(tag
								, seq
								, true
								, account
								, "OK"
								, query
								, bag));

							ctx.queue(cyng::generate_invoke("log.msg.info", "[" + account + "] has session tag", tag));

							//
							//	write statistics
							//
							write_stat(tbl_tsdb, tag, account, "login", "OK");

							//
							//	update cluster table
							//
							//auto tag_cluster = cyng::object_cast<session>(self)->vm_.tag();
							auto key_cluster = cyng::table::key_generator(cluster_tag);
							auto list = get_clients_by_peer(tbl_session, cluster_tag);
							tbl_cluster->modify(key_cluster, cyng::param_factory("clients", static_cast<std::uint64_t>(list.size())), tag);

						}
						else
						{
							ctx.queue(client_res_login(tag
								, seq
								, false
								, account
								, "cannot create session"
								, query
								, bag));

							ctx.queue(cyng::generate_invoke("log.msg.error"
								, "cannot insert new session of account "
								, account
								, " with pk "
								, tag));

							//
							//	write statistics
							//
							write_stat(tbl_tsdb, tag, account, "login", "internal error");

						}
					}
				}
				else
				{
					found = true;

					ctx.queue(client_res_login(tag
						, seq
						, false
						, account
						, "already online"
						, 0
						, bag));

					//
					//	write statistics
					//
					write_stat(tbl_tsdb, tag, account, "login", "already online");

				}
			}
		}	, cyng::store::read_access("TDevice")
			, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Cluster")
			, cyng::store::write_access("_TimeSeries"));

		if (!found)
		{
			ctx.queue(client_res_login(tag
				, seq
				, false
				, account
				, "unknown device"
				, 0
				, bag));

			if (!wrong_pwd && is_connection_auto_login(global_configuration_.load()))
			{
				//
				//	create new device record
				//
				if (!db_.insert("TDevice"
					, cyng::table::key_generator(uuid_gen_())	//	new pk
					, cyng::table::data_generator(account
						, pwd
						, account	//	use account as number
						, "auto"	//	comment
						, ""	//	device id
						, NODE_VERSION	//	firmware version
						, is_connection_auto_enabled(global_configuration_.load())	//	enabled
						, std::chrono::system_clock::now()
						, 6u)
					, 1
					, tag))
				{
					ctx.queue(cyng::generate_invoke("log.msg.error"
						, account
						, "auto login failed"));

				}
				else
				{
					ctx.queue(cyng::generate_invoke("log.msg.warning"
						, account
						, "auto login"));
				}
			}
			else
			{
				ctx.queue(cyng::generate_invoke("log.msg.warning"
					, "bus.req.login failed"
					, account));

			}

			//
			//	system message
			//
			if (wrong_pwd)
			{
				insert_msg(db_, cyng::logging::severity::LEVEL_WARNING
					, "login of [" + account + "] failed (cause: incorrect password)"
					, tag);
			}
			else {
				insert_msg(db_, cyng::logging::severity::LEVEL_WARNING
					, "login of [" + account + "] failed (cause: unknown device)"
					, tag);
			}
		}
	}

	std::tuple<bool, bool, boost::uuids::uuid, std::uint32_t> 
		client::test_credential(cyng::context& ctx, const cyng::store::table* tbl, std::string const& account, std::string const& pwd)
	{
		bool found{ false };
		bool wrong_pwd{ false };
		boost::uuids::uuid dev_tag{ boost::uuids::nil_uuid() };
		std::uint32_t query{ 6 };

		tbl->loop([&](cyng::table::record const& rec) -> bool {

			const auto rec_account = cyng::value_cast<std::string>(rec["name"], "");

			if (boost::algorithm::equals(account, rec_account))
			{
				ctx.queue(cyng::generate_invoke("log.msg.trace", "account match [", account, "] OK"));

				//
				//	matching name
				//	test password
				//
				const auto rec_pwd = cyng::value_cast<std::string>(rec["pwd"], "");
				if (boost::algorithm::equals(pwd, rec_pwd))
				{
					ctx.queue(cyng::generate_invoke("log.msg.info", "password match [", account, "] OK"));

					//
					//	matching password
					//	insert new session
					//
					dev_tag = cyng::value_cast(rec["pk"], dev_tag);
					query = cyng::value_cast<std::uint32_t>(rec["query"], 6);

					//
					//	terminate loop
					//
					found = true;
				}
				else
				{
					//
					//	set wrong password marker
					//
					wrong_pwd = true;

					ctx.queue(cyng::generate_invoke("log.msg.warning"
						, "password does not match ["
						, account
						, pwd
						, ("(" + rec_pwd + ")")));
				}
			}

			//	continue loop
			return !found;
		});

#ifdef _DEBUG
		if (!found) {
			tbl->loop([&](cyng::table::record const& rec) -> bool {

				const auto rec_account = cyng::value_cast<std::string>(rec["name"], "");
				CYNG_LOG_DEBUG(logger_, "lookup: " << account << " - " << rec_account);
				return !boost::algorithm::equals(pwd, rec_account);
			});
		}
#endif

		return std::make_tuple(found, wrong_pwd, dev_tag, query);
	}

	bool client::check_auth_state(cyng::store::table* tbl, boost::uuids::uuid tag)
	{
		//
		//	generate table key
		//
		auto key = cyng::table::key_generator(tag);
		return tbl->exist(key);
	}

	bool client::check_online_state(cyng::context& ctx, cyng::store::table* tbl, std::string const& account)
	{
		bool online{ false };
		tbl->loop([&](cyng::table::record const& rec) -> bool {

			const auto rec_name = cyng::value_cast<std::string>(rec["name"], "");
			if (boost::algorithm::equals(account, rec_name))
			{
				//
				//	already online
				//
				online = true;
				const auto rec_tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());

				if (is_connection_superseed(global_configuration_.load()))
				{
					//
					//	ToDo: probably not the same peer!
					//
					ctx.queue(client_req_close(rec_tag, 0));
					ctx.queue(cyng::generate_invoke("log.msg.warning", account, rec_tag, "will be superseded"));
				}
				else
				{
					ctx.queue(cyng::generate_invoke("log.msg.warning", account, rec_tag, "already online"));

				}
			}

			//	false terminates the loop
			return !online;
		});

		return online;
	}

	void client::req_close(cyng::context& ctx)
	{
		//	[d673954a-741d-41c1-944c-d0036be5353f,47e74fa2-1410-4f65-b4fb-b8bce05d8061,16,10054]
		//
		//	* remote client tag
		//	* remote peer tag
		//	* sequence number (should be continuously incremented)
		//	* error code value
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_INFO(logger_, "client.req.close " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] cluster tag - peer
			std::uint64_t,			//	[2] sequence number
			int						//	[3] error code
		>(frame);

		close_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, ctx.tag()
			, true);	//	request
	}

	void client::res_close(cyng::context& ctx)
	{
		//	[d673954a-741d-41c1-944c-d0036be5353f,47e74fa2-1410-4f65-b4fb-b8bce05d8061,16,10054]
		//
		//	* remote client tag
		//	* remote peer tag
		//	* sequence number (should be continuously incremented)
		//	* [bool] success flag
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "client.res.close " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			bool					//	[3] success flag
		>(frame);

		if (!std::get<3>(tpl))
		{
			CYNG_LOG_WARNING(logger_, "client.res.close "
				<< cyng::io::to_str(frame)
				<< " failed");
		}

		close_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, ctx.tag()
			, false);	//	resonse
	}

	void client::close_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid cluster_tag
		, std::uint64_t seq
		, boost::uuids::uuid self
		, bool req)
	{
		//
		//	remove session record
		//
		bool success{ false };
		db_.access([&](cyng::store::table* tbl_session
			, cyng::store::table* tbl_target
			, cyng::store::table* tbl_cluster
			, cyng::store::table* tbl_connection
			, cyng::store::table* tbl_tsdb)->void {

			//
			//	generate table key for
			//	* session table: _Session
			//
			auto key = cyng::table::key_generator(tag);

			//
			//	get session object
			//
			cyng::table::record rec = tbl_session->lookup(key);

			if (!rec.empty())
			{

				//
				//	generate statistics
				//	
				const std::string account = cyng::value_cast<std::string>(rec["name"], "");

				//
				//	close open connections
				//	
				auto local_peer = cyng::object_cast<session>(rec["local"]);
				auto remote_peer = cyng::object_cast<session>(rec["remote"]);
				if (local_peer && remote_peer)
				{
					//
					//	There is an open connection
					//
					const auto rtag = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());
					const auto rkey = cyng::table::key_generator(rtag);

					BOOST_ASSERT(tag != rtag);
					BOOST_ASSERT(!rtag.is_nil());

					//
					//	simulate a bag
					//
					cyng::param_map_t bag = cyng::param_map_factory("origin-tag", tag)
						("local-peer", cluster_tag)
						("local-connect", cyng::make_object(local_peer->hash() == remote_peer->hash()));

					if (local_peer->hash() == remote_peer->hash())
					{
						//
						//	local
						//
						ctx.queue(cyng::generate_invoke("log.msg.warning"
							, "forward (local) connection close request from"
							, tag
							, " ==> "
							, rtag));
					
						ctx.queue(client_req_close_connection_forward(rtag, tag, seq, true, cyng::param_map_factory("local-connect", true), bag));

						//
						//	write stats
						//
						write_stat(tbl_tsdb, rtag, account, "close connection", "local");

					}
					else
					{
						//
						//	remote
						//
						ctx.queue(cyng::generate_invoke("log.msg.warning"
							, "forward (distinct) connection close request from"
							, tag
							, " ==> "
							, rtag));
						remote_peer->vm_.async_run(client_req_close_connection_forward(rtag, tag, seq, true, cyng::param_map_factory("local-connect", false), bag));

						write_stat(tbl_tsdb, rtag, account, "close connection", "remote");
					}

					//
					//	close open connection
					//
					connection_erase(tbl_connection, cyng::table::key_generator(tag, rtag), tag);

					//
					//	clean up remote session
					//
					tbl_session->modify(rkey, cyng::param_factory("rtag", boost::uuids::nil_uuid()), tag);
					tbl_session->modify(rkey, cyng::param_factory("remote", cyng::null()), tag);

				}

				//
				//	ToDo: close open channels
				//

				//
				//	remove registered targets
				//
				const auto count_targets = remove_targets_by_tag(tbl_target, tag);
				ctx.queue(cyng::generate_invoke("log.msg.info"
					, count_targets
					, "targets of session "
					, tag
					, "removed"));

				if (count_targets != 0u) {
					write_stat(tbl_tsdb, tag, account, "remove targets", count_targets);
				}

				//
				//	remove session
				//	update success flag
				//
				success = tbl_session->erase(key, tag);

				ctx.queue(cyng::generate_invoke("log.msg.info"
					, "client session"
					, tag
					, account
					, "removed"));

				const auto now = std::chrono::system_clock::now();
				auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - cyng::value_cast(rec["loginTime"], now));
				write_stat(tbl_tsdb, tag, account, "offline", uptime);

				//
				//	update cluster table
				//
				auto list = get_clients_by_peer(tbl_session, cluster_tag);
				auto key_cluster = cyng::table::key_generator(cluster_tag);
				tbl_cluster->modify(key_cluster, cyng::param_factory("clients", static_cast<std::uint64_t>(list.size())), tag);
			}
			else
			{
				ctx.queue(cyng::generate_invoke("log.msg.warning"
					, "client session"
					, tag
					, "not found"));
			}

		}	, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Target")
			, cyng::store::write_access("_Cluster")
			, cyng::store::write_access("_Connection")
			, cyng::store::write_access("_TimeSeries"));

		if (req)
		{
			//
			//	send a response
			//
			ctx.queue(client_res_close(tag
					, seq
					, success));
		}

	}

	void client::req_open_connection(cyng::context& ctx)
	{
		//	[client.req.open.connection,[29feef99-3f33-4c90-b0db-8c2c50d3d098,90732ba2-1c8a-4587-a19f-caf27752c65a,3,LSMTest1,%(("seq":1),("tp-layer":ipt))]]
		//
		//	* client tag
		//	* peer
		//	* bus sequence
		//	* number
		//	* bag
		//	* this session object
		//
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.open.connection", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] number
			cyng::param_map_t		//	[4] bag
		>(frame);

		req_open_connection_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, frame.at(5));
	}

	void client::req_open_connection_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, std::uint64_t seq
		, std::string number
		, cyng::param_map_t const& bag
		, cyng::object self)
	{
		auto caller_peer = cyng::object_cast<session>(self);
		BOOST_ASSERT(caller_peer != nullptr);

		cyng::param_map_t options;
		options["origin-tag"] = cyng::make_object(tag);		//	send response to this session
		options["local-peer"] = cyng::make_object(peer);	//	and this peer
															
		bool success{ false };
		db_.access([&](cyng::store::table const* tbl_device, cyng::store::table* tbl_session, cyng::store::table* tbl_tsdb)->void {

			//
			//	generate statistics
			//	get session object
			//
			cyng::table::record rec = tbl_session->lookup(cyng::table::key_generator(tag));
			const std::string account = cyng::value_cast<std::string>(rec["name"], "");

			//
			//	search device with the given number
			//
			CYNG_LOG_INFO(logger_, "find number \""
				<< number
				<< "\" out of "
				<< tbl_device->size()
				<< " devices");

			tbl_device->loop([&](cyng::table::record const& rec) -> bool {

				//
				//	search a matching number
				//
				const auto dev_number = cyng::value_cast<std::string>(rec["msisdn"], "");

				//CYNG_LOG_DEBUG(logger_, dev_number);
				if (boost::algorithm::equals(number, dev_number))
				{
					//
					//	search session of this device
					//
					const auto dev_tag = cyng::value_cast(rec["pk"], boost::uuids::nil_uuid());
					const auto callee = cyng::value_cast<std::string>(rec["name"], "");
					options["device-name"] = rec["name"];

					//
					//	do not call if device is not enabled
					//
					const auto enabled = cyng::value_cast(rec["enabled"], false);

					if (enabled)
					{
						//
						//	device found
						//
						CYNG_LOG_TRACE(logger_, "matching device "
							<< dev_tag
							<< ':'
							<< callee);
					}
					else
					{
						//
						//	device found but disabled
						//
						CYNG_LOG_WARNING(logger_, "matching device "
							<< dev_tag
							<< ':'
							<< callee
							<< " is not enabled");

						//
						//	write statistics
						//
						write_stat(tbl_tsdb, tag, account, "dialup " + dev_number, "disabled");

						//
						//	abort loop
						//
						return false;
					}

					tbl_session->loop([&](cyng::table::record const& rec) -> bool {

						const auto ses_tag = cyng::value_cast(rec["device"], boost::uuids::nil_uuid());
						if (dev_tag == ses_tag)
						{
							//
							//	session found
							//
							CYNG_LOG_TRACE(logger_, "matching session " 
								<< cyng::io::to_str(rec.key())
								<< cyng::io::to_str(rec.data()));

							options["remote-tag"] = rec["tag"];
							options["remote-peer"] = rec["peer"];

							//
							//	forward connection open request
							//
							auto remote_tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());
							BOOST_ASSERT_MSG(tag != remote_tag, "connection close request with same client tags");

							auto callee_peer = cyng::object_cast<session>(rec["local"]);
							BOOST_ASSERT(callee_peer != nullptr);

							const bool local_connect = caller_peer->hash() == callee_peer->hash();
							options["local-connect"] = cyng::make_object(local_connect);

							if (local_connect)
							{
								CYNG_LOG_TRACE(logger_, "forward (local) connection open request from"
									<< tag
									<< " ==> "
									<< remote_tag);

								//
								//	forward connection open request in same VM
								//
								ctx.queue(client_req_open_connection_forward(remote_tag
									, number
									, options
									, bag));

							}
							else
							{
								CYNG_LOG_TRACE(logger_, "forward (distinct) connection open request from"
									<< tag
									<< " ==> "
									<< remote_tag);

								//
								//	forward connection open request in different VM
								//
								callee_peer->vm_.async_run(client_req_open_connection_forward(remote_tag
									, number
									, options
									, bag));

							}

							//
							//	write statistics
							//
							if (is_generate_time_series()) {
								write_stat(tbl_tsdb, tag, account, "dialup", dev_number.c_str());
								write_stat(tbl_tsdb, tag, callee, "called by", account.c_str());
							}

							success = true;
							return false;
						}
						//	continue loop in *Session
						return true;
					});

					//	continue loop in TDevice
					return false;
				}
				return true;
			});

		}	, cyng::store::read_access("TDevice")
			, cyng::store::write_access("_Session")
			, cyng::store::write_access("_TimeSeries"));

		if (!success)
		{
			options["response-code"] = cyng::make_object<std::uint8_t>(ipt::tp_res_open_connection_policy::DIALUP_FAILED);
			CYNG_LOG_WARNING(logger_, "cannot open connection: device #"
				<< number
				<< " not found");

			ctx.queue(client_res_open_connection_forward(tag
				, seq
				, success
				, options
				, bag));

			//
			//	place a system message
			//
			insert_msg(db_, cyng::logging::severity::LEVEL_WARNING
				, "cannot open connection: device #" + number + " not found"
				, tag);
		}
	}

	void client::res_open_connection(cyng::context& ctx)
	{
		//	[client.res.open.connection,
		//	[232e356c-7188-4c2b-bc4b-ee3247061904,b81f32c5-eecd-43f2-9183-cacc255ad03b,0,false,
		//	%(("seq":1),("tp-layer":ipt))]]
		//
		//	* origin session tag
		//	* peer (origin)
		//	* cluster sequence
		//	* success
		//	* options
		//	* bag (origin)
		const cyng::vector_t frame = ctx.get_frame();
		//ctx.run(cyng::generate_invoke("log.msg.info", "client.res.open.connection", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			bool,					//	[3] success
			cyng::param_map_t,		//	[4] options
			cyng::param_map_t		//	[5] bag
		>(frame);

		res_open_connection_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl));
	}

	void client::res_open_connection_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		//	"remote-tag":5e8843cc-cba3-4a0e-80dd-5c734950017c
		//
		//	dom reader
		//
		auto dom = cyng::make_reader(options);
		auto rtag = cyng::value_cast(dom.get("remote-tag"), boost::uuids::nil_uuid());

		CYNG_LOG_INFO(logger_, "establish connection between "
			<< tag
			<< " and "
			<< rtag);

		//
		//	insert connection record
		//
		db_.access([&](cyng::store::table* tbl_session, cyng::store::table* tbl_connection, cyng::store::table* tbl_tsdb)->void {

			//
			//	generate statistics
			//	
			cyng::table::record rec = tbl_session->lookup(cyng::table::key_generator(tag));
			const std::string account = cyng::value_cast<std::string>(rec["name"], "");

			//
			//	generate table keys
			//
			auto caller_tag = cyng::table::key_generator(tag);
			auto callee_tag = cyng::table::key_generator(rtag);

			//
			//	get session objects
			//
			cyng::table::record caller_rec = tbl_session->lookup(caller_tag);
			cyng::table::record callee_rec = tbl_session->lookup(callee_tag);

			BOOST_ASSERT_MSG(!caller_rec.empty(), "no caller record");
			BOOST_ASSERT_MSG(!callee_rec.empty(), "no callee record");

			if (!caller_rec.empty() && !callee_rec.empty())
			{
				auto caller_peer = cyng::object_cast<session>(caller_rec["local"]);
				auto callee_peer = cyng::object_cast<session>(callee_rec["local"]);
				auto callee = cyng::value_cast<std::string>(callee_rec["name"], "");

				//
				//	detect local connection
				//
				const bool local = caller_peer->hash() == callee_peer->hash();

				if (success)
				{
					//
					//	update "remote" attributes
					//
					tbl_session->modify(caller_tag, cyng::param_t("remote", callee_rec["local"]), tag);
					tbl_session->modify(caller_tag, cyng::param_t("rtag", cyng::make_object(rtag)), tag);

					tbl_session->modify(callee_tag, cyng::param_t("remote", caller_rec["local"]), tag);
					tbl_session->modify(callee_tag, cyng::param_t("rtag", cyng::make_object(tag)), tag);

					//
					//	insert connection record
					//
					if (!tbl_connection->insert(cyng::table::key_generator(tag, rtag)
						, cyng::table::data_generator(caller_rec["name"]	//	caller
							, callee_rec["name"]	//	callee
							, local	// local/distinguished
							, caller_rec["layer"]	//	protocol layer of caller
							, callee_rec["layer"]	//	protocol layer of callee
							, 0u	//	data throughput
							, std::chrono::system_clock::now())	//	start time
						, 1, tag))
					{
						CYNG_LOG_ERROR(logger_, "insert connection record "
							<< tag
							<< " and "
							<< rtag
							<< " failed");

					}

					//
					//	write statistics
					//
					write_stat(tbl_tsdb, tag, account, "answer from " + callee, "OK");

				}
				else
				{
					//
					//	write statistics
					//
					write_stat(tbl_tsdb, tag, account, "answer from " + callee, "failed");
				}

				if (local)
				{
					CYNG_LOG_TRACE(logger_, "connection between "
						<< tag
						<< " and "
						<< rtag
						<< " is local");

					//
					//	forward to caller (origin)
					//	same VM
					//
					ctx.queue(client_res_open_connection_forward(tag
						, seq
						, success
						, options
						, bag));
				}
				else
				{
					CYNG_LOG_TRACE(logger_, "connection between "
						<< tag
						<< " and "
						<< rtag
						<< " is distinct");

					//
					//	different VM
					//
					caller_peer->vm_.async_run(client_res_open_connection_forward(tag
						, seq
						, success
						, options
						, bag));
				}
			}
			else
			{
				CYNG_LOG_FATAL(logger_, "client.res.open.connection between "
					<< tag
					<< " and "
					<< rtag
					<< " failed");
			}

		}	, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Connection")
			, cyng::store::write_access("_TimeSeries"));

	}

	void client::res_close_connection(cyng::context& ctx)
	{
		//	[906add5a-403d-4237-8275-478ba9efec4b,4386c0d0-4307-4160-bb76-6fb8ae4d5c4b,2,true,%(("local-connect":false),("local-peer":08b3ef0a-c492-4317-be22-d34b95651e56),("origin-tag":906add5a-403d-4237-8275-478ba9efec4b),("send-response":false)),%()]
		//	, tag
		//	, cyng::code::IDENT
		//	, seq
		//	, success
		//	, options
		//	, bag))

		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.res.close.connection", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			bool,					//	[3] success
			cyng::param_map_t,		//	[4] options
			cyng::param_map_t		//	[5] bag
		>(frame);

		res_close_connection_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl));

	}

	void client::res_close_connection_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		//
		//	[462cb5e1-ce52-4966-a445-c983ed1ddcc4,bdc31cf8-e18e-4d95-ad31-ad821661e857,4,true,
		//	%(("local-connect":true),("local-peer":bdc31cf8-e18e-4d95-ad31-ad821661e857)),
		//	%(("tp-layer":modem),("origin-tag":5afa7628-caa3-484d-b1de-a4730b53a656))]
		//
		db_.access([&](cyng::store::table* tbl_session, cyng::store::table* tbl_connection)->void {

			//
			//	send response back to "origin-tag"
			//	create session table key
			//
			auto dom = cyng::make_reader(bag);
			auto tag = cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());
			auto key = cyng::table::key_generator(tag);
			BOOST_ASSERT(!tag.is_nil());

			//
			//	get session objects
			//
			cyng::table::record rec = tbl_session->lookup(key);

			if (!rec.empty())
			{
				auto local_peer = cyng::object_cast<session>(rec["local"]);
				auto remote_peer = cyng::object_cast<session>(rec["remote"]);

				const auto rtag = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());
				const auto rkey = cyng::table::key_generator(rtag);

				BOOST_ASSERT(tag != rtag);
				BOOST_ASSERT(!rtag.is_nil());

				//
				//	get name of origin
				//
				auto local_name = cyng::value_cast<std::string>(rec["name"], "");

				if (remote_peer && local_peer)
				{

					if (local_peer->hash() == remote_peer->hash())
					{

						CYNG_LOG_TRACE(logger_, "forward connection close response (local) to "
							<< local_name
							<< ':'
							<< tag);

						//
						//	forward to caller (origin)
						//	same VM
						//
						ctx.queue(client_res_close_connection_forward(tag
							, seq
							, success
							, options
							, bag));

					}
					else
					{
						CYNG_LOG_TRACE(logger_, "forward connection close response (distinct) to "
							<< local_name
							<< ':'
							<< tag);

						//
						//	different VM
						//
						local_peer->vm_.async_run(client_res_close_connection_forward(tag
							, seq
							, success
							, options
							, bag));
					}

					//
					//	remove connection record
					//
					if (!connection_erase(tbl_connection, cyng::table::key_generator(tag, rtag), tag))
					{
						CYNG_LOG_ERROR(logger_, tag
							<< "remove connection record "
							<< local_name
							<< ":"
							<< tag
							<< " <=> "
							<< rtag
							<< " failed");
					}

					//
					//	cleanup "remote" attributes
					//
					tbl_session->modify(key, cyng::param_factory("rtag", boost::uuids::nil_uuid()), tag);
					tbl_session->modify(key, cyng::param_factory("remote", cyng::null()), tag);

					tbl_session->modify(rkey, cyng::param_factory("rtag", boost::uuids::nil_uuid()), tag);
					tbl_session->modify(rkey, cyng::param_factory("remote", cyng::null()), tag);

				}
				else
				{
					CYNG_LOG_ERROR(logger_, tag
						<< "session ("
						<< local_name
						<< ") has no remote peer - cannot forward connection close response");
				}
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "session ("
					<< tag
					<< ") not found - cannot forward connection close response");
			}

		}	, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Connection"));
	}

	void client::req_transmit_data(cyng::context& ctx)
	{
		//	 [client.req.transmit.data,
		//	[540a2bda-5891-44a6-909b-89bfdc494a02,767a51e3-ded3-4eb3-bc61-8bfd931afc75,7,
		//	%(("tp-layer":ipt))
		//	&&1B1B1B1B010101017681063138...8C000000001B1B1B1B1A03E6DD]]
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.trace", "client.req.transmit.data", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			cyng::param_map_t		//	[3] bag
		>(frame);

		req_transmit_data_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, frame.at(4));
	}

	void client::req_transmit_data_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, std::uint64_t seq
		, cyng::param_map_t const& bag
		, cyng::object data)
	{
		cyng::vector_t prg;

		//
		//	transmit data
		//
		db_.access([&](cyng::store::table* tbl_session, cyng::store::table* tbl_connection)->void {

			//
			//	generate session table key
			//
			auto caller_tag = cyng::table::key_generator(tag);

			//
			//	data size
			//
			const auto buffer_ptr = cyng::object_cast<cyng::buffer_t>(data);
			BOOST_ASSERT_MSG(buffer_ptr != nullptr, "no data");
			const std::size_t size = (buffer_ptr != nullptr) 
				? cyng::object_cast<cyng::buffer_t>(data)->size()
				: 0u
				;

			//
			//	get session objects
			//
			cyng::table::record rec = tbl_session->lookup(cyng::table::key_generator(tag));
			if (!rec.empty()) {

				//
				//	update rx
				//	data from device
				//
				const std::uint64_t rx = cyng::value_cast<std::uint64_t>(rec["rx"], 0);
				tbl_session->modify(rec.key(), cyng::param_factory("rx", static_cast<std::uint64_t>(rx + size)), tag);

				//
				//	transfer data
				//
				boost::uuids::uuid link = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());

				auto local_peer = cyng::object_cast<session>(rec["local"]);
				auto remote_peer = cyng::object_cast<session>(rec["remote"]);

				if (remote_peer && local_peer)
				{
					if (local_peer->hash() == remote_peer->hash())
					{
						CYNG_LOG_TRACE(logger_, "transmit local "
							<< size
							<< " bytes from "
							<< tag
							<< " => "
							<< link);
						//
						//	local connection
						//
						ctx.queue(client_req_transfer_data_forward(link
							, seq
							, bag
							, data));
					}
					else
					{
						CYNG_LOG_TRACE(logger_, "transmit distinct "
							<< size
							<< " bytes from "
							<< tag
							<< " => "
							<< link);

						//
						//	distinct connection
						//
						remote_peer->vm_.async_run(client_req_transfer_data_forward(link
							, seq
							, bag
							, data));
					}

					//
					//	update sx
					//	data to device
					//
					cyng::table::record rec_link = tbl_session->lookup(cyng::table::key_generator(link));
					std::uint64_t sx = cyng::value_cast<std::uint64_t>(rec_link["sx"], 0u);
					tbl_session->modify(rec_link.key(), cyng::param_factory("sx", static_cast<std::uint64_t>(sx + size)), tag);

					//
					//	get record from connection table
					//
					cyng::table::record rec_conn = connection_lookup(tbl_connection, cyng::table::key_generator(tag, link));
					if (!rec_conn.empty())
					{
						//
						//	update connection throughput
						//
						std::uint64_t throughput = cyng::value_cast<std::uint64_t>(rec_conn["throughput"], 0u);
						tbl_connection->modify(rec_conn.key(), cyng::param_factory("throughput", static_cast<std::uint64_t>(throughput + size)), tag);
					}
					else
					{
						CYNG_LOG_ERROR(logger_, "no connection record for "
							<< cyng::value_cast<std::string>(rec["name"], "")
							<< " <=> "
							<< cyng::value_cast<std::string>(rec_link["name"], ""));
					}
				}
				else
				{
					CYNG_LOG_ERROR(logger_, tag
						<< " ("
						<< cyng::value_cast<std::string>(rec["name"], "")
						<< ") has no remote peer - "
						<< size
						<< " bytes get lost");

#ifdef _DEBUG
					const auto p = cyng::object_cast<cyng::buffer_t>(data);
					if (p != nullptr)
					{
						std::stringstream ss;
						cyng::io::hex_dump hd;
						hd(ss, p->begin(), p->end());
						CYNG_LOG_TRACE(logger_, ss.str());
					}
#endif
				}
			}
			else {
				CYNG_LOG_ERROR(logger_, "client.req.transmit.data failed - session "
					<< tag
					<< " not found - "
					<< size
					<< " bytes get lost");

			}
		}	, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Connection"));
	}

	void client::req_close_connection(cyng::context& ctx)
	{
		//
		//	[5afa7628-caa3-484d-b1de-a4730b53a656,bdc31cf8-e18e-4d95-ad31-ad821661e857,false,8,
		//	%(("tp-layer":modem))]
		//
		//	* remote session tag
		//	* remote peer
		//	* shutdown
		//	* cluster sequence
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			bool,					//	[2] shutdown
			std::uint64_t,			//	[3] sequence number
			cyng::param_map_t		//	[4] bag
		>(frame);

		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.close.connection", frame));

		req_close_connection_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)	//	shutdown mode
			, std::get<3>(tpl)
			, std::get<4>(tpl));

	}

	void client::req_close_connection_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, bool shutdown
		, std::uint64_t	seq
		, cyng::param_map_t const&	bag)
	{
		//
		//	[5afa7628-caa3-484d-b1de-a4730b53a656,bdc31cf8-e18e-4d95-ad31-ad821661e857,false,8,%(("tp-layer":modem))]
		//

		db_.access([&](cyng::store::table* tbl_session)->void {

			cyng::param_map_t options;
			options["local-peer"] = cyng::make_object(peer);	//	and this peer

			//
			//	get record of session that triggered the connection close request
			//
			auto origin_tag = cyng::table::key_generator(tag);
			cyng::table::record rec = tbl_session->lookup(origin_tag);
			if (!rec.empty())
			{
				auto local_peer = cyng::object_cast<session>(rec["local"]);
				auto remote_peer = cyng::object_cast<session>(rec["remote"]);
				auto remote_tag = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());
				auto name = cyng::value_cast<std::string > (rec["name"], "");

				BOOST_ASSERT_MSG(tag != remote_tag, "connection close request with same client tags");
				if (local_peer && remote_peer)
				{
					options["local-connect"] = cyng::make_object(local_peer->hash() == remote_peer->hash());

					//	"client.req.close.connection.forward"
					if (local_peer->hash() == remote_peer->hash())
					{
						CYNG_LOG_TRACE(logger_, "forward (local) connection close request from"
							<< tag
							<< " ==> "
							<< remote_tag);

						ctx.queue(client_req_close_connection_forward(remote_tag
							, tag
							, seq
							, shutdown	//	no shutdown
							, options
							, bag));
					}
					else
					{
						//
						//	forward connection close request in different VM
						//
						CYNG_LOG_TRACE(logger_, "forward (distinnct) connection close request from"
							<< tag
							<< " ==> "
							<< remote_tag);

						remote_peer->vm_.async_run(client_req_close_connection_forward(remote_tag
							, tag
							, seq
							, shutdown	//	no shutdown
							, options
							, bag));
					}
				}
				else
				{

					//
					//	There is no open connection to close.
					//	Send response with error code.
					//

					options["local-connect"] = cyng::make_object(false);
					options["msg"] = cyng::make_object("[" + name + "] has no open connection to close");

					//	"client.res.close.connection.forward"
					ctx.queue(client_res_close_connection_forward(tag
						, seq
						, false //	no success
						, options
						, bag));

					insert_msg(db_, cyng::logging::severity::LEVEL_WARNING
						, "[" + name + "] has no open connection to close"
						, tag);
				}
			}
			else
			{
				//	error
				CYNG_LOG_ERROR(logger_, "no session record: " << tag);
			}

		}, cyng::store::write_access("_Session"));
	}

	void client::req_open_push_channel(cyng::context& ctx)
	{
		//	[00517ac4-e4cb-4746-83c9-51396043678a,0c2058e2-0c7d-4974-98bd-bf1b14c7ba39,12,LSM_Events,,,,,003c,%(("seq":52),("tp-layer":ipt))]
		//
		//	* client tag
		//	* peer
		//	* bus sequence
		//	* target name
		//	* device name
		//	* device number
		//	* device software version
		//	* device id
		//	* timeout
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "client.req.open.push.channel " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] remote peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] target name
			std::string,			//	[4] device name
			std::string,			//	[5] device number
			std::string,			//	[6] device software version
			std::string,			//	[7] device id
			std::chrono::seconds,	//	[8] timeout
			cyng::param_map_t		//	[9] bag
		>(frame);

		req_open_push_channel_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, std::get<6>(tpl)
			, std::get<7>(tpl)
			, std::get<8>(tpl)
			, std::get<9>(tpl));
			//, ctx.tag());
	}

	void client::req_open_push_channel_impl(cyng::context& ctx
		, boost::uuids::uuid tag, //	remote tag
		boost::uuids::uuid peer,	//	[1] remote peer
		std::uint64_t seq,			//	[2] sequence number
		std::string name,			//	[3] target name
		std::string account,		//	[4] device name
		std::string number,			//	[5] device number
		std::string software,		//	[6] device software version
		std::string version,			//	[7] device id
		std::chrono::seconds timeout,	//	[8] timeout
		cyng::param_map_t const& bag)
	{
		if (name.empty())
		{
			insert_msg(db_, cyng::logging::severity::LEVEL_WARNING
				, "no target specified"
				, tag);
			req_open_push_channel_empty(ctx, tag, seq, bag);
			return;
		}

		//
		//	find matching target
		//
		db_.access([&](const cyng::store::table* tbl_target
			, cyng::store::table* tbl_channel
			, const cyng::store::table* tbl_session
			, cyng::store::table* tbl_msg
			, const cyng::store::table* tbl_device
			, cyng::store::table* tbl_tsdb)->void {


			//
			//	get source channel
			//
			std::uint32_t source_channel;
			std::string account;
			boost::uuids::uuid source_tag;	//	key for device table
			std::tie(source_channel, account, source_tag) = get_source_channel(tbl_session, tag);

			//
			//	check if device is enabled
			//
			auto device_rec = tbl_device->lookup(cyng::table::key_generator(source_tag));
			BOOST_ASSERT_MSG(!device_rec.empty(), "node device record");
			const bool enabled = !device_rec.empty() && cyng::value_cast(device_rec["enabled"], false);
			if (!enabled)
			{
				ctx.queue(cyng::generate_invoke("log.msg.warning"
					, "open push channel - device"
					, account
					, "is not enabled"));
				req_open_push_channel_empty(ctx, tag, seq, bag);

				insert_msg(tbl_msg
					, cyng::logging::severity::LEVEL_INFO
					, "open push channel - device [" + account + "] is not enabled"
					, tag
					, 1000u);

				return;
			}

			//
			//	get a list of matching targets (matching by name)
			//
			auto r = collect_matching_targets(tbl_target
				, tag
				, name);

			ctx.queue(cyng::generate_invoke("log.msg.trace"
				, r.first.size()
				, "matching target(s) for"
				, name));

			//
			//	ToDo: check device name, number, version and id
			//

			if (r.first.empty())
			{
				insert_msg(tbl_msg
					, cyng::logging::severity::LEVEL_WARNING
					, "no target [" + name + "] registered"
					, tag
					, 1000u);

				//
				//	write statistics
				//
				write_stat(tbl_tsdb, tag, account, "no target", name.c_str());

			}

			//
			//	new (random) channel id
			//
			const std::uint32_t channel = rng_();

			//
			//	create push channels
			//
			for (auto key : r.first)
			{
				auto target_rec = tbl_target->lookup(key);
				BOOST_ASSERT(!target_rec.empty());

				if (create_channel(ctx
					, tbl_channel
					, tbl_session
					, tbl_msg
					, name
					, account
					, source_channel
					, channel
					, target_rec
					, tag
					, ctx.tag()	//	self
					, timeout
					, r.second
					, r.first.size()))
				{
					//
					//	write statistics
					//
					std::stringstream ss;
					ss
						<< name 
						<< " ("
						<< channel
						<< ")"
						;
					write_stat(tbl_tsdb, tag, account, "open push channel", ss.str());
				}

			}

			cyng::param_map_t options;
			options["response-code"] = cyng::make_object<std::uint8_t>(r.first.empty() ? ipt::tp_res_close_push_channel_policy::UNDEFINED : ipt::tp_res_close_push_channel_policy::SUCCESS);
			options["channel-status"] = cyng::make_object<std::uint8_t>(0); //	always 0
			options["packet-size"] = cyng::make_object(r.second);
			options["window-size"] = cyng::make_object<std::uint8_t>(1); //	always 1

			ctx.queue(client_res_open_push_channel(tag
				, seq
				, !r.first.empty()	//	success
				, channel			//	channel
				, source_channel	//	source
				, static_cast<std::uint32_t>(r.first.size())	//	count
				, options
				, bag));

		}	, cyng::store::read_access("_Target")
			, cyng::store::write_access("_Channel")
			, cyng::store::read_access("_Session")
			, cyng::store::write_access("_SysMsg")
			, cyng::store::read_access("TDevice")
			, cyng::store::write_access("_TimeSeries"));
	}

	bool client::create_channel(cyng::context& ctx
		, cyng::store::table* tbl_channel
		, const cyng::store::table* tbl_session
		, cyng::store::table* tbl_msg
		, std::string const& target_name
		, std::string const& account
		, std::uint32_t source_channel
		, std::uint32_t channel
		, cyng::table::record const& target_rec
		, boost::uuids::uuid tag
		, boost::uuids::uuid self
		, std::chrono::seconds timeout
		, std::uint16_t max_packet_size
		, std::size_t count)
	{
		//
		//	get target channel
		//
		const std::uint32_t target = cyng::value_cast<std::uint32_t>(target_rec["channel"], 0);

		//
		//	get target session
		//
		const boost::uuids::uuid target_session_tag = cyng::value_cast(target_rec["tag"], boost::uuids::nil_uuid());
		auto target_session_rec = tbl_session->lookup(cyng::table::key_generator(target_session_tag));
		if (!target_session_rec.empty())
		{

			//
			//	insert new push channel
			//
			if (tbl_channel->insert(cyng::table::key_generator(channel, source_channel, target)
				, cyng::table::data_generator(target_rec["tag"]	//	target session tag
					, target_session_rec["local"]	//	target peer object - TargetPeer
					, self	// ChannelPeer
					, max_packet_size	//	max packet size
					, timeout	//	timeout
					, count
					, account	//	owner
					, target_name)
				, 1, tag))
			{
				ctx.queue(cyng::generate_invoke("log.msg.info"
					, "open push channel"
					, channel
					, source_channel
					, target));

				return true;
			}
			else
			{
				ctx.queue(cyng::generate_invoke("log.msg.error"
					, "open push channel - failed"
					, channel, source_channel, target));

				insert_msg(tbl_msg
					, cyng::logging::severity::LEVEL_WARNING
					, "open push channel - [" + target_name + "] failed"
					, tag
					, 1000u);

			}
		}
		else
		{
			ctx.queue(cyng::generate_invoke("log.msg.warning"
				, "open push channel - no target session"
				, target_session_tag));

			insert_msg(tbl_msg
				, cyng::logging::severity::LEVEL_WARNING
				, "open push channel - no target [" + target_name + "] session"
				, tag
				, 1000u);

		}
		return false;
	}

	void client::res_open_push_channel(cyng::context& ctx)
	{
		//	 [5e065593-d587-4b1b-a5c0-e79bd0fb2eb0,ee82b721-1411-46f5-8df7-7c797842e4f3,0,false,00000000,00000000,00000000,%(),%(("seq":1),("tp-layer":ipt))]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_WARNING(logger_, "client.res.open.push.channel " << cyng::io::to_str(frame));

		//
		//	There is a bug in the EMH VSM to respond to connection close request with an "open channel response".
		//	So this message can be partly ignored.
		//	Additionally the VSM don't send a connection open request the first time after a connection was cleared from
		//	the remote side.
		//

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] tag
			boost::uuids::uuid,		//	[1] peer
			std::uint64_t,			//	[2] cluster sequence
			bool,					//	[3] success flag
			std::uint32_t,			//	[4] channel
			std::uint32_t,			//	[5] source
			std::uint32_t,			//	[6] count
			cyng::param_map_t,		//	[7] options
			cyng::param_map_t		//	[8] bag
		>(frame);

		boost::ignore_unused(tpl);
	}

	void client::req_close_push_channel(cyng::context& ctx)
	{
		//	[b7fcf460-84e4-493e-910e-2f9736774351,fbda6a25-d406-4d22-aca0-0ba3a8cd589a,682,2fd6e208,%(("seq":d7),("tp-layer":ipt))]
		//
		//	* remote session tag
		//	* remote peer
		//	* cluster sequence
		//	* channel
		//	* bag
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "client.req.close.push.channel " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::uint32_t,			//	[3] channel
			cyng::param_map_t		//	[4] bag
		>(frame);

		req_close_push_channel_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl));
	}


	void client::req_close_push_channel_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, std::uint64_t seq			//	[2] sequence number
		, std::uint32_t channel		//	[3] channel id
		, cyng::param_map_t const& bag)
	{
		cyng::table::key_list_t pks;

		db_.access([&](cyng::store::table* tbl_channel, cyng::store::table* tbl_tsdb)->void {
			tbl_channel->loop([&](cyng::table::record const& rec) -> bool {
				if (channel == cyng::value_cast<std::uint32_t>(rec["channel"], 0u))
				{ 
					//
					//	store table key
					//
					pks.push_back(rec.key());

					//
					//	write a debug message
					//
					ctx.queue(cyng::generate_invoke("log.msg.debug"
						, "req.close.push.channel"
						, rec.key()));

					//
					//	generate a time series event
					//
					auto const account = cyng::value_cast<std::string>(rec["owner"], "unknown");
					auto const target = cyng::value_cast<std::string>(rec["name"], "unknown");
					std::stringstream ss;
					ss
						<< target
						<< " ("
						<< channel
						<< ")"
						;
					write_stat(tbl_tsdb, tag, account, "close push channel", ss.str());
				}
				//	continue
				return true;
			});

			//
			//	remove channels from table
			//
			cyng::erase(tbl_channel, pks, tag);

		}	, cyng::store::write_access("_Channel")
			, cyng::store::write_access("_TimeSeries"));

		//
		//	send response
		//
		ctx.queue(client_res_close_push_channel(tag
			, seq
			, !pks.empty()	//	success
			, channel		//	channel
			, bag));
	}

	void client::req_transfer_pushdata(cyng::context& ctx)
	{
		//	[89fd8b7b-ceae-4803-bdd9-e88dfa07cbf0,1bbcd541-5d5f-46fa-b6a8-2e8c5fa25d26,17,4ee4092b,f807b7df,%(("block":0),("seq":a3),("status":c1),("tp-layer":ipt)),1B1B1B1B010101017608343131323237...53B01EC46010163FA7D00]
		//
		//	* session tag
		//	* peer
		//	* cluster seq
		//	* channel
		//	* source
		//	* bag
		//	* data
		const cyng::vector_t frame = ctx.get_frame();
		//ctx.run(cyng::generate_invoke("log.msg.info", "client.req.transfer.pushdata", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::uint32_t,			//	[3] channel
			std::uint32_t,			//	[4] source
			cyng::param_map_t		//	[5] bag
		>(frame);

		req_transfer_pushdata_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, std::get<5>(tpl)
			, frame.at(6));

	}

	void client::req_transfer_pushdata_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, std::uint64_t seq
		, std::uint32_t channel
		, std::uint32_t source
		, cyng::param_map_t const& bag
		, cyng::object data)
	{

		//
		//	list of all target session
		//
		cyng::table::key_list_t targets;

		//
		//	push data to target(s)
		//
		std::size_t counter{ 0 };
		db_.access([&](cyng::store::table* tbl_channel)->void {

			tbl_channel->loop([&](cyng::table::record const& rec) -> bool {

				// [transfer.push.data,474ba8c4,e9d30005,[4ee4092b,f807b7df,e7e1faee]]
				// [transfer.push.data,474ba8c4,e9d30005,[18f86863,a1e24bba,e7e1faee]]
				// [transfer.push.data,474ba8c4,e9d30005,[474ba8c4,e9d30005,d5c31f79]]
				// [transfer.push.data,474ba8c4,e9d30005,[8c16a625,2082352c,22ae9ef6]]
				//ctx.queue(cyng::generate_invoke("log.msg.debug"
				//	, "transfer.push.data"
				//	, channel, source, rec.key()));

				if (channel == cyng::value_cast<std::uint32_t>(rec["channel"], 0))
				{
					const auto target_tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());
					const auto target = cyng::value_cast<std::uint32_t>(rec["target"], 0);
					const auto owner_peer = cyng::value_cast(rec["peerChannel"], boost::uuids::nil_uuid());

					//
					//	transfer data
					//

					auto target_session = cyng::object_cast<session>(rec["peerTarget"]);
					BOOST_ASSERT(target_session != nullptr);
					if (owner_peer == target_session->vm_.tag())
					{
						CYNG_LOG_INFO(logger_, "transfer.push.data local: "
							<< tag
							<< " =="
							<< channel
							<< ':'
							<< source
							<< ':'
							<< target
							<< '#'
							<< counter
							<< "==> "
							<< target_tag);

						ctx.queue(client_req_transfer_pushdata_forward(target_tag
							, channel
							, source
							, target
							, data
							, bag));
					}
					else
					{
						CYNG_LOG_INFO(logger_, "transfer.push.data distinct: "
							<< tag
							<< " ==="
							<< channel
							<< ':'
							<< source
							<< ':'
							<< target
							<< '#'
							<< counter
							<< "===> "
							<< target_tag);

						target_session->vm_.async_run(client_req_transfer_pushdata_forward(target_tag
							, channel
							, source
							, target
							, data
							, bag));
					}

					//
					//	list of all target session
					//
					targets.push_back(cyng::table::key_generator(target));
					counter++;
				}

				//	continue
				return true;
			});

		}, cyng::store::write_access("_Channel"));

		if (counter == 0)
		{
			CYNG_LOG_WARNING(logger_, "transfer.push.data "
				<< tag
				<< " =="
				<< channel
				<< ':'
				<< source
				<< "==> no target ");

			insert_msg(db_
				, cyng::logging::severity::LEVEL_WARNING
				, "transfer.push.data without target"
				, tag);
		}
		else
		{
			//
			//	get data size
			//
			const std::size_t size = cyng::object_cast<cyng::buffer_t>(data)->size();

			//
			//	update px value of session
			//
			db_.access([&](cyng::store::table* tbl_session)->void {

				//
				//	generate table key
				//
				cyng::table::record rec = tbl_session->lookup(cyng::table::key_generator(tag));
				if (rec.empty())
				{
					CYNG_LOG_WARNING(logger_, "transfer.push.data - session "
						<< tag
						<< " not found");

				}
				else
				{
					std::uint64_t px = cyng::value_cast<std::uint64_t>(rec["px"], 0);
					tbl_session->modify(rec.key(), cyng::param_factory("px", static_cast<std::uint64_t>(px + size)), tag);
				}
			}, cyng::store::write_access("_Session"));

			//
			//	update px value and msg counter of targets
			//
			db_.access([&](cyng::store::table* tbl_target)->void {

				for (const auto& key : targets)
				{
					auto rec = tbl_target->lookup(key);
					if (!rec.empty())
					{
						std::uint64_t px = cyng::value_cast<std::uint64_t>(rec["px"], 0);
						tbl_target->modify(rec.key(), cyng::param_factory("px", static_cast<std::uint64_t>(px + size)), tag);

						std::uint64_t counter = cyng::value_cast<std::uint64_t>(rec["counter"], 0);
						tbl_target->modify(rec.key(), cyng::param_factory("counter", static_cast<std::uint64_t>(counter + 1)), tag);
					}
				}
			}, cyng::store::write_access("_Target"));
		}
		ctx.queue(client_res_transfer_pushdata(tag
			, seq
			, channel		//	channel
			, source		//	source
			, counter		//	count
			, bag));

	}

	void client::req_open_push_channel_empty(cyng::context& ctx
		, boost::uuids::uuid tag
		, std::uint64_t seq
		, cyng::param_map_t const& bag)
	{

		cyng::param_map_t options;
		options["response-code"] = cyng::make_object<std::uint8_t>(ipt::tp_res_close_push_channel_policy::UNREACHABLE);
		options["channel-status"] = cyng::make_object<std::uint8_t>(0);
		options["packet-size"] = cyng::make_object<std::uint16_t>(0);
		options["window-size"] = cyng::make_object<std::uint8_t>(0);

		ctx.queue(cyng::generate_invoke("log.msg.warning"
			, "client.req.open.push.channel"
			, "target name is empty"));


		ctx.queue(client_res_open_push_channel(tag
			, seq
			, false
			, 0		//	channel
			, 0		//	source
			, 0		//	count
			, options
			, bag));
	}

	void client::req_register_push_target(cyng::context& ctx)
	{
		//	[4e5dc7e8-37e7-4267-a3b6-d68a9425816f,76fbb814-3e74-419b-8d43-2b7b59dab7f1,67,data.sink.2,%(("pSize":65535),("seq":2),("tp-layer":ipt),("wSize":1))]
		//
		//	* remote client tag
		//	* remote peer
		//	* bus sequence
		//	* target
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.register.push.target", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] target name
			cyng::param_map_t		//	[4] bag
		>(frame);

		req_register_push_target_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, ctx.tag());
	}

	void client::req_register_push_target_impl(cyng::context& ctx
		, boost::uuids::uuid tag //	remote tag
		, boost::uuids::uuid peer	//	remote peer
		, std::uint64_t seq
		, std::string target
		, cyng::param_map_t const& bag
		, boost::uuids::uuid self)
	{
		cyng::param_map_t options;

		//
		//	generate a random new target/channel id
		//
		const auto channel = rng_();
		bool success{ false };
		db_.access([&](const cyng::store::table* tbl_session, cyng::store::table* tbl_target)->void {

			auto session_rec = tbl_session->lookup(cyng::table::key_generator(tag));
			if (!session_rec.empty())
			{
				//
				//	test for duplicate target names of same session
				//
				bool already_registered{ false };
				tbl_target->loop([&](cyng::table::record const& rec)->bool {

					auto target_name = cyng::value_cast<std::string>(rec["name"], "");
					if (boost::algorithm::equals(target_name, target))
					{
						//
						//	test if this session already registered a target with the same name
						//	[uuid] owner session - primary key 
						//
						const boost::uuids::uuid rec_tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());
						if (tag == rec_tag)
						{
							//
							//	duplicate target name
							//
							CYNG_LOG_WARNING(logger_, "client.req.register.push.target - session "
								<< tag
								<< " already registered "
								<< target_name);

							already_registered = true;

							//	stop loop
							return false;
						}
						else
						{
							CYNG_LOG_TRACE(logger_, "client.req.register.push.target - session "
								<< rec_tag
								<< " registered "
								<< target_name
								<< " too");
						}
					}

					//	continue
					return true;
				});

				if (!already_registered)
				{
					//
					//	session found - insert new target
					//
					auto dom = cyng::make_reader(bag);
					const std::uint16_t p_size = cyng::value_cast<std::uint16_t>(dom.get("pSize"), 0xffff);
					const std::uint8_t w_size = cyng::value_cast<std::uint8_t>(dom.get("wSize"), 1);

					success = tbl_target->insert(cyng::table::key_generator(channel)
						, cyng::table::data_generator(tag
							, self
							, target
							, session_rec["device"]	//	owner of target
							, session_rec["name"]	//	name of target owner
							, p_size
							, w_size
							, std::chrono::system_clock::now()	//	reg-time
							, static_cast<std::uint64_t>(0)		//	px
							, static_cast<std::uint64_t>(0))	//	counter
						, 1, tag);
				}
			}
			else
			{
				CYNG_LOG_FATAL(logger_, "client.req.register.push.target - session "
					<< tag
					<< " not found");
			}

		}	, cyng::store::read_access("_Session")
			, cyng::store::write_access("_Target"));

		options["response-code"] = cyng::make_object<std::uint8_t>(success
			? ipt::ctrl_res_register_target_policy::OK
			: ipt::ctrl_res_register_target_policy::GENERAL_ERROR);
		options["target-name"] = cyng::make_object(target);

		ctx.queue(client_res_register_push_target(tag
			, seq
			, success
			, channel		//	channel
			, options
			, bag));
	}

	void client::req_deregister_push_target(cyng::context& ctx)
	{
		//	[4e5dc7e8-37e7-4267-a3b6-d68a9425816f,76fbb814-3e74-419b-8d43-2b7b59dab7f1,67,power@solostec,%(("seq":2),("tp-layer":ipt))]
		//
		//	* remote client tag
		//	* remote peer
		//	* bus sequence
		//	* target
		//	* bag
		//
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.info", "client.req.deregister.push.target", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] remote client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] target name
			cyng::param_map_t		//	[4] bag
		>(frame);

		req_deregister_push_target_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, std::get<4>(tpl)
			, ctx.tag());
	}

	void client::req_deregister_push_target_impl(cyng::context& ctx
		, boost::uuids::uuid tag //	remote tag
		, boost::uuids::uuid peer	//	remote peer
		, std::uint64_t seq
		, std::string target
		, cyng::param_map_t const& bag
		, boost::uuids::uuid self)
	{
		cyng::param_map_t options;

		bool success{ false };
		db_.access([&](const cyng::store::table* tbl_session, cyng::store::table* tbl_target)->void {

			auto session_rec = tbl_session->lookup(cyng::table::key_generator(tag));
			if (!session_rec.empty())
			{
				cyng::table::key_type target_key;
				tbl_target->loop([&](cyng::table::record const& rec)->bool {

					auto target_name = cyng::value_cast<std::string>(rec["name"], "");
					const boost::uuids::uuid owner_tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());
					if (boost::algorithm::equals(target_name, target) && (owner_tag == tag))
					{
						//
						//	target and owner found
						//
						CYNG_LOG_INFO(logger_, "client.req.deregister.push.target - "
							<< target_name
							<< " of owner  "
							<< tag);

						target_key = rec.key();

						//	stop loop
						return false;
					}

					//	continue
					return true;
				});

				if (!target_key.empty())
				{
					success = tbl_target->erase(target_key, tag);
				}
			}
			else
			{
				CYNG_LOG_FATAL(logger_, "client.req.register.push.target - session "
					<< tag
					<< " not found");
			}
		}	, cyng::store::read_access("_Session")
			, cyng::store::write_access("_Target"));

		options["response-code"] = cyng::make_object<std::uint8_t>(success
			? ipt::ctrl_res_deregister_target_policy::OK
			: ipt::ctrl_res_deregister_target_policy::GENERAL_ERROR);

		ctx.queue(client_res_deregister_push_target(tag
			, seq
			, success
			, target		//	channel
			, options
			, bag));
	}

	void client::inc_throughput(cyng::context& ctx)
	{
		//	[31481e4a-7952-40a7-9e13-6bd9255ac84b,812a8baa-41aa-4633-9379-0cb9a562ce62,112]
		const cyng::vector_t frame = ctx.get_frame();
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin tag
			boost::uuids::uuid,		//	[1] target tag
			boost::uuids::uuid,		//	[2] peer
			std::uint64_t			//	[3] bytes transferred
		>(frame);

		db_.access([&](cyng::store::table* tbl_session, cyng::store::table* tbl_connection)->void {

			cyng::table::record rec_origin = tbl_session->lookup(cyng::table::key_generator(std::get<0>(tpl)));
			if (!rec_origin.empty())
			{
				const std::uint64_t rx = cyng::value_cast<std::uint64_t>(rec_origin["rx"], 0u);
				tbl_session->modify(rec_origin.key(), cyng::param_factory("rx", static_cast<std::uint64_t>(rx + std::get<3>(tpl))), std::get<2>(tpl));
			}

			cyng::table::record rec_target = tbl_session->lookup(cyng::table::key_generator(std::get<1>(tpl)));
			if (!rec_target.empty())
			{
				const std::uint64_t sx = cyng::value_cast<std::uint64_t>(rec_target["sx"], 0u);
				tbl_session->modify(rec_target.key(), cyng::param_factory("sx", static_cast<std::uint64_t>(sx + std::get<3>(tpl))), std::get<2>(tpl));
			}

			cyng::table::record rec_conn = connection_lookup(tbl_connection, cyng::table::key_generator(std::get<0>(tpl), std::get<1>(tpl)));
			if (!rec_conn.empty())
			{
				const std::uint64_t throughput = cyng::value_cast<std::uint64_t>(rec_conn["throughput"], 0u);
				tbl_connection->modify(rec_conn.key(), cyng::param_factory("throughput", static_cast<std::uint64_t>(throughput + std::get<3>(tpl))), std::get<2>(tpl));
			}
		}	, cyng::store::write_access("_Session")
			, cyng::store::write_access("_Connection"));
	}

	void client::update_attr(cyng::context& ctx)
	{
		//	[a960e209-c2bb-427f-83a5-b0c0c2e75251,944d1f66-c6dd-4ed8-a96b-fbcc2d07b82a,13,software.version,0.2.2018030,%(("seq":2),("tp-layer":ipt))]
		//
		//	* session tag
		//	* peer
		//	* cluster seq
		//	* attribute name
		//	* value
		//	* bag
		const cyng::vector_t frame = ctx.get_frame();
		ctx.run(cyng::generate_invoke("log.msg.trace", "client.update.attr", frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] origin client tag
			boost::uuids::uuid,		//	[1] peer tag
			std::uint64_t,			//	[2] sequence number
			std::string,			//	[3] name
			cyng::param_map_t		//	[4] bag
		>(frame);

		update_attr_impl(ctx
			, std::get<0>(tpl)
			, std::get<1>(tpl)
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, frame.at(5)
			, std::get<4>(tpl));
	}

	void client::update_attr_impl(cyng::context& ctx
		, boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, std::uint64_t seq
		, std::string name
		, cyng::object value
		, cyng::param_map_t bag)
	{
		ctx.queue(cyng::generate_invoke("log.msg.debug"
			, "client.update.attr"
			, name
			, value));

		if (boost::algorithm::equals(name, "TDevice.vFirmware") || boost::algorithm::equals(name, "TDevice.id"))
		{
			db_.access([&](cyng::store::table const* tbl_session, cyng::store::table* tbl_device, cyng::store::table* tbl_gw)->void {

				auto session_rec = tbl_session->lookup(cyng::table::key_generator(tag));
				if (!session_rec.empty())
				{
					const boost::uuids::uuid dev_tag = cyng::value_cast(session_rec["device"], boost::uuids::nil_uuid());
					const auto dev_pk = cyng::table::key_generator(dev_tag);

					ctx.queue(cyng::generate_invoke("log.msg.info"
						, "update device"
						, dev_tag
						, session_rec["name"]));

					if (boost::algorithm::equals(name, "TDevice.vFirmware"))
					{ 
						//	firmware
						tbl_device->modify(dev_pk, cyng::param_t("vFirmware", value), tag);
					}
					else if (boost::algorithm::equals(name, "TDevice.id"))
					{
						//	model
						tbl_device->modify(dev_pk, cyng::param_t("id", value), tag);

						//
						//	model names are hard coded
						//	EMH-VMET, EMH-VSES
						//
						const std::string model = cyng::value_cast<std::string>(value, "");
						if (boost::algorithm::starts_with(model, "EMH-")	//	this excludes EMHcomBase
							|| boost::algorithm::istarts_with(model, "variomuc")
							|| boost::algorithm::equals(model, "ipt:gateway"))
						{
							auto dev_rec = tbl_device->lookup(dev_pk);

							//
							//	test for gateway (same key as TDevice)
							//
							std::string server_id = "05000000000000";
							auto gw_rec = tbl_gw->lookup(dev_pk);
							if (gw_rec.empty() && !dev_rec.empty())
							{
								//
								//	create a gateway record
								//	a00153b01EA61en ==> 0500153b01EA61
								//
								const std::string dev_name = cyng::value_cast<std::string>(dev_rec["name"], "");
								if (boost::algorithm::starts_with(dev_name, "a00153b") && dev_name.size() == 15)
								{
									server_id = dev_name.substr(1, 12);
								}

								tbl_gw->insert(dev_pk
									, cyng::table::data_generator(server_id
										, (boost::algorithm::equals(model, "ipt:gateway") ? "solosTec" : "EMH")
										, std::chrono::system_clock::now()
										, "factory-nr"
										, cyng::mac48(0, 1, 2, 3, 4, 5)
										, cyng::mac48(0, 1, 2, 3, 4, 6)
										, "user"
										, "pwd"
										, "mbus"
										, "operator"
										, "operator")
									, 0
									, tag);
							}
						}
					}
				}
				else
				{
					ctx.queue(cyng::generate_invoke("log.msg.warning"
						, "client.update.attr - session not found "
						, tag));

				}
			}	, cyng::store::read_access("_Session")
				, cyng::store::write_access("TDevice")
				, cyng::store::write_access("TGateway"));

		}
		else
		{
			ctx.queue(cyng::generate_invoke("log.msg.warning"
				, "client.update.attr - cannot handle "
				, name));
		}
	}

	cyng::table::key_list_t client::get_clients_by_peer(const cyng::store::table* tbl_session, boost::uuids::uuid tag)
	{
		cyng::table::key_list_t pks;
		tbl_session->loop([&](cyng::table::record const& rec) -> bool {

			if (tag == cyng::value_cast(rec["peer"], boost::uuids::nil_uuid()))
			{
				pks.push_back(rec.key());
			}

			//	continue
			return true;
		});

		return pks;
	}

	std::pair<cyng::table::key_list_t, std::uint16_t> collect_matching_targets(const cyng::store::table* tbl
		, boost::uuids::uuid tag
		, std::string const& target)
	{
		//
		//	get a list of matching targets (matching by name)
		//
		cyng::table::key_list_t keys;
		std::uint16_t packet_size = std::numeric_limits<std::uint16_t>::max();
		tbl->loop([&](cyng::table::record const& rec) -> bool {

			const std::string rec_name = cyng::value_cast<std::string>(rec["name"], "");
			const boost::uuids::uuid rec_tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());

			//
			//	dont't open channels to your own targets
			//
			if (tag != rec_tag)
			{
				if (boost::algorithm::starts_with(rec_name, target))
				{
					const std::uint16_t p_size = cyng::value_cast<std::uint16_t>(rec["pSize"], 0);
					//	lowest threshold is 0xff
					if ((p_size < packet_size) && (p_size > 0xff))
					{
						packet_size = p_size;
					}
					keys.push_back(rec.key());
				}
			}

			//	continue
			return true;
		});

		return std::make_pair(keys, packet_size);
	}

	std::tuple<std::uint32_t, std::string, boost::uuids::uuid> get_source_channel(const cyng::store::table* tbl_session, boost::uuids::uuid tag)
	{
		auto session_rec = tbl_session->lookup(cyng::table::key_generator(tag));
		return std::make_tuple(cyng::value_cast<std::uint32_t>(session_rec["source"], 0)
			, cyng::value_cast<std::string>(session_rec["name"], "")
			, cyng::value_cast(session_rec["device"], boost::uuids::nil_uuid()));
	}

	std::size_t remove_targets_by_tag(cyng::store::table* tbl_target, boost::uuids::uuid tag)
	{
		//
		//	get all registered targets of session tag
		//
		cyng::table::key_list_t pks;
		tbl_target->loop([&](cyng::table::record const& rec) -> bool {
			if (tag == cyng::value_cast(rec["tag"], boost::uuids::nil_uuid()))
			{
				pks.push_back(rec.key());
			}

			//	continue
			return true;
		});

		//
		//	remove targets of session tag
		//
		return cyng::erase(tbl_target, pks, tag);
	}

	cyng::table::key_list_t get_targets_by_peer(cyng::store::table const* tbl_target, boost::uuids::uuid tag)
	{
// 		static boost::hash<boost::uuids::uuid> uuid_hasher;

		//
		//	get all registered targets of the specified peer
		//
		cyng::table::key_list_t pks;
		tbl_target->loop([&](cyng::table::record const& rec) -> bool {
			if (tag == cyng::value_cast(rec["peer"], boost::uuids::nil_uuid()))
			{
				pks.push_back(rec.key());
			}

			//	continue
			return true;
		});

		return pks;
	}

	cyng::table::key_list_t get_channels_by_peer(cyng::store::table const* tbl_channel, boost::uuids::uuid tag)
	{
		//
		//	get all registered targets of the specified peer
		//
		cyng::table::key_list_t pks;
		tbl_channel->loop([&](cyng::table::record const& rec) -> bool {
			if (tag == cyng::value_cast(rec["peerChannel"], boost::uuids::nil_uuid()))
			{
				pks.push_back(rec.key());
			}

			//	continue
			return true;
		});

		return pks;
	}

}


