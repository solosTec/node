/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "attention.h"
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

		attention::attention(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cyng::controller& vm
			, cyng::store::db& config_db)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, config_db_(config_db)
		{
			vm.register_function("sml.attention.init", 4, std::bind(&attention::attention_init, this, std::placeholders::_1));
			vm.register_function("sml.attention.send", 1, std::bind(&attention::attention_send, this, std::placeholders::_1));
			vm.register_function("sml.attention.set", 2, std::bind(&attention::attention_set, this, std::placeholders::_1));
		}

		void attention::attention_init(cyng::context& ctx)
		{
			//	[190418223120978108-2,01EC4D010000103C02,8181C7C7FD00,Set Proc Parameter Request]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	insert new entry
			//
			config_db_.insert("trx"
				, cyng::table::key_generator(frame.at(0))
				, cyng::table::data_generator(frame.at(2), frame.at(1), frame.at(3))
				, 1
				, ctx.tag());
		}

		void attention::attention_send(cyng::context& ctx)
		{
			//	[190418223120978108-2]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			config_db_.access([&](cyng::store::table* tbl) {

				auto const rec = tbl->lookup(cyng::table::key_generator(frame.at(0)));
				if (!rec.empty()) {

					cyng::buffer_t buffer;
					obis const code(cyng::value_cast(rec["code"], buffer));

					auto const msg = cyng::value_cast<std::string>(rec["msg"], "");

					//
					//	send entry
					//
					this->send_attention_code(rec["trx"], rec["serverID"], code, msg);

					//
					//	remove entry
					//
					tbl->erase(rec.key(), ctx.tag());
				}
			}, cyng::store::write_access("trx"));
		}

		void attention::attention_set(cyng::context& ctx)
		{
			//	[190418223120978108-2,8181C7C7FD00]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			config_db_.access([&](cyng::store::table* tbl) {

				tbl->modify(cyng::table::key_generator(frame.at(0)), cyng::param_t("code", frame.at(1)), ctx.tag());

			}, cyng::store::write_access("trx"));

		}

		void attention::send_attention_code_ok(cyng::object trx, cyng::buffer_t server_id)
		{
			sml_gen_.attention_msg(trx	// trx
				, server_id	//	server ID
				, OBIS_ATTENTION_OK.to_buffer()
				, "OK"
				, cyng::tuple_t());
		}

		void attention::send_attention_code_ok(cyng::object trx, cyng::object server_id)
		{
			cyng::buffer_t id;
			id = cyng::value_cast(server_id, id);
			send_attention_code_ok(trx, id);
		}

		void attention::send_attention_code(cyng::object trx, cyng::buffer_t server_id, obis code, std::string msg)
		{
			sml_gen_.attention_msg(trx	// trx
				, server_id	//	server ID
				, code.to_buffer()
				, msg
				, cyng::tuple_t());
		}

		void attention::send_attention_code(cyng::object trx, cyng::object server_id, obis code, std::string msg)
		{
			cyng::buffer_t id;
			id = cyng::value_cast(server_id, id);
			send_attention_code(trx, id, code, msg);
		}


	}	//	sml
}

