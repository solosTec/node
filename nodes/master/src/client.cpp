/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "client.h"
#include "session.h"
#include <NODE_project_info.h>
#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>
//#include <cyng/vm/domain/log_domain.h>
//#include <cyng/vm/domain/store_domain.h>
//#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
//#include <cyng/object_cast.hpp>
#include <cyng/dom/reader.h>
//#include <cyng/tuple_cast.hpp>
//#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/assert.hpp>

namespace node 
{
	client::client(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& db)
	: mux_(mux)
		, logger_(logger)
		, db_(db)
		, rng_()
		, distribution_(std::numeric_limits<std::uint32_t>::min(), std::numeric_limits<std::uint32_t>::max())
	{
	}

	cyng::vector_t client::req_login(boost::uuids::uuid tag,
		boost::uuids::uuid peer,
		std::uint64_t seq,
		std::string account,
		std::string pwd,
		std::string scheme,
		cyng::param_map_t const& bag,
		cyng::object self)
	{
		cyng::vector_t prg;

		//
		//	test credentials
		//
		bool found{ false };
		db_.access([&](const cyng::store::table* tbl_device, cyng::store::table* tbl_session)->void {

			tbl_device->loop([&](cyng::table::record const& rec) -> bool {

				const auto rec_account = cyng::value_cast<std::string>(rec["name"], "");

				if (boost::algorithm::equals(account, rec_account))
				{
					prg << cyng::unwinder(cyng::generate_invoke("log.msg.info", "account match [", account, "] OK"));

					//
					//	matching name
					//	test password
					//
					const auto rec_pwd = cyng::value_cast<std::string>(rec["pwd"], "");
					if (boost::algorithm::equals(pwd, rec_pwd))
					{
						prg << cyng::unwinder(cyng::generate_invoke("log.msg.info", "password match [", account, "] OK"));

						//
						//	matching password
						//	insert new session
						//
						const auto rec_tag = cyng::value_cast(rec["pk"], boost::uuids::nil_uuid());

						if (tbl_session->insert(cyng::table::key_generator(tag)
							, cyng::table::data_generator(self
								, cyng::make_object()
								, peer
								, rec_tag
								, account
								, distribution_(rng_)
								, std::chrono::system_clock::now()
								, boost::uuids::nil_uuid())
							, 1))
						{
							prg << cyng::unwinder(client_res_login(tag
								, seq
								, true
								, "OK"
								, bag));
						}
						else
						{
							prg << cyng::unwinder(client_res_login(tag
								, seq
								, false
								, "cannot create session"
								, bag));

							prg << cyng::unwinder(cyng::generate_invoke("log.msg.error"
								, "cannot insert new session of account "
								, account
								, " with pk "
								, tag));
						}

						//
						//	terminate loop
						//
						found = true;
					}
					else
					{
						prg << cyng::unwinder(cyng::generate_invoke("log.msg.warning"
							, "password match ["
							, account
							, pwd
							, rec_pwd
							, "failed"));
					}
				}

				//	continue loop
				return !found;
			});

		}	, cyng::store::read_access("TDevice")
			, cyng::store::write_access("*Session"));

		if (!found)
		{
			prg << cyng::unwinder(client_res_login(tag
				, seq
				, false
				, "unknown device"
				, bag));

			prg << cyng::unwinder(cyng::generate_invoke("log.msg.warning"
				, account
				, "not found"));
		}

		return prg;
	}

	cyng::vector_t client::req_open_connection(boost::uuids::uuid tag,
		boost::uuids::uuid peer,
		std::uint64_t seq,
		std::string number,
		cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;

		cyng::param_map_t options;
		options["origin-tag"] = cyng::make_object(tag);	//	send response to this session
		options["local-peer"] = cyng::make_object(peer);	//	and this peer

		bool success{ false };
		db_.access([&](cyng::store::table const* tbl_device, cyng::store::table* tbl_session)->void {

			//
			//	search device with the given number
			//
			prg << cyng::unwinder(cyng::generate_invoke("log.msg.info"
				, "select device with number"
				, number));

			tbl_device->loop([&](cyng::table::record const& rec) -> bool {

				const auto dev_number = cyng::value_cast<std::string>(rec["number"], "");
				if (boost::algorithm::equals(number, dev_number))
				{
					//
					//	device found
					//
					prg << cyng::unwinder(cyng::generate_invoke("log.msg.trace"
						, "matching device", rec.key(), rec.data()));

					//
					//	search session of this device
					//
					const auto dev_tag = cyng::value_cast(rec["pk"], boost::uuids::nil_uuid());
					options["device-name"] = rec["name"];

					tbl_session->loop([&](cyng::table::record const& rec) -> bool {

						const auto ses_tag = cyng::value_cast(rec["device"], boost::uuids::nil_uuid());
						if (dev_tag == ses_tag)
						{
							//
							//	session found
							//
							prg << cyng::unwinder(cyng::generate_invoke("log.msg.trace"
								, "matching session", rec.key(), rec.data()));

							options["remote-tag"] = rec["tag"];
							options["remote-peer"] = rec["peer"];

							//
							//	forward connection open request
							//
							auto remote_tag = cyng::value_cast(rec["tag"], boost::uuids::nil_uuid());
							cyng::object_cast<session>(rec["local"])->vm_.async_run(client_req_open_connection_forward(remote_tag
								, number
								, options
								, bag));

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
			, cyng::store::write_access("*Session"));

		if (!success)
		{
			options["response-code"] = cyng::make_object<std::uint8_t>(ipt::tp_res_open_connection_policy::DIALUP_FAILED);

			prg 
				<< cyng::unwinder(cyng::generate_invoke("log.msg.info", "send connection open response", options))
				<< cyng::unwinder(client_res_open_connection_forward(tag
				, seq
				, success
				, options
				, bag));
		}

		return prg;
	}

	cyng::vector_t client::res_open_connection(boost::uuids::uuid tag,
		boost::uuids::uuid peer,
		std::uint64_t seq,
		bool success,
		cyng::param_map_t const& options,
		cyng::param_map_t const& bag)
	{
		if (success)
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
			db_.access([&](cyng::store::table* tbl_session)->void {

				//
				//	generate tabley keys
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

				//
				//	update "remote" attributes
				//
				tbl_session->modify(caller_tag, cyng::param_t("remote", callee_rec["local"]));
				tbl_session->modify(caller_tag, cyng::param_t("rtag", cyng::make_object(rtag)));

				tbl_session->modify(callee_tag, cyng::param_t("remote", caller_rec["local"]));
				tbl_session->modify(callee_tag, cyng::param_t("rtag", cyng::make_object(tag)));

			}, cyng::store::write_access("*Session"));
		}

		//
		//	forward to caller (origin)
		//
		return client_res_open_connection_forward(tag
			, seq
			, success
			, options
			, bag);

	}

	void client::req_transmit_data(boost::uuids::uuid tag,
		boost::uuids::uuid peer,
		std::uint64_t seq,
		cyng::param_map_t const& bag,
		cyng::object data)
	{
		//
		//	transfer data
		//
		db_.access([&](cyng::store::table* tbl_session)->void {

			//
			//	generate tabley keys
			//
			auto caller_tag = cyng::table::key_generator(tag);

			//
			//	get session objects
			//
			cyng::table::record rec = tbl_session->lookup(cyng::table::key_generator(tag));

			BOOST_ASSERT_MSG(!rec.empty(), "no session record");

			//
			//	transfer data
			//
			boost::uuids::uuid link = cyng::value_cast(rec["rtag"], boost::uuids::nil_uuid());

			CYNG_LOG_INFO(logger_, "client sessions are "
				<< tag
				<< " / "
				<< link);

			auto remote_peer = cyng::object_cast<session>(rec["remote"]);
			if (remote_peer)
			{

				cyng::object_cast<session>(rec["remote"])->vm_.async_run(client_req_transfer_data_forward(link
					, seq
					, bag
					, data));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "remote peer is null");
			}

		}, cyng::store::write_access("*Session"));

	}

}


