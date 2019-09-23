/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "cfg_push.h"
#include <smf/mbus/defs.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/protocol/message.h>

#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/vm/generator.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		cfg_push::cfg_push(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cyng::controller& vm
			, cyng::store::db& config_db)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, config_db_(config_db)
		{
			//
			//	these functions are removed - compare "sml.get.proc.parameter.request"
			//
			//vm.register_function("sml.set.proc.push.source", 7, std::bind(&cfg_push::sml_set_proc_push_source, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.push.interval", 7, std::bind(&cfg_push::sml_set_proc_push_interval, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.push.delay", 7, std::bind(&cfg_push::sml_set_proc_push_delay, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.push.name", 7, std::bind(&cfg_push::sml_set_proc_push_name, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.push.profile", 7, std::bind(&cfg_push::sml_set_proc_push_profile, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.push.service", 7, std::bind(&cfg_push::sml_set_proc_push_service, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.push.delete", 6, std::bind(&cfg_push::sml_set_proc_push_delete, this, std::placeholders::_1));
			//vm.register_function("sml.set.proc.push.reserve", 6, std::bind(&cfg_push::sml_set_proc_push_reserve, this, std::placeholders::_1));
		}

		//void cfg_push::sml_set_proc_push_source(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));
		//		auto const rec = tbl->lookup(key);
		//		if (!rec.empty()) {
		//			tbl->modify(rec.key(), cyng::param_t("source", frame.at(6)), ctx.tag());
		//		}
		//		else {

		//			//
		//			//	send error message
		//			//
		//			ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_NO_SERVER_ID.to_buffer()));
		//			CYNG_LOG_WARNING(logger_, "push.ops not found " << cyng::io::to_str(key));
		//		}
		//	}, cyng::store::write_access("push.ops"));
		//}

		//void cfg_push::sml_set_proc_push_interval(cyng::context& ctx)
		//{
		//	//	[a72a7054-f089-4df2-a592-3983935ba7df,180711174520423322-2,1,01A815743145050102,operator,operator,03c0]
		//	//
		//	//	* pk
		//	//	* transaction id
		//	//	* push index
		//	//	* server ID
		//	//	* username
		//	//	* password
		//	//	* push interval in seconds
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));
		//		auto const rec = tbl->lookup(key);
		//		if (!rec.empty()) {
		//			tbl->modify(key, cyng::param_t("interval", frame.at(6)), ctx.tag());
		//		}
		//		else {

		//			//
		//			//	send error message
		//			//
		//			ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_NO_SERVER_ID.to_buffer()));
		//			CYNG_LOG_WARNING(logger_, "push.ops not found " << cyng::io::to_str(key));
		//		}
		//	}, cyng::store::write_access("push.ops"));
		//}

		//void cfg_push::sml_set_proc_push_delay(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));
		//		auto const rec = tbl->lookup(key);
		//		if (!rec.empty()) {
		//			tbl->modify(rec.key(), cyng::param_t("delay", frame.at(6)), ctx.tag());
		//		}
		//		else {

		//			//
		//			//	send error message
		//			//
		//			ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_NO_SERVER_ID.to_buffer()));
		//			CYNG_LOG_WARNING(logger_, "push.ops not found " << cyng::io::to_str(key));
		//		}
		//	}, cyng::store::write_access("push.ops"));
		//}

		//void cfg_push::sml_set_proc_push_name(cyng::context& ctx)
		//{
		//	//	[b5b5fe15-d43f-4d2a-953e-1e2e1d153d89,180711180023633812-3,2,01A815743145050102,operator,operator,new-target-name]
		//	//
		//	//	* pk
		//	//	* transaction id
		//	//	* [uint8] push index
		//	//	* server ID
		//	//	* username
		//	//	* password
		//	//	* [string] push target name
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		CYNG_LOG_INFO(logger_, tbl->size() << " push.ops");
		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));

		//		auto const rec = tbl->lookup(key);
		//		if (!rec.empty()) {
		//			tbl->modify(rec.key(), cyng::param_t("target", frame.at(6)), ctx.tag());
		//		}
		//		else {

		//			//
		//			//	send error message
		//			//
		//			ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_NO_SERVER_ID.to_buffer()));
		//			CYNG_LOG_WARNING(logger_, "push.ops not found " << cyng::io::to_str(key));
		//		}
		//	}, cyng::store::write_access("push.ops"));
		//}

		//void cfg_push::sml_set_proc_push_profile(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		CYNG_LOG_INFO(logger_, tbl->size() << " push.ops");
		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));

		//		auto const rec = tbl->lookup(key);
		//		if (!rec.empty()) {
		//			tbl->modify(rec.key(), cyng::param_t("profile", frame.at(6)), ctx.tag());
		//		}
		//		else {

		//			//
		//			//	send error message
		//			//
		//			ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_NO_SERVER_ID.to_buffer()));
		//			CYNG_LOG_WARNING(logger_, "push.ops not found " << cyng::io::to_str(key));
		//		}
		//	}, cyng::store::write_access("push.ops"));
		//}

		//void cfg_push::sml_set_proc_push_service(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		CYNG_LOG_INFO(logger_, tbl->size() << " push.ops");
		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));

		//		auto const rec = tbl->lookup(key);
		//		if (!rec.empty()) {
		//			tbl->modify(rec.key(), cyng::param_t("source", frame.at(6)), ctx.tag());
		//		}
		//		else {

		//			//
		//			//	send error message
		//			//
		//			ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_NO_SERVER_ID.to_buffer()));
		//			CYNG_LOG_WARNING(logger_, "push.ops not found " << cyng::io::to_str(key));
		//		}
		//	}, cyng::store::write_access("push.ops"));
		//}

		//void cfg_push::sml_set_proc_push_delete(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		CYNG_LOG_INFO(logger_, tbl->size() << " push.ops");
		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));

		//		if (!tbl->erase(key, ctx.tag())) {
		//			//
		//			//	send error message
		//			//
		//			ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_CANNOT_WRITE.to_buffer()));
		//		}
		//	}, cyng::store::write_access("push.ops"));
		//}

		//void cfg_push::sml_set_proc_push_reserve(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		auto const key = cyng::table::key_generator(frame.at(3), frame.at(2));

		//		if (!tbl->exist(key)) {

		//			//
		//			//	create new record
		//			//
		//			create_push_op(tbl
		//				, key
		//				, cyng::table::data_generator(static_cast<std::uint32_t>(900u)	//	15 min interval
		//					, static_cast<std::uint32_t>(0u)	//	delay in seconds
		//					, "to-be-defined"	//	target name
		//					, OBIS_PUSH_SOURCE_PROFILE.to_buffer()	//	source
		//					, OBIS_PROFILE_15_MINUTE.to_buffer()	//	profile
		//					, static_cast<std::uint64_t>(0u))
		//				, ctx.tag());
		//		}

		//	}, cyng::store::write_access("push.ops"));
		//}

		bool cfg_push::create_push_op(cyng::store::table* tbl
			, cyng::table::key_type key
			, cyng::table::data_type data
			, boost::uuids::uuid tag)
		{
			return tbl->insert(key, data, 1, tag);
		}

	}	//	sml
}

