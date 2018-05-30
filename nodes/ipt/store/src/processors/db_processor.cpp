/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "db_processor.h"
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/sql.h>
#include <cyng/db/interface_session.h>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	db_processor::db_processor(cyng::io_service_t& ios
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::uint32_t channel
		, std::uint32_t source
		, std::string target
		, cyng::db::session db
		, std::map<std::string, cyng::table::meta_table_ptr>& mtt)
	: logger_(logger)
		, vm_(ios, tag)
		, parser_([this, channel, source, target](cyng::vector_t&& prg) {

			CYNG_LOG_DEBUG(logger_, "db processor "
				<< channel
				<< ':'
				<< source
				<< ':'
				<< target
				<< " - "
				<< prg.size()
				<< " instructions");

			//
			//	execute programm
			//
			vm_.async_run(std::move(prg));

		}, false)
		, db_(db)
		, mt_table_(mtt)
		, exporter_(channel, source, target)
	{
		init();
		CYNG_LOG_INFO(logger_, "DB processor "
			<< channel
			<< ':'
			<< source
			<< ':'
			<< target
			<< " is running");
	}

	void db_processor::init()
	{
		if (vm_.same_thread())
		{
			vm_.register_function("sml.msg", 1, std::bind(&db_processor::sml_msg, this, std::placeholders::_1));
			vm_.register_function("sml.eom", 1, std::bind(&db_processor::sml_eom, this, std::placeholders::_1));
			vm_.register_function("sml.parse", 1, std::bind(&db_processor::sml_parse, this, std::placeholders::_1));
			vm_.register_function("db.insert.meta", 1, std::bind(&db_processor::insert_meta_data, this, std::placeholders::_1));
		}
		else
		{
			vm_.register_function("sml.msg", 1, std::bind(&db_processor::sml_msg, this, std::placeholders::_1));
			vm_.register_function("sml.eom", 1, std::bind(&db_processor::sml_eom, this, std::placeholders::_1));
			vm_.register_function("sml.parse", 1, std::bind(&db_processor::sml_parse, this, std::placeholders::_1));
			vm_.register_function("db.insert.meta", 1, std::bind(&db_processor::insert_meta_data, this, std::placeholders::_1));
		}
		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);

	}

	db_processor::~db_processor()
	{
		stop();
	}

	void db_processor::stop()
	{
		//
		//	halt VM
		//
		vm_.access([this](cyng::vm& vm) {

			//
			//	halt VM
			//
			vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });
			CYNG_LOG_INFO(logger_, "DB processor stopped");
		});
	}

	void db_processor::parse(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "SML/DB processor parse "
			<< data.size()
			<< " bytes");

		parser_.read(data.begin(), data.end());
	}

	void db_processor::sml_parse(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		cyng::buffer_t data;
		data = cyng::value_cast(frame.at(0), data);
		parse(data);
	}

	void db_processor::sml_msg(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();

		//
		//	print sml message number
		//
		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
		CYNG_LOG_INFO(logger_, "DB processor sml.msg #"
			<< idx);

		//
		//	get message body
		//
		cyng::tuple_t msg;
		msg = cyng::value_cast(frame.at(0), msg);

		exporter_.read(ctx, msg, idx);
	}


	void db_processor::sml_eom(cyng::context& ctx)
	{
		//	[5213,3]
		//
		//	* CRC
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();

		const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);

		CYNG_LOG_INFO(logger_, "DB processor sml.eom #"
			<< idx);

		exporter_.reset();

		ctx.attach(cyng::generate_invoke("stop.writer", cyng::code::IDENT));
	}

	void db_processor::insert_meta_data(cyng::context& ctx)
	{
		//	[28ceaacc-a789-4f65-bf82-e03900758c3c,5912287,1,2018-02-21 10:44:55.00000000,2018-02-21 10:45:00.00000000,056d5023,0500153B02297E,01E61E130900163C07,0,4d4a2dbe,e7e1faee,water@solostec]
		//
		//	* pk
		//	* trx
		//	* msg index
		//	* roTime
		//	* actTime
		//	* valTime (index)
		//	* clientId
		//	* serverId
		//	* status
		//	* source
		//	* channel
		//	* target
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "db.insert.meta " << cyng::io::to_str(frame));

		
		cyng::sql::command cmd(mt_table_.find("TSMLMeta")->second, db_.get_dialect());
		cmd.insert();
		auto sql = cmd.to_str();
		auto stmt = db_.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		BOOST_ASSERT(r.first == 12);	//	11 parameters to bind
		BOOST_ASSERT(r.second);

		//	bind parameters
		stmt->push(frame.at(0), 0)	//	pk
			.push(frame.at(1), 16)	//	trxID
			.push(frame.at(2), 0)	//	msgIdx
			.push(frame.at(3), 0)	//	roTime
			.push(frame.at(4), 0)	//	actTime
			.push(frame.at(5), 0)	//	valTime
			.push(frame.at(6), 0)	//	gateway
			.push(frame.at(7), 0)	//	server
			.push(frame.at(8), 0)	//	status
			.push(frame.at(9), 0)	//	source
			.push(frame.at(10), 0)	//	channel
			.push(frame.at(11), 32)	//	target
			;
		stmt->execute();
		stmt->clear();

	}

}
