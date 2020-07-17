/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "dispatcher.h"
#include <smf/shared/db_schemes.h>
#include <smf/cluster/generator.h>

#include <cyng/table/meta.hpp>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{

	dispatcher::dispatcher(cyng::async::mux& mux, cyng::logging::log_ptr logger, std::set<std::size_t> tasks)
		: mux_(mux)
		, logger_(logger)
		, tasks_(tasks)
	{}

	void dispatcher::register_this(cyng::controller& vm)
	{
		vm.register_function("db.res.insert", 4, std::bind(&dispatcher::db_res_insert, this, std::placeholders::_1));
		vm.register_function("db.res.remove", 2, std::bind(&dispatcher::db_res_remove, this, std::placeholders::_1));
		vm.register_function("db.res.modify.by.attr", 3, std::bind(&dispatcher::db_res_modify_by_attr, this, std::placeholders::_1));
		vm.register_function("db.res.modify.by.param", 3, std::bind(&dispatcher::db_res_modify_by_param, this, std::placeholders::_1));
		vm.register_function("db.req.insert", 4, std::bind(&dispatcher::db_req_insert, this, std::placeholders::_1));
		vm.register_function("db.req.remove", 3, std::bind(&dispatcher::db_req_remove, this, std::placeholders::_1));
		vm.register_function("db.req.modify.by.param", 5, std::bind(&dispatcher::db_req_modify_by_param, this, std::placeholders::_1));
	}

	void dispatcher::db_res_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[TDevice,[32f1a373-83c9-4f24-8fac-b13103bc7466],[00000006,2018-03-11 18:35:33.61302590,true,,,comment #10,1010000,secret,device-10000],0]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	
		CYNG_LOG_TRACE(logger_, "db.res.insert - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t			//	[3] generation
		>(frame);

		//
		//	assemble a record
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

		distribute(std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::get<2>(tpl)		//	[2] record
			, std::get<3>(tpl)		//	[3] generation
			, ctx.tag());	//	[4] origin	
	}

	void dispatcher::db_req_insert(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[*Session,[2ce46726-6bca-44b6-84ed-0efccb67774f],[00000000-0000-0000-0000-000000000000,2018-03-12 17:56:27.10338240,f51f2ae7,data-store,eaec7649-80d5-4b71-8450-3ee2c7ef4917,94aa40f9-70e8-4c13-987e-3ed542ecf7ab,null,session],1]
		//
		//	* table name
		//	* record key
		//	* record data
		//	* generation
		//	
		CYNG_LOG_TRACE(logger_, "db.req.insert - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::table::data_type,	//	[2] record
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid		//	[4] source
		>(frame);

		//
		//	assemble a record
		//
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());
		std::reverse(std::get<2>(tpl).begin(), std::get<2>(tpl).end());

		distribute(std::get<0>(tpl)		//	[0] table name
			, std::get<1>(tpl)		//	[1] table key
			, std::get<2>(tpl)		//	[2] record
			, std::get<3>(tpl)		//	[3] generation
			, std::get<4>(tpl));	//	[4] origin	

	}

	void dispatcher::db_req_remove(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[*Session,[e72bc048-cb37-4a86-b156-d07f22608476]]
		//
		//	* table name
		//	* record key
		//	* source
		//	
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			boost::uuids::uuid		//	[2] source
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		CYNG_LOG_TRACE(logger_, ctx.get_name()
			<< " - "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl)));

	}

	void dispatcher::db_res_remove(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		//
		//	[TDevice,[def8e1ef-4a67-49ff-84a9-fda31509dd8e]]
		//
		//	* table name
		//	* record key
		//	
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type	//	[1] table key
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		CYNG_LOG_TRACE(logger_, ctx.get_name()
			<< " - "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl)));
	}

	void dispatcher::db_res_modify_by_attr(cyng::context& ctx)
	{
		//	[TDevice,[0950f361-7800-4d80-b3bc-c6689f159439],(1:secret)]
		//
		//	* table name
		//	* record key
		//	* attr [column,value]
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::attr_t			//	[2] attribute
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		CYNG_LOG_TRACE(logger_, ctx.get_name()
			<< " - "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl))
			<< " => "
			<< std::get<2>(tpl).first
			<< " = "
			<< cyng::io::to_str(std::get<2>(tpl).second));
	}

	void dispatcher::db_res_modify_by_param(cyng::context& ctx)
	{
		//	
		//
		//	* table name
		//	* record key
		//	* param [column,value]
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::param_t			//	[2] parameter
		>(frame);

		CYNG_LOG_TRACE(logger_, ctx.get_name()
			<< " - "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl))
			<< " => "
			<< std::get<2>(tpl).first
			<< " = "
			<< cyng::io::to_str(std::get<2>(tpl).second));
	}

	void dispatcher::db_req_modify_by_param(cyng::context& ctx)
	{
		//	
		//	[*Session,[35d1d76d-56c3-4df7-b1ff-b7ad374d2e8f],("rx":33344),327,35d1d76d-56c3-4df7-b1ff-b7ad374d2e8f]
		//	[*Cluster,[1e4527b3-6479-4b2c-854b-e4793f40d864],("ping":00:00:0.003736),4,1e4527b3-6479-4b2c-854b-e4793f40d864]
		//
		//	* table name
		//	* record key
		//	* param [column,value]
		//	* generation
		//	* source
		//	
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto tpl = cyng::tuple_cast<
			std::string,			//	[0] table name
			cyng::table::key_type,	//	[1] table key
			cyng::param_t,			//	[2] parameter
			std::uint64_t,			//	[3] generation
			boost::uuids::uuid		//	[4] source
		>(frame);

		//
		//	reordering table key
		//	
		std::reverse(std::get<1>(tpl).begin(), std::get<1>(tpl).end());

		CYNG_LOG_TRACE(logger_, ctx.get_name()
			<< " - "
			<< std::get<0>(tpl)		// table name
			<< " - "
			<< cyng::io::to_str(std::get<1>(tpl))
			<< " => "
			<< std::get<2>(tpl).first
			<< " = "
			<< cyng::io::to_str(std::get<2>(tpl).second));
	}

	void dispatcher::distribute(std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::table::data_type data	//	[2] record
		, std::uint64_t	gen
		, boost::uuids::uuid origin)
	{
		//	[2019-03-06 14:15:00.69611500,2e98ff09-e324-4251-a16e-fa73d6f996f3,LSMTest5,open push channel,water@solostec]
		//	
		//	* timestamp
		//	* session tag
		//	* device name
		//	* event
		//	* description
		//
		CYNG_LOG_TRACE(logger_, "distribute - "
			<< table		
			<< " * "
			<< tasks_.size()
			<< " - "
			<< cyng::io::to_str(key)
			<< " => "
			<< cyng::io::to_str(data));

		BOOST_ASSERT(data.size() == 5);
		for (auto const tsk : tasks_) {

			auto tpl = cyng::tuple_cast<
				std::chrono::system_clock::time_point,			//	[0] time stamp
				boost::uuids::uuid,		//	[1] session tag
				std::string,	// [2] device name
				std::string,	// [3] event
				std::string		// [4] description
			>(data);

			mux_.post(tsk
				, 0	//	slot
				, cyng::tuple_factory(table
					, cyng::value_cast<std::size_t>(key.at(0), 0u)
					, std::get<0>(tpl)
					, std::get<1>(tpl)
					, std::get<2>(tpl)
					, std::get<3>(tpl)
					, std::get<4>(tpl)
				));
		}
	}

	void dispatcher::res_subscribe(std::string const& table		//	[0] table name
		, cyng::table::key_type key		//	[1] table key
		, cyng::table::data_type data	//	[2] record
		, std::uint64_t	gen				//	[3] generation
		, boost::uuids::uuid origin		//	[4] origin session id
		, std::size_t tsk)
	{
		//
		//	insert new record
		//
		CYNG_LOG_INFO(logger_, "subscription response "
			<< table		// table name
			<< " - "
			<< cyng::io::to_str(key)
			<< " => "
			<< cyng::io::to_str(data));

		//
		//	distribute data
		//
		distribute(table
			, key
			, data
			, gen
			, origin);
	}

}
