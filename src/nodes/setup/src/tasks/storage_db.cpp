/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/storage_db.h>

#include <smf/config/schemes.h>
#include <smf.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>
#include <cyng/sql/sql.hpp>
#include <cyng/db/details/statement_interface.h>
#include <cyng/db/storage.h>

#include <crypto/hash/sha1.h>

#include <iostream>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace smf {

	storage_db::storage_db(cyng::channel_weak wp
		, cyng::controller& ctl
		, boost::uuids::uuid tag
		, cyng::logger logger
		, cyng::param_map_t&& cfg)
	: sigs_{ 
		std::bind(&storage_db::open, this),
		std::bind(&storage_db::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, ctl_(ctl)
		, tag_(tag)
		, logger_(logger)
		, db_(cyng::db::create_db_session(cfg))
		, store_()
		, store_map_()
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("open", 0);
			//sp->set_channel_name("load-data", 1);
		}

		CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");

	}

	storage_db::~storage_db()
	{
#ifdef _DEBUG_IPT
		std::cout << "cluster(~)" << std::endl;
#endif
	}


	void storage_db::open() {
		if (!db_.is_alive()) {
			CYNG_LOG_ERROR(logger_, "cannot open database");
		}
		else {
			//
			//	database is open fill cache
			//
			CYNG_LOG_INFO(logger_, "database is open");
			init_store();
			load_store();
		}
	}

	void storage_db::init_store() {
		init_store(config::get_store_device());
		init_store(config::get_store_lora());
		init_store(config::get_store_gui_user());
	}

	void storage_db::init_store(cyng::meta_store m) {
		store_map_.emplace(m.get_name(), m);
		if (!store_.create_table(m)) {
			CYNG_LOG_ERROR(logger_, "cannot create table: " << m.get_name());
		}
		else {
			CYNG_LOG_TRACE(logger_, "cache table [" << m.get_name() << "] created" );
		}
	}

	void storage_db::load_store() {
		//
		//	load data from database
		//
		auto const tbl_vec = get_sql_meta_data();
		for (auto const& ms : tbl_vec) {
			load_store(ms);
		}
	}

	void storage_db::load_store(cyng::meta_sql const& ms) {
		//
		//	get in-memory meta data
		//
		auto const m = cyng::to_mem(ms);
		CYNG_LOG_INFO(logger_, "load table [" << ms.get_name() << "/" << m.get_name() << "]");

		store_.access([&](cyng::table* tbl) {

			cyng::db::storage s(db_);
			s.loop(ms, [&](cyng::record const& rec)->bool {

				tbl->insert(rec.key()
					, rec.data()
					, rec.get_generation()
					, tag_);
				return true;
			});
		}, cyng::access::write(m.get_name()));

		//CYNG_LOG_INFO(logger_, "table [" << m.get_name() << " holds " << store_.size(m.get_name()) << " entries");

	}

	void storage_db::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop storage_db task(" << tag_ << ")");
	}


	bool init_storage(cyng::db::session& db) {
		BOOST_ASSERT(db.is_alive());

		try {
			//
			//	start transaction
			//
			cyng::db::transaction	trx(db);

			auto const vec = get_sql_meta_data();
			for (auto const& m : vec) {
				auto const sql = cyng::sql::create(db.get_dialect(), m).to_str();
				if (!db.execute(sql)) {
					std::cerr
						<< "**error: "
						<< sql
						<< std::endl;
				}
			}

			return true;
		}
		catch (std::exception const& ex) {
			boost::ignore_unused(ex);
		}
		return false;
	}

	std::vector< cyng::meta_sql > get_sql_meta_data() {

		return {
			config::get_table_device(),
			config::get_table_lora(),
			config::get_table_gui_user()
		};
	}

	void generate_access_rights(cyng::db::session& db, std::string const& user) {

		auto ms = config::get_table_gui_user();
		auto const sql = cyng::sql::insert(db.get_dialect(), ms).bind_values(ms).to_str();
		auto stmt = db.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {

			auto digest = cyng::sha1_hash("user");

			stmt->push(cyng::make_object(boost::uuids::random_generator()()), 36);	//	pk
			stmt->push(cyng::make_object(1u), 32);	//	generation
			stmt->push(cyng::make_object(user), 32);	//	val
			stmt->push(cyng::make_object(cyng::crypto::digest_sha1(digest)), 40);	//	pwd
			stmt->push(cyng::make_object(std::chrono::system_clock::now()), 0);	//	last login
			stmt->push(cyng::make_object("{devices: [\"view\", \"delete\", \"edit\"], meters: [\"view\", \"edit\"], gateways: [\"view\", \"delete\", \"edit\"]}"), 4096);	//	desc
			if (stmt->execute()) {
				stmt->clear();
			}

		}
	}

	void generate_random_devs(cyng::db::session& db, std::uint32_t count) {

		auto ms = config::get_table_device();
		auto const sql = cyng::sql::insert(db.get_dialect(), ms).bind_values(ms).to_str();
		auto stmt = db.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {
			while (count-- != 0) {

				auto const tag = boost::uuids::random_generator()();
				auto const user = boost::uuids::to_string(tag);

				std::cerr
					<< "#"
					<< count 
					<< " user: "
					<< user
					<< std::endl;

				stmt->push(cyng::make_object(tag), 36);	//	pk
				stmt->push(cyng::make_object(1u), 0);	//	generation
				stmt->push(cyng::make_object(user), 128);	//	name
				stmt->push(cyng::make_object(user.substr(0,8)), 32);	//	pwd
				stmt->push(cyng::make_object(user.substr(25)), 64);	//	msisdn
				stmt->push(cyng::make_object("#" + std::to_string(count)), 512);	//	descr
				stmt->push(cyng::make_object("model-" + std::to_string(count)), 64);	//	id
				stmt->push(cyng::make_object(SMF_VERSION_SUFFIX), 64);	//	vFirmware
				stmt->push(cyng::make_object(false), 0);	//	enabled
				stmt->push(cyng::make_object(std::chrono::system_clock::now()), 0);	//	creationTime
				stmt->push(cyng::make_object(6u), 0);	//	query
				if (stmt->execute()) {
					stmt->clear();
				}
				else {
					std::cerr
						<< "**error: "
						<< sql
						<< std::endl;
					return;
				}
			}
		}
	}

}


