/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "storage_db.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/db/connection_types.h>
#include <cyng/db/session.h>
#include <cyng/db/interface_session.h>
#include <cyng/db/sql_table.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/sql.h>
#include <cyng/sql/dsl/binary_expr.hpp>
#include <cyng/sql/dsl/list_expr.hpp>
#include <cyng/sql/dsl/operators.hpp>
#include <cyng/sql/dsl/assign.hpp>
#include <cyng/sql/dsl/aggregate.h>
#include <cyng/table/meta.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	storage_db::storage_db(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& cache
		, cyng::param_map_t cfg)
	: base_(*btp)
		, logger_(logger)
		, pool_(base_.mux_.get_io_service(), cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite")))
		, meta_map_(init_meta_map())
		, cache_(cache)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		if (!pool_.start(cfg))
		{
			CYNG_LOG_FATAL(logger_, "to start DB connection pool failed");
		}
		else
		{
			CYNG_LOG_INFO(logger_, "DB connection pool is running with "
				<< pool_.get_pool_size() 
				<< " connection(s)");
		}
	}

	cyng::continuation storage_db::run()
	{	
		CYNG_LOG_INFO(logger_, "storage_db is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_db::stop()
	{
		CYNG_LOG_INFO(logger_, "storage_db is stopped");
	}

	//	slot 0 - merge
	cyng::continuation storage_db::process(std::string name, std::size_t sync_tsk, boost::uuids::uuid tag)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> load table "
			<< name);

		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());
			cmd.select().all();
			std::string sql = cmd.to_str();
			CYNG_LOG_TRACE(logger_, sql);	//	select ... from name

			auto stmt = s.create_statement();
			stmt->prepare(sql);

			//
			//	lookup cache
			//
			std::size_t counter{ 0 };
			cache_.access([&](cyng::store::table* tbl)->void {

				BOOST_ASSERT(meta->get_name() == tbl->meta().get_name());

				//
				//	read all results and populate cache
				//
				while (auto res = stmt->get_result())
				{
					//
					//	convert SQL result to record
					//
					cyng::table::record rec = cyng::to_record(meta, res);

					//
					//	ToDo: transform SQL record into cache record
					//
					if (!tbl->insert(rec.key(), rec.data(), rec.get_generation(), tag))
					{
						CYNG_LOG_ERROR(logger_, "copy record into cache " 
							<< meta->get_name()
							<< " failed - "
							<< cyng::io::to_str(rec.key()));
					}
					else
					{
						CYNG_LOG_TRACE(logger_, "insert record into cache "
							<< meta->get_name()
							<< " - "
							<< cyng::io::to_str(rec.key()));
						counter++;
					}
				}

				CYNG_LOG_INFO(logger_, "cache "
					<< meta->get_name()
					<< " is populated with "
					<< counter
					<< " records");

			}, cyng::store::write_access(name));

			//
			//	cache complete
			//	tell sync to synchronise the cache with master
			//
			BOOST_ASSERT(counter == cache_.size(name));
			base_.mux_.post(sync_tsk, 0, cyng::tuple_factory(name, counter));
		}
		else
		{
			CYNG_LOG_FATAL(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown table "
				<< name);
			base_.mux_.post(sync_tsk, 0, cyng::tuple_factory(name, 0));
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation storage_db::process(std::string name
		, cyng::vector_t const& key
		, cyng::vector_t data
		, std::uint64_t gen)
	{
		std::reverse(data.begin(), data.end());

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> insert "
			<< name
			<< " "
			<< cyng::io::to_str(key)
			<< cyng::io::to_str(data)
			<< " - gen "
			<< gen);

		try
		{
			//
			//	insert into SQL database
			//
			auto pos = meta_map_.find(name);
			if (pos != meta_map_.end())
			{
				//
				//	test consistency
				//
				BOOST_ASSERT_MSG(pos->second->check_key(key), "invalid key");
				BOOST_ASSERT_MSG(pos->second->size() == (key.size() + data.size() + 1), "invalid key or data");

				auto s = pool_.get_session();
				cyng::table::meta_table_ptr meta = (*pos).second;
				cyng::sql::command cmd(meta, s.get_dialect());
				cmd.insert();
				std::string sql = cmd.to_str();
				//CYNG_LOG_TRACE(logger_, sql);	//	insert

				auto stmt = s.create_statement();
				std::pair<int, bool> r = stmt->prepare(sql);
				boost::ignore_unused(r);    //  for release versions

				//
				//	test 
				//
				BOOST_ASSERT(r.second);
				BOOST_ASSERT_MSG(r.first == pos->second->size(), "invalid key or data");


				if (boost::algorithm::equals(name, "TDevice"))
				{
					//	[763ae055-449c-4783-b383-8fc8cd52f44f]
					//	[2018-01-23 15:10:47.65306710,true,vFirmware,id,descr,number,name]
					//	bind parameters
					//	INSERT INTO TDevice (pk, gen, name, pwd, number, descr, id, vFirmware, enabled, creationTime, query) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
					stmt->push(key.at(0), 36);	//	pk
					meta->loop_body([&](cyng::table::column&& col) {
						if (col.pos_ == 0) 	{
							stmt->push(cyng::make_object(gen), col.width_);
						}
						else {
							stmt->push(data.at(col.pos_ - 1), col.width_);
						}
					});
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql insert failed: " << sql);
						CYNG_LOG_TRACE(logger_, "pk [" << key.size() << "]: " << cyng::io::to_str(key));
						CYNG_LOG_TRACE(logger_, "data [" << data.size() << "]: " << cyng::io::to_str(data));
					}
					else {
						stmt->clear();
					}

				}
				else if (boost::algorithm::equals(name, "TGateway"))
				{
					//	[54c15b9e-858a-431c-9c2c-8b654c7d7651][05000000000000,EMH,EMH-VMET,2018-06-06 07:22:47.26852400,VARIOMUC-ETHERNET-1.407_14232___11X022a,factory-nr,00:01:02:03:04:05,00:01:02:03:04:06,user,pwd,mbus,operator,operator]

					//meta_map.emplace("TGateway", cyng::table::make_meta_table<1, 14>("TGateway",
					//	{ "pk", "gen", "serverId", "manufacturer", "model", "proddata", "vFirmware", "factoryNr", "ifService", "ifData", "pwdDef", "pwdRoot", "mbus", "userName", "userPwd" },
					//	{ cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_TIME_POINT, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING },
					//	{ 36, 0, 23, 64, 64, 0, 64, 8, 18, 18, 32, 32, 16, 32, 32 }));

					stmt->push(key.at(0), 36);	//	pk (same as TDevice)
					meta->loop_body([&](cyng::table::column&& col) {
						if (col.pos_ == 0)
						{
							stmt->push(cyng::make_object(gen), col.width_);
						}
						else
						{
							stmt->push(data.at(col.pos_ - 1), col.width_);
						}
					});
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql insert failed: " << sql);
						CYNG_LOG_TRACE(logger_, "pk [" << key.size() << "]: " << cyng::io::to_str(key));
						CYNG_LOG_TRACE(logger_, "data [" << data.size() << "]: " << cyng::io::to_str(data));
					}
					else {
						stmt->clear();
					}
				}
				else if (boost::algorithm::equals(name, "TLoRaDevice"))
				{
					//	insert TLoRaDevice 
					//	[f4f697c0-670f-4897-9500-adfc9bff60fb]
					//	[0100:0302:0504:0706,1122334455667788990011223344556677889900112233445566778899001122,demo,true,100,0100:0302:0604:0807,0100:0302:0604:0807] - gen 8
					stmt->push(key.at(0), 36)	//	pk
						.push(cyng::make_object(gen), 0)	//	generation
						.push(data.at(0), 19)	//	DevEUI
						.push(data.at(1), 0)	//	AESKey
						.push(data.at(2), 0)	//	driver
						.push(data.at(3), 0)	//	activation
						.push(data.at(4), 0)	//	DevAddr
						.push(data.at(5), 0)	//	AppEUI
						.push(data.at(6), 0)	//	GatewayEUI
						;
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql insert failed: " << sql);
						CYNG_LOG_TRACE(logger_, "pk [" << key.size() << "]: " << cyng::io::to_str(key));
						CYNG_LOG_TRACE(logger_, "data [" << data.size() << "]: " << cyng::io::to_str(data));
					}
					else {
						stmt->clear();
					}
				}
				else if (boost::algorithm::equals(name, "TLoraUplink"))
				{
					//	INSERT INTO TLoraUplink (pk, DevEUI, roTime, FPort, FCntUp, ADRbit, MType, FCntDn, payload, mic, LrrRSSI, LrrSNR, SpFact, SubBand, Channel, DevLrrCnt, Lrrid, CustomerID, LrrLAT, LrrLON) VALUES (?, ?, julianday(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
					//	[96205c3e-3973-4db0-b07c-294e58596720][5,52,100000728,29000071,1,LC2,G1,12,7,-99,444eefd3,32332...3538,148,0,1,1,0001,2015-10-22 13:39:59.48900000,F03D291000001180]
					BOOST_ASSERT(r.first == 21);	//	20 parameters to bind
					BOOST_ASSERT_MSG(key.size() == 1, "wrong TLoraUplink key size");
					BOOST_ASSERT_MSG(data.size() == 19, "wrong TLoraUplink data size");
					stmt->push(key.at(0), 36)	//	pk
						.push(cyng::make_object(gen), 0)	//	generation
						.push(data.at(18), 19)	//	DevEUI
						.push(data.at(17), 0)	//	roTime
						.push(data.at(16), 0)	//	FPort
						.push(data.at(15), 0)	//	FCntUp
						.push(data.at(14), 0)	//	ADRbit
						.push(data.at(13), 0)	//	MType
						.push(data.at(12), 0)	//	FCntDn
						.push(data.at(11), 40)	//	payload_hex
						.push(data.at(10), 8)	//	mic_hex
						.push(data.at(9), 0)	//	LrrRSSI
						.push(data.at(8), 0)	//	LrrSNR
						.push(data.at(7), 8)	//	SpFact
						.push(data.at(6), 8)	//	SubBand
						.push(data.at(5), 8)	//	Channel
						.push(data.at(4), 0)	//	DevLrrCnt
						.push(data.at(3), 8)	//	Lrrid
						.push(data.at(2), 16)	//	CustomerID
						.push(data.at(1), 0)	//	LrrLAT
						.push(data.at(0), 0)	//	LrrLON
						;
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql insert failed: " << sql);
						CYNG_LOG_TRACE(logger_, "pk [" << key.size() << "]: " << cyng::io::to_str(key));
						CYNG_LOG_TRACE(logger_, "data [" << data.size() << "]: " << cyng::io::to_str(data));
					}
					else {
						stmt->clear();
					}
				}
				else if (boost::algorithm::equals(name, "TMeter"))
				{
					//	insert TMeter 
					stmt->push(key.at(0), 36)	//	pk
						.push(cyng::make_object(gen), 0)	//	generation
						.push(data.at(0), 24)	//	ident
						.push(data.at(1), 8)	//	meter
						.push(data.at(2), 64)	//	maker
						.push(data.at(3), 0)	//	tom
						.push(data.at(4), 64)	//	vFirmware
						.push(data.at(5), 64)	//	vParam
						.push(data.at(6), 32)	//	factoryNr
						.push(data.at(7), 128)	//	item
						.push(data.at(8), 8)	//	mClass
						.push(data.at(9), 36)	//	gw
						;
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql insert failed: " << sql);
						CYNG_LOG_TRACE(logger_, "pk [" << key.size() << "]: " << cyng::io::to_str(key));
						std::size_t idx{ 0 };
						for (auto const& obj : data) {
							CYNG_LOG_TRACE(logger_, "data [" 
								<< idx++ 
								<< "] " 
								<< obj.get_class().type_name()
								<< ": "
								<< cyng::io::to_str(obj))
								;
						}
					}
					else {
						stmt->clear();
					}
				}
				else
				{
					CYNG_LOG_ERROR(logger_, "unknown table " << name);	//	insert
				}
			}
			else
			{
				CYNG_LOG_FATAL(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> unknown table "
					<< name);
			}
		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> table "
				<< name
				<< " exception: "
				<< ex.what());
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation storage_db::process(std::string name
		, cyng::vector_t const& key
		, cyng::param_t const& data
		, std::uint64_t gen)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> update "
			<< name
			<< ' '
			<< cyng::io::to_str(key)
			<< ' '
			<< data.first
			<< " = "
			<< cyng::io::to_str(data.second));

		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());

			//
			//	To add 1 hack is required since the column index starts with 1 (and not 0)
			//	The where clause works only for tables with a simple primary key.
			//
			std::pair<std::ptrdiff_t, bool> idx = meta->get_record_index(data.first);
			if (idx.second)
			{
				//
				//	update specific attribute
				//
				cmd.update(cyng::sql::make_assign(idx.first + 1, cyng::sql::make_placeholder())).where(cyng::sql::column(1) == cyng::sql::make_placeholder());

				std::string sql = cmd.to_str();
				//CYNG_LOG_TRACE(logger_, sql);	//	update

				auto stmt = s.create_statement();
				std::pair<int, bool> r = stmt->prepare(sql);
				if (!r.second)
				{
					//	maybe database connection was closed
					CYNG_LOG_FATAL(logger_, "sql prepare failed: " << sql);
					CYNG_LOG_INFO(logger_, "test if database connection was closed");
					//BOOST_ASSERT(r.second);
				}
				else
				{
					stmt->push(data.second, 0)
						.push(key.at(0), 36);
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql update failed: " << sql);
					}
					stmt->clear();
				}

				//
				//	update gen(eration)
				//
				cmd.update(cyng::sql::make_assign(2, cyng::sql::make_placeholder())).where(cyng::sql::column(1) == cyng::sql::make_placeholder());
				sql = cmd.to_str();
				//CYNG_LOG_TRACE(logger_, sql);	//	update

				//stmt = s.create_statement();
				r = stmt->prepare(sql);
				if (!r.second)
				{
					//	maybe database connection was closed
					CYNG_LOG_FATAL(logger_, "sql prepare failed: " << sql);
					CYNG_LOG_INFO(logger_, "test if datanase connection was closed");
					//BOOST_ASSERT(r.second);
				}
				else
				{
					stmt->push(cyng::make_object(gen), 0)
						.push(key.at(0), 36);
					if (!stmt->execute())
					{
						CYNG_LOG_ERROR(logger_, "sql update failed: " << sql);
					}
					stmt->clear();
				}
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "sql update unknown index: " << data.first);

			}
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "unknown table "
				<< name);	//	insert
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation storage_db::process(std::string name
		, cyng::vector_t const& key)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> remove "
			<< name
			<< " "
			<< cyng::io::to_str(key));
		auto pos = meta_map_.find(name);
		if (pos != meta_map_.end())
		{
			auto s = pool_.get_session();
			cyng::table::meta_table_ptr meta = (*pos).second;
			cyng::sql::command cmd(meta, s.get_dialect());

			cmd.remove().by_key();	//	different key size
			std::string sql = cmd.to_str();
			CYNG_LOG_TRACE(logger_, sql);	//	update

			auto stmt = s.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			BOOST_ASSERT(r.second);
			BOOST_ASSERT(r.first == key.size());
            boost::ignore_unused(r);    //  for release versions

			//stmt->push(key.at(0), 36);
			for (int idx = decltype(key.size()){0}; idx < key.size(); ++idx)
			{
				stmt->push(key.at(idx), 0);
			}
			if (!stmt->execute())
			{
				CYNG_LOG_ERROR(logger_, "sql delete failed: " << sql);
			}
			stmt->clear();

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "unknown table "
				<< name);	//	remove
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	int storage_db::init_db(cyng::tuple_t tpl, std::size_t count)
	{
		auto cfg = cyng::to_param_map(tpl);
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second)
		{
			//
			//	connect string
			//
			std::cout << "connect string: " << r.first << std::endl;

			auto meta_map = storage_db::init_meta_map();
			for (auto tbl : meta_map)
			{
				cyng::sql::command cmd(tbl.second, s.get_dialect());
				cmd.create();
				std::string sql = cmd.to_str();
				std::cout << sql << std::endl;
				s.execute(sql);

				if (tbl.first == "TDevice")
				{
					for (std::size_t idx = 0; idx < count; ++idx)
					{
						cmd.insert();
						sql = cmd.to_str();
						auto stmt = s.create_statement();
						std::pair<int, bool> r = stmt->prepare(sql);
						BOOST_ASSERT(r.first == 11);	//	11 parameters to bind
						BOOST_ASSERT(r.second);
                        boost::ignore_unused(r);    //  for release versions

						//	bind parameters
						stmt->push(cyng::make_object(boost::uuids::random_generator()()), 0)
							.push(cyng::make_object<std::uint64_t>(idx), 0)
							.push(cyng::make_object("device-" + std::to_string(idx + 1)), 128)
							.push(cyng::make_object("secret"), 16)
							.push(cyng::make_object(std::to_string(idx + 1001)), 128)
							.push(cyng::make_object("comment #" + std::to_string(idx + 1)), 512)
							.push(cyng::make_object("ID"), 64)
							.push(cyng::make_object("v" + std::to_string(idx + 1)), 64)
							.push(cyng::make_object(true), 0)
							.push(cyng::make_object(std::chrono::system_clock::now()), 0)
							.push(cyng::make_object<std::uint32_t>((((idx + 1)% 7) == 0) ? 319 : 6), 0)
							;
						stmt->execute();
						stmt->clear();
					}
				}
#ifdef _DEBUG
				else if (tbl.first == "TLL")
				{
					cmd.insert();
					sql = cmd.to_str();
					auto stmt = s.create_statement();
					std::pair<int, bool> r = stmt->prepare(sql);
					//BOOST_ASSERT(r.first == 9);	//	3 parameters to bind
					BOOST_ASSERT(r.second);

					//	bind parameters
					stmt->push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(14ull), 0)
						.push(cyng::make_object("Leased Line -  1"), 128)
						.push(cyng::make_object(true), 0)
						.push(cyng::make_object(std::chrono::system_clock::now()), 0)
						;
					stmt->execute();
					stmt->clear();

					r = stmt->prepare(sql);
					//	bind parameters
					stmt->push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(boost::uuids::random_generator()()), 0)
						.push(cyng::make_object(441ull), 0)
						.push(cyng::make_object("Leased Line - 2"), 128)
						.push(cyng::make_object(false), 0)
						.push(cyng::make_object(std::chrono::system_clock::now()), 0)
						;
					stmt->execute();
					stmt->clear();
				}
#endif
			}



			return EXIT_SUCCESS;
		}
		std::cerr << "connect to database failed" << std::endl;
		return EXIT_FAILURE;
	}

	std::map<std::string, cyng::table::meta_table_ptr> storage_db::init_meta_map()
	{
		//
		//	SQL table scheme
		//
		std::map<std::string, cyng::table::meta_table_ptr> meta_map;
		meta_map.emplace("TDevice", cyng::table::make_meta_table<1, 10>("TDevice",
			{ "pk", "gen", "name", "pwd", "msisdn", "descr", "id", "vFirmware", "enabled", "creationTime", "query" },
			{ cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT, cyng::TC_UINT32 },
			{ 36, 0, 128, 16, 128, 512, 64, 64, 0, 0, 0 }));

		meta_map.emplace("TUser", cyng::table::make_meta_table<1, 6>("TUser",
			{ "pk", "gen", "name", "team", "priv_read", "priv_write", "priv_delete" },
			{ cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_BOOL, cyng::TC_BOOL, cyng::TC_BOOL },
			{ 36, 0, 128, 0, 0, 0, 0 }));

		meta_map.emplace("TGroup", cyng::table::make_meta_table<1, 4>("TGroup",
			{ "id", "gen", "descr", "admin", "shortname" },
			{ cyng::TC_UINT32, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_STRING },
			{ 0, 0, 256, 0, 64 }));

		//	vFirmware-id: (i.e. MUC-ETHERNET-1.318_11332000)
		//	factoryNr: (i.e. 06441734)
		//	mbus: W-Mbus ID (i.e. A815408943050131)
		meta_map.emplace("TGateway", cyng::table::make_meta_table<1, 12>("TGateway",
			{ "pk"
			, "gen"
			, "serverId"	//	05 + MAC
			, "manufacturer"
			, "proddata"	//	production date
			, "factoryNr"	//	example: "06441734"
			, "ifService"	//	MAC service interface
			, "ifData"		//	MAC data interface
			, "pwdDef"		//	default password
			, "pwdRoot"		//	root password
			, "mbus"		//	W-Mbus ID (i.e. A815408943050131)
			, "userName"	//	usually: operator
			, "userPwd"		//	usually: operator
			},
			{ cyng::TC_UUID
			, cyng::TC_UINT64	//	gen
			, cyng::TC_STRING	//	server ID
			, cyng::TC_STRING	//	manufacturer
			, cyng::TC_TIME_POINT
			, cyng::TC_STRING	//	factoryNr
			, cyng::TC_STRING	//	ifService
			, cyng::TC_STRING	//	ifData
			, cyng::TC_STRING	//	pwdDef
			, cyng::TC_STRING	//	pwdRoot
			, cyng::TC_STRING	//	mbus
			, cyng::TC_STRING	//	userName
			, cyng::TC_STRING	//	userPwd
			},
			{ 36, 0, 23, 64, 0, 8, 18, 18, 32, 32, 16, 32, 32 }));

		//	id: (i.e. 1EMH0006441734)
		//	vParam: parameterization software version (i.e. 16A098828.pse)
		//	vFirmware: (i.e. 11600000)
		//	item: artikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
		//	class: Metrological Class: A, B, C, Q3/Q1, ...
		meta_map.emplace("TMeter", cyng::table::make_meta_table<1, 11>("TMeter",
			{ "pk"
			, "gen"			//	generation
			, "ident"		//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
			, "meter"		//	meter number (i.e. 16000913) 4 bytes 
			, "maker"		//	manufacturer
			, "tom"			//	time of manufacture
			, "vFirmware"	//	firmware version (i.e. 11600000)
			, "vParam"		//	parametrierversion (i.e. 16A098828.pse)
			, "factoryNr"	//	fabrik nummer (i.e. 06441734)
			, "item"		//	ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
			, "mClass"		//	Metrological Class: A, B, C, Q3/Q1, ...
			, "gw"			//	optional gateway pk
			},
			{ cyng::TC_UUID			//	pk
			, cyng::TC_UINT64		//	gen
			, cyng::TC_STRING		//	ident
			, cyng::TC_STRING		//	meter
			, cyng::TC_STRING		//	maker
			, cyng::TC_TIME_POINT	//	tom
			, cyng::TC_STRING		//	vFirmware
			, cyng::TC_STRING		//	vParam
			, cyng::TC_STRING		//	factoryNr
			, cyng::TC_STRING		//	item
			, cyng::TC_STRING		//	mClass
			, cyng::TC_UUID			//	gw
			},
			{ 36	//	pk
			, 0		//	gen
			, 24	//	ident
			, 8		//	meter
			, 64	//	maker
			, 0		//	tom
			, 64	//	vFirmware
			, 64	//	vParam
			, 32	//	factoryNr
			, 128	//	item
			, 8		//	mClass 
			, 36	//	gw
			}));

		//	https://www.thethingsnetwork.org/docs/lorawan/address-space.html#devices
		//	DevEUI - 64 bit end-device identifier, EUI-64 (unique)
		//	DevAddr - 32 bit device address (non-unique)
		meta_map.emplace("TLoRaDevice", cyng::table::make_meta_table<1, 8>("TLoRaDevice",
			{ "pk"
			, "gen"
			, "DevEUI"
			, "AESKey"		// 256 Bit
			, "driver"	
			, "activation"	//	OTAA/ABP
			, "DevAddr"		//	32 bit device address (non-unique)
			, "AppEUI"		//	64 bit application identifier, EUI-64 (unique)
			, "GatewayEUI"	//	64 bit gateway identifier, EUI-64 (unique)
			},
			{ cyng::TC_UUID
			, cyng::TC_UINT64	//	gen
			, cyng::TC_MAC64	//	DevEUI
			, cyng::TC_STRING	//	AESKey
			, cyng::TC_STRING	//	driver
			, cyng::TC_BOOL		//	activation
			, cyng::TC_UINT32	//	DevAddr
			, cyng::TC_MAC64	//	AppEUI
			, cyng::TC_MAC64	//	GatewayEUI
			},
			{ 36, 0, 0, 64, 32, 0, 0, 0, 0 }));

		meta_map.emplace("TLL", cyng::table::make_meta_table<2, 4>("TLL",
			{ "first"
			, "second"
			, "gen"
			, "descr"
			, "enabled"
			, "creationTime" },
			{ cyng::TC_UUID, cyng::TC_UUID, cyng::TC_UINT64, cyng::TC_STRING, cyng::TC_BOOL, cyng::TC_TIME_POINT },
			{ 36, 36, 0, 128, 0, 0 }));

		meta_map.emplace("TSysMsg", cyng::table::make_meta_table<1, 3>("TSysMsg",
			{ "id"
			, "ts"
			, "severity"
			, "msg" },
			{ cyng::TC_UINT64, cyng::TC_TIME_POINT, cyng::TC_UINT8, cyng::TC_STRING },
			{ 0, 0, 0, 256 }));

		//
		//	meta data (without payload)
		//
		meta_map.emplace("TLoraUplink", cyng::table::make_meta_table<1, 20>("TLoraUplink",
			{ "pk"
			, "gen"		//	virtual
			, "DevEUI"
			, "roTime"
			, "FPort" 
			, "FCntUp"
			, "ADRbit"
			, "MType"
			, "FCntDn"
			, "payload"	//	payload_hex
			, "mic"		//	mic_hex
						//	skip <Lrcid>
			, "LrrRSSI"
			, "LrrSNR"
			, "SpFact"
			, "SubBand"
			, "Channel"
			, "DevLrrCnt"
			, "Lrrid"		//	Lrrid
							//	skip <Lrrs>
			, "CustomerID"	//	CustomerID
							//	skip <CustomerData>
							//	skip <ModelCfg>
							//	There are geo data available: latitude, longitute
							//	<LrrLAT>47.386738< / LrrLAT>
							//	<LrrLON>8.522902< / LrrLON>
			, "LrrLAT"
			, "LrrLON"
			},

			{ cyng::TC_UUID			//	pk [boost::uuids::uuid]
			, cyng::TC_UINT64		//	gen
			, cyng::TC_STRING		//	DevEUI [cyng::mac64]
			//, cyng::TC_MAC64		//	DevEUI [cyng::mac64] - ToDo: implement adaptor
			, cyng::TC_TIME_POINT	//	roTime [ std::chrono::system_clock::time_point]
			, cyng::TC_UINT16		//	FPort [std::uint16_t]
			, cyng::TC_INT32		//	FCntUp [std::int32_t]
			, cyng::TC_INT32		//	ADRbit [std::int32_t]
			, cyng::TC_INT32		//	MType [std::int32_t]
			, cyng::TC_INT32		//	FCntDn [std::int32_t]
			, cyng::TC_STRING		//	payload_hex [std::string]
			, cyng::TC_STRING		//	mic_hex [std::string]
			, cyng::TC_DOUBLE		//	LrrRSSI [double]
			, cyng::TC_DOUBLE		//	LrrSNR [double]
			, cyng::TC_STRING		//	SpFact [std::string]
			, cyng::TC_STRING		//	SubBand [std::string]
			, cyng::TC_STRING		//	Channel [std::string]
			, cyng::TC_INT32		//	DevLrrCnt [std::int32_t]
			, cyng::TC_STRING		//	Lrrid [std::string]
			, cyng::TC_STRING		//	CustomerID [std::string]
			, cyng::TC_DOUBLE		//	LrrLAT
			, cyng::TC_DOUBLE		//	LrrLON
			},
			{ 36, 0, 0, 0, 0, 0, 0, 0, 0, 40, 8, 0, 0, 8, 8, 8, 0, 8, 16, 0, 0 }));

		meta_map.emplace("TLoraLocation", cyng::table::make_meta_table<1, 13>("TLoraLocation",
			{ "pk"
			, "gen"		//	virtual
			, "DevEUI"
			, "DevAddr"
			, "Lrcid"
			, "roTime"
			, "DevLocTime"
			, "DevLAT"
			, "DevLON"
			, "DevAlt"
			, "DevAcc"
			, "DevLocRadius"
			, "DevAltRadius"
			, "CustomerID"	//	CustomerID
			},

			{ cyng::TC_UUID			//	pk [boost::uuids::uuid]
			, cyng::TC_UINT64		//	gen
			, cyng::TC_MAC64		//	DevEUI [cyng::mac64]
			, cyng::TC_UINT32		//	DevAddr [std::uint32_t]
			, cyng::TC_UINT32		//	Lrcid [std::uint32_t]
			, cyng::TC_TIME_POINT	//	roTime [ std::chrono::system_clock::time_point]
			, cyng::TC_TIME_POINT	//	DevLocTime [ std::chrono::system_clock::time_point]
			, cyng::TC_DOUBLE		//	DevLAT [double]
			, cyng::TC_DOUBLE		//	DevLON [double]
			, cyng::TC_DOUBLE		//	DevAlt [double]
			, cyng::TC_DOUBLE		//	DevAcc [double]
			, cyng::TC_DOUBLE		//	DevLocRadius [double]
			, cyng::TC_DOUBLE		//	DevAltRadius [double]
			, cyng::TC_STRING		//	CustomerID [std::string]
			},
			{ 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16 }));

		meta_map.emplace("TLoraPayload", cyng::table::make_meta_table<1, 15>("TLoraPayload",
			{ "pk"		
			, "gen"		//	virtual
			, "type"	//	version
			, "manufacturer"
			, "id"
			, "medium"
			, "state"
			, "actd"	//	actuality duration
			, "vif"
			, "volume"
			, "value"
			, "afun"
			, "lifetime"	//	battery lifetime (plain)
			, "semester"	//	battery lifetime (calculated semester)
			, "lnkErr"		//	link error (bit 2)
			, "CRC"
			},
			{ cyng::TC_UUID			//	pk [boost::uuids::uuid]
			, cyng::TC_UINT64		//	gen
			, cyng::TC_UINT8		//	type [std::uint8_t]
			, cyng::TC_STRING		//	manufacturer [std::string]
			, cyng::TC_STRING		//	meter ID [std::string]
			, cyng::TC_UINT8		//	medium [std::uint8_t]
			, cyng::TC_UINT8		//	M-Bus state [std::uint8_t]
			, cyng::TC_UINT16		//	actuality duration [std::uint16_t]
			, cyng::TC_UINT8		//	Volume VIF [std::uint8_t]
			, cyng::TC_UINT32		//	Volume [std::uint32_t]
			, cyng::TC_STRING		//	value [std::string]
			, cyng::TC_UINT8		//	addition functions [std::uint8_t]
			, cyng::TC_UINT8		//	battery lifetime (semester) [std::uint8_t]
			, cyng::TC_UINT32		//	battery lifetime (calculated) [std::uint32_t]
			, cyng::TC_BOOL			//	link error (bit 2) [bool]
			, cyng::TC_BOOL			//	CRC OK [bool]
			},
			{ 36, 0, 0, 3, 8, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0 }));

		return meta_map;
	}

}
