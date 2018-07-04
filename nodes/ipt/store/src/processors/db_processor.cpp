/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "db_processor.h"
#include <NODE_project_info.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
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
		, std::string const& schema
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

		}, false, false)	//	not verbose, no log instructions
		, db_(db)
		, schema_(schema)
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
		vm_.register_function("sml.msg", 1, std::bind(&db_processor::sml_msg, this, std::placeholders::_1));
		vm_.register_function("sml.eom", 1, std::bind(&db_processor::sml_eom, this, std::placeholders::_1));
		vm_.register_function("sml.parse", 1, std::bind(&db_processor::sml_parse, this, std::placeholders::_1));
		vm_.register_function("db.insert.meta", 14, std::bind(&db_processor::insert_meta_data, this, std::placeholders::_1));
		vm_.register_function("db.insert.data", 10, std::bind(&db_processor::insert_value_data, this, std::placeholders::_1));

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);

	}

	db_processor::~db_processor()
	{
		//stop();
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

		//exporter_.reset();

		ctx.attach(cyng::generate_invoke("stop.writer", cyng::code::IDENT));
	}

	void db_processor::insert_meta_data(cyng::context& ctx)
	{
		//	[28ceaacc-a789-4f65-bf82-e03900758c3c,5912287,1,2018-02-21 10:44:55.00000000,2018-02-21 10:45:00.00000000,056d5023,0500153B02297E   ,01E61E130900163C07,0,4d4a2dbe,e7e1faee,water@solostec]
		//	[0bf4bfff-6a83-411d-ab60-1e98fc232fd2,51207  ,1,2018-06-29 19:59:14.00000000,2018-07-01 22:24:00.00000000,05e01d33,00:15:3b:02:29:7e,01-e61e-13090016-3c-07,0,b1094be7,9db50fd3,water@solostec]
		//	[e78d2908-6b48-476e-a5fc-1bd7cf0ca73c,516287911,1,null,2018-07-03 21:02:03.00000000,046ec328,null,null,012D2C887599691C04,01-2d2c-88759969-1c-04,0,915cc253,39320ca2,pushStore]
		//
		//	* pk
		//	* trx
		//	* msg index
		//	* roTime
		//	* actTime
		//	* valTime (index)
		//	* client
		//	* clientId
		//	* server
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

		if (boost::algorithm::equals(schema_, "v4.0"))
		{
			//
			//	examples:
			//
			//	sqlite> select * from TSMLMeta where roTime > julianday('2018-07-01');
			//	ident|trxID|midx|roTime|actTime|valTime|gateway|server|meter|status|source|channel
			//	91e54897-68aa-477b-97ae-efe459c192be|618732131|1|2458300.51030093|2458300.51070602|63122537|0500153b025459|01-2d2c-40800078-1c-04|78008040|0|949333985|581869302
			//	fa6f80d6-2a18-4873-ae15-407629529757|535249935|1|2458300.51025463|2458300.51070602|81491646|0500153b02499e|01-a815-76007904-01-02|04790076|131712|545404204|581869302
			//	5e45f89d-9a36-4f9b-8e79-8fdd617ace8f|65331623|1|2458300.50975694|2458300.51070602|41419967|0500153b023b36|02-d81c-05252359-ff-02|59232505|0|-1579004998|581869302
			//	a21ee414-8442-4b5b-bab0-2a815a034e73|510800075|1|2458300.50994213|2458300.51070602|55253893|0500153b025182|01-a815-77007904-01-02|04790077|131712|-372047867|581869302
			//	1df825d4-93f7-4518-b493-030b150ea3fa|439163211|1|2458300.51001157|2458300.51070602|92472720|0500153b024974|01-a815-46007904-01-02|04790046|131712|-1946129057|581869302
			//	c664ca3b-4dbf-4473-a847-f404118ce3fc|612380399|1|2458300.51028935|2458300.51070602|89139036|0500153b024996|01-a815-73007904-01-02|04790073|131714|-133711905|581869302
			//	5d2f7607-21b4-4a0b-835e-055118d8feb9|523186555|3|2458300.51025463|2458300.51070602|81442784|0500153b024998|01-a815-75007904-01-02|04790075|131712|-30574576|581869302
			//	90202d0f-46df-4f4e-9327-cd77ef96d0bb|51786655|4|2458300.51037037|2458300.51070602|44248573|0500153b024952|01-a815-65957202-01-02|02729565|640|418932835|581869302
			//	f29ff3f0-f570-43a5-93f2-933838e42d1f|514196843|4|2458300.51011574|2458300.51070602|79963528|0500153b024972|01-a815-44007904-01-02|04790044|131778|537655879|581869302
			//	601a7c18-ee9a-4957-8d06-d255b6b42ca2|533395235|1|2458300.51017361|2458300.51070602|48450161|0500153b026d31|02-d81c-05252350-ff-02|50232505|0|-150802599|581869302
			//	b70815c1-74e9-4881-b9d8-00db2f9a7fe6|49606703|4|2458300.51018519|2458300.51070602|44489752|0500153b026d35|01-a815-78957202-01-02|02729578|640|1323567403|581869302

			cyng::buffer_t server, client;
			server = cyng::value_cast(frame.at(8), server);
			client = cyng::value_cast(frame.at(6), client);

			stmt->push(frame.at(0), 0)	//	ident
				.push(frame.at(1), 16)	//	trxID
				.push(frame.at(2), 0)	//	midx
				.push(frame.at(3), 0)	//	roTime
				.push(frame.at(4), 0)	//	actTime
				.push(frame.at(5), 0)	//	valTime
				.push(cyng::make_object(cyng::io::to_hex(client)), 0)	//	gateway/client
				//	clientId from_server_id
				.push(cyng::make_object(sml::from_server_id(server)), 0)	//	server
				//	serverId
				.push(cyng::make_object(sml::get_serial(server)), 0)	//	meter
				.push(frame.at(10), 0)	//	status
				.push(frame.at(11), 0)	//	source
				.push(frame.at(12), 0)	//	channel
				;
		}
		else
		{
			if (!boost::algorithm::equals(schema_, NODE_SUFFIX))
			{
				CYNG_LOG_WARNING(logger_, "use non-standard db schema " << schema_);
			}

			//
			//	examples:
			//
			//	pk|trxID|msgIdx|roTime|actTime|valTime|gateway|server|status|source|channel|target
			//	0bf4bfff-6a83-411d-ab60-1e98fc232fd2|51207|1|2458299.33280093|2458301.43333333|98573619|00:15:3b:02:29:7e|01-e61e-13090016-3c-07|0|-1324790809|-1649078317|water@solostec

			//	bind parameters
			stmt->push(frame.at(0), 0)	//	pk
				.push(frame.at(1), 16)	//	trxID
				.push(frame.at(2), 0)	//	msgIdx
				.push(frame.at(3), 0)	//	roTime
				.push(frame.at(4), 0)	//	actTime
				.push(frame.at(5), 0)	//	valTime
				//	client
				.push(frame.at(7), 0)	//	gateway
				//	server
				.push(frame.at(9), 0)	//	server
				.push(frame.at(10), 0)	//	status
				.push(frame.at(11), 0)	//	source
				.push(frame.at(12), 0)	//	channel
				.push(frame.at(13), 32)	//	target
				;
		}

		if (!stmt->execute())
		{
			CYNG_LOG_ERROR(logger_, sql << " failed with the following data set: " << cyng::io::to_str(frame));

		}
		stmt->clear();

	}

	void db_processor::insert_value_data(cyng::context& ctx)
	{
		//
		//	[0e5c944f-b2af-487a-b67d-f3fff1ccdfcd,5120143,100,81062B070000,ff,counter,i32, ,-29,-29]
		//	
		//	* [uuid] pk (meta)
		//	* [string] trx
		//	* idx
		//	* [buffer] OBIS code 
		//	* [uint8] unit code
		//	* [string] unit name
		//	* [string] CYNG data type name
		//	* scaler
		//	* raw value
		//	* formatted value

		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, "db.insert.data " << cyng::io::to_str(frame));


		cyng::sql::command cmd(mt_table_.find("TSMLData")->second, db_.get_dialect());
		cmd.insert();
		auto sql = cmd.to_str();
		CYNG_LOG_INFO(logger_, "db.insert.data " << sql);
		auto stmt = db_.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		BOOST_ASSERT(r.first == 8);	//	11 parameters to bind
		BOOST_ASSERT(r.second);

		cyng::buffer_t tmp = cyng::value_cast(frame.at(3), tmp);

		if (boost::algorithm::equals(schema_, "v4.0"))
		{
			sml::obis code(cyng::value_cast(frame.at(3), tmp));
			stmt->push(frame.at(0), 36)	//	pk
				.push(cyng::make_object(sml::to_string(code)), 24)	//	OBIS
				.push(frame.at(4), 0)	//	unitCode
				.push(frame.at(5), 0)	//	unitName
				.push(frame.at(6), 0)	//	SML data type
				.push(frame.at(7), 0)	//	scaler
				.push(frame.at(8), 0)	//	value
				.push(frame.at(9), 512)	//	result
				;
		}
		else
		{
			if (!boost::algorithm::equals(schema_, NODE_SUFFIX))
			{
				CYNG_LOG_WARNING(logger_, "use non-standard db schema " << schema_);
			}

			stmt->push(frame.at(0), 36)	//	pk
				.push(cyng::make_object(cyng::io::to_hex(tmp)), 24)	//	OBIS
				.push(frame.at(4), 0)	//	unitCode
				.push(frame.at(5), 0)	//	unitName
				.push(frame.at(6), 0)	//	SML data type
				.push(frame.at(7), 0)	//	scaler
				.push(frame.at(8), 0)	//	value
				.push(frame.at(9), 512)	//	result
				;

		}

		if (!stmt->execute())
		{
			CYNG_LOG_ERROR(logger_, sql << " failed with the following data set: " << cyng::io::to_str(frame));

		}
		stmt->clear();

	}

}
