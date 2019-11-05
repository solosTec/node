/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "set_proc_parameter.h"
#include "../cache.h"
#include "../segw.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_io.h>

#include <cyng/io/serializer.h>

namespace node
{
	namespace sml
	{
		set_proc_parameter::set_proc_parameter(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg
			, cyng::buffer_t const& id)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
			, config_ipt_(logger, sml_gen, cfg, id)
		{}

		void set_proc_parameter::generate_response(obis_path const& path
			, std::string trx
			, cyng::buffer_t srv_id
			, std::string user
			, std::string pwd
			, cyng::param_t	param)
		{

			//	[190919215155673187-2,81490D0700FF 81490D070002 81491A070002,0500FFB04B94F8,operator,operator,("81491A070002":68f0)]
			if (!path.empty()) {

				auto pos = path.begin();
				auto end = path.end();

				switch (pos->to_uint64()) {
				case 0x81490D0700FF:	//	IP-T
					if (pos != end)	_81490d0700ff(++pos, end, trx, srv_id, user, pwd, param);
					break;
				default:
					CYNG_LOG_ERROR(logger_, "sml.set.proc.parameter.request - unknown OBIS code "
						<< to_string(*pos)
						<< " / "
						<< cyng::io::to_hex(pos->to_buffer()));
					break;
				}
			}
			else {
				CYNG_LOG_ERROR(logger_, "set_proc_parameter(empty path)");
			}
		}

		void set_proc_parameter::_81490d0700ff(obis_path::const_iterator pos
			, obis_path::const_iterator end
			, std::string trx
			, cyng::buffer_t srv_id
			, std::string user
			, std::string pwd
			, cyng::param_t	param)
		{
			switch (pos->to_uint64()) {
			case 0x81490D070001:
			case 0x81490D070002:
				if (pos != end)	config_ipt_.set_param(*++pos, param);
				break;
			case 0x814827320601:	//	WAIT_TO_RECONNECT
			case 0x814831320201:	//	TCP_CONNECT_RETRIES
			case 0x0080800003FF:	//	use SSL
				config_ipt_.set_param(*pos, param);
				break;
			default:
				CYNG_LOG_ERROR(logger_, "sml.set.proc.parameter.request <_81490d0700ff> - unknown OBIS code "
					<< to_string(*pos)
					<< " / "
					<< cyng::io::to_hex(pos->to_buffer()));
				break;
			}
		}

		//vm.register_function("sml.set.proc.if1107.param", 7, std::bind(&kernel::sml_set_proc_if1107_param, this, std::placeholders::_1));
		//vm.register_function("sml.set.proc.if1107.device", 7, std::bind(&kernel::sml_set_proc_if1107_device, this, std::placeholders::_1));

		//vm.register_function("sml.set.proc.mbus.param", 7, std::bind(&kernel::sml_set_proc_mbus_param, this, std::placeholders::_1));
		//vm.register_function("sml.set.proc.sensor", 7, std::bind(&kernel::sml_set_proc_sensor, this, std::placeholders::_1));

		//void kernel::sml_set_proc_activate(cyng::context& ctx)
		//{
		//	//	[3ad68eea-ac45-4349-96c6-d77f0a45a67d,190215220337893589-2,1,0500FFB04B94F8,operator,operator,01E61E733145040102]
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	auto const tpl = cyng::tuple_cast<
		//		boost::uuids::uuid,	//	[0] pk
		//		std::string,		//	[1] trx
		//		std::uint8_t,		//	[2] record index (1..2)
		//		cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
		//		std::string,		//	[4] user name
		//		std::string,		//	[5] password
		//		cyng::buffer_t		//	[6] server/meter ID 
		//	>(frame);

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		auto const rec = tbl->lookup(cyng::table::key_generator(std::get<6>(tpl)));
		//		if (!rec.empty()) {

		//			//
		//			//	set sensor/actor active
		//			//
		//			CYNG_LOG_INFO(logger_, "activate sensor: " << cyng::io::to_hex(std::get<6>(tpl)));
		//			tbl->modify(rec.key(), cyng::param_factory("active", true), ctx.tag());

		//			//
		//			//	set attention code ATTENTION_OK - this is default
		//			//
		//			//ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_OK.to_buffer()));
		//		}
		//		else {

		//			//
		//			//	send attention code OBIS_ATTENTION_NO_SERVER_ID
		//			//
		//			sml_gen_.attention_msg(frame.at(1)	// trx
		//				, std::get<3>(tpl)	//	server ID
		//				, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
		//				, ctx.get_name()
		//				, cyng::tuple_t());

		//			CYNG_LOG_WARNING(logger_, ctx.get_name() << " - wrong server ID: "
		//				<< cyng::io::to_hex(std::get<3>(tpl)))
		//				;
		//		}
		//	}, cyng::store::write_access("mbus-devices"));

		//}

		//void kernel::sml_set_proc_deactivate(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	auto const tpl = cyng::tuple_cast<
		//		boost::uuids::uuid,	//	[0] pk
		//		std::string,		//	[1] trx
		//		std::uint8_t,		//	[2] record index (1..2)
		//		cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
		//		std::string,		//	[4] user name
		//		std::string,		//	[5] password
		//		cyng::buffer_t		//	[6] server/meter ID 
		//	>(frame);

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		auto const rec = tbl->lookup(cyng::table::key_generator(std::get<6>(tpl)));
		//		if (!rec.empty()) {

		//			//
		//			//	set sensor/actor active
		//			//
		//			CYNG_LOG_INFO(logger_, "deactivate sensor: " << cyng::io::to_hex(std::get<6>(tpl)));
		//			tbl->modify(rec.key(), cyng::param_factory("active", false), ctx.tag());

		//			//
		//			//	send attention code ATTENTION_OK - this is default
		//			//
		//			//ctx.queue(cyng::generate_invoke("sml.attention.set", frame.at(1), OBIS_ATTENTION_OK.to_buffer()));

		//		}
		//		else {

		//			//
		//			//	send attention code OBIS_ATTENTION_NO_SERVER_ID
		//			//
		//			sml_gen_.attention_msg(frame.at(1)	// trx
		//				, std::get<3>(tpl)	//	server ID
		//				, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
		//				, ctx.get_name()
		//				, cyng::tuple_t());

		//			CYNG_LOG_WARNING(logger_, ctx.get_name() << " - wrong server ID: "
		//				<< cyng::io::to_hex(std::get<3>(tpl)))
		//				;
		//		}
		//	}, cyng::store::write_access("mbus-devices"));
		//}

		//void kernel::sml_set_proc_delete(cyng::context& ctx)
		//{
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	auto const tpl = cyng::tuple_cast<
		//		boost::uuids::uuid,	//	[0] pk
		//		std::string,		//	[1] trx
		//		std::uint8_t,		//	[2] record index (1..2)
		//		cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
		//		std::string,		//	[4] user name
		//		std::string,		//	[5] password
		//		cyng::buffer_t		//	[6] server/meter ID 
		//	>(frame);

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		//
		//		//	remove sensor/actor
		//		//
		//		CYNG_LOG_WARNING(logger_, "delete sensor: " << cyng::io::to_hex(std::get<6>(tpl)));
		//		auto const key = cyng::table::key_generator(std::get<6>(tpl));
		//		if (tbl->erase(key, ctx.tag())) {

		//			//
		//			//	send attention code ATTENTION_OK
		//			//
		//			sml_gen_.attention_msg(frame.at(1)	// trx
		//				, std::get<3>(tpl)	//	server ID
		//				, OBIS_ATTENTION_OK.to_buffer()
		//				, "OK"
		//				, cyng::tuple_t());
		//		}
		//		else {

		//			//
		//			//	send attention code OBIS_ATTENTION_NO_SERVER_ID
		//			//
		//			sml_gen_.attention_msg(frame.at(1)	// trx
		//				, std::get<3>(tpl)	//	server ID
		//				, OBIS_ATTENTION_NO_SERVER_ID.to_buffer()
		//				, ctx.get_name()
		//				, cyng::tuple_t());

		//			CYNG_LOG_WARNING(logger_, ctx.get_name() << " - wrong server ID: "
		//				<< cyng::io::to_hex(std::get<3>(tpl)))
		//				;
		//		}
		//	}, cyng::store::write_access("mbus-devices"));
		//}

		//void kernel::sml_set_proc_if1107_param(cyng::context& ctx)
		//{
		//	//	[eb672857-669e-41cd-8e19-0b0bb187dec4,19022815085510333-3,0500FFB04B94F8,operator,operator,8181C79305FF,138a]
		//	cyng::vector_t const frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	config_db_.access([&](cyng::store::table* tbl) {
		//		cyng::buffer_t tmp;
		//		obis code(cyng::value_cast(frame.at(5), tmp));
		//		tbl->modify(cyng::table::key_generator(code.to_str()), cyng::param_t("value", frame.at(6)), ctx.tag());
		//	}, cyng::store::write_access("_Config"));

		//	//
		//	//	send attention code ATTENTION_OK
		//	//
		//	cyng::buffer_t tmp;
		//	sml_gen_.attention_msg(frame.at(1)	// trx
		//		, cyng::value_cast(frame.at(2), tmp)	//	server ID
		//		, OBIS_ATTENTION_OK.to_buffer()
		//		, "OK"
		//		, cyng::tuple_t());

		//}

		//void kernel::sml_set_proc_if1107_device(cyng::context& ctx)
		//{
		//	//	 [ce4769ae-da55-4464-9a47-e65d14f1afb5,1903010920587636-2,2,0500FFB04B94F8,operator,operator,%(("8181C7930AFF":31454D4830303035353133383936),("8181C7930BFF":4b00),("8181C7930CFF":31454D4830303035353133383936),("8181C7930DFF":31323133),("8181C7930EFF":313233313233))]
		//	//	 [5e9e8ac8-be28-4284-ba3f-cae2d8151ace,190301092252659302-2,1,0500FFB04B94F8,operator,operator,%(("8181C7930EFF":3030303030303031))]
		//	cyng::vector_t const frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	auto const tpl = cyng::tuple_cast<
		//		boost::uuids::uuid,	//	[0] pk
		//		std::string,		//	[1] trx
		//		std::uint8_t,		//	[2] record index (1..2)
		//		cyng::buffer_t,		//	[3] gateway id e.g 0500FFB04B94F8
		//		std::string,		//	[4] user name
		//		std::string,		//	[5] password
		//		cyng::param_map_t	//	[6] new/mofified parameters
		//	>(frame);

		//	config_db_.access([&](cyng::store::table* tbl) {
		//		if (std::get<2>(tpl) > tbl->size()) {

		//			//
		//			//	new device
		//			//
		//			cyng::buffer_t tmp;
		//			auto const reader = cyng::make_reader(std::get<6>(tpl));
		//			auto const meter = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_METER_ID.to_str()), tmp);
		//			auto const address = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_ADDRESS.to_str()), tmp);
		//			auto const baudrate = cyng::numeric_cast(reader.get(OBIS_CODE_IF_1107_BAUDRATE.to_str()), 9600u);
		//			auto const p1 = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_P1.to_str()), tmp);
		//			auto const w5 = cyng::value_cast(reader.get(OBIS_CODE_IF_1107_W5.to_str()), tmp);

		//			auto const b = tbl->insert(cyng::table::key_generator(meter)
		//				, cyng::table::data_generator(address
		//					, cyng::to_str(std::chrono::system_clock::now())
		//					, baudrate
		//					, std::string(p1.begin(), p1.end())
		//					, std::string(w5.begin(), w5.end()))
		//				, 1	//	generation
		//				, ctx.tag());

		//		}
		//		else {

		//			//
		//			//	modify device
		//			//	locate device with the specified index
		//			//	
		//			//	ToDo: use an u8 data type as index
		//			//	Currently we cannot modify the meterId since this is the table index
		//			//
		//			auto const rec = tbl->nth_record(std::get<2>(tpl) - 1);
		//			for (auto const& p : std::get<6>(tpl)) {
		//				if (p.first == OBIS_CODE_IF_1107_ADDRESS.to_str()) {
		//					tbl->modify(rec.key(), cyng::param_t("address", p.second), ctx.tag());
		//				}
		//				else if (p.first == OBIS_CODE_IF_1107_BAUDRATE.to_str()) {
		//					tbl->modify(rec.key(), cyng::param_t("baudrate", p.second), ctx.tag());
		//				}
		//				else if (p.first == OBIS_CODE_IF_1107_P1.to_str()) {
		//					tbl->modify(rec.key(), cyng::param_t("p1", p.second), ctx.tag());
		//				}
		//				else if (p.first == OBIS_CODE_IF_1107_W5.to_str()) {
		//					tbl->modify(rec.key(), cyng::param_t("w5", p.second), ctx.tag());
		//				}
		//			}
		//		}
		//	}, cyng::store::write_access("iec62056-21-devices"));

		//	//
		//	//	send attention code ATTENTION_OK
		//	//
		//	cyng::buffer_t tmp;
		//	sml_gen_.attention_msg(frame.at(1)	// trx
		//		, cyng::value_cast(frame.at(2), tmp)	//	server ID
		//		, OBIS_ATTENTION_OK.to_buffer()
		//		, "OK"
		//		, cyng::tuple_t());

		//}

		//void kernel::sml_set_proc_mbus_param(cyng::context& ctx)
		//{
		//	//	[828497da-d0d5-4e55-8b2d-8bd4d8c4c6e0,0343476-2,0500FFB04B94F8,OBIS,operator,operator,0001517f]
		//	cyng::vector_t const frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	//
		//	//	get OBIS code
		//	//
		//	cyng::buffer_t tmp;
		//	obis const code(cyng::value_cast(frame.at(3), tmp));

		//	//
		//	//	update config db
		//	//
		//	config_db_.modify("_Config", cyng::table::key_generator(code.to_str()), cyng::param_t("value", frame.at(6)), ctx.tag());

		//	//
		//	//	send attention code ATTENTION_OK
		//	//
		//	tmp.clear();
		//	sml_gen_.attention_msg(frame.at(1)	// trx
		//		, cyng::value_cast(frame.at(2), tmp)	//	server ID
		//		, OBIS_ATTENTION_OK.to_buffer()
		//		, "OK"
		//		, cyng::tuple_t());

		//}

		//void kernel::sml_set_proc_sensor(cyng::context& ctx)
		//{
		//	//
		//	//	examples:
		//	//
		//	//	[eea7d18d-f7c3-4cf5-af66-4d1d9011beb3,5522760-4,01E61E29436587BF03,operator,operator,8181C78205FF,00000000000000000000000000000000]
		//	//	[eea7d18d-f7c3-4cf5-af66-4d1d9011beb3,5522760-2,01E61E29436587BF03,operator,operator,8181613C01FF,75736572]
		//	//
		//	//
		//	cyng::vector_t const frame = ctx.get_frame();
		//	CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	//
		//	//	get OBIS code
		//	//
		//	cyng::buffer_t tmp;
		//	obis const code(cyng::value_cast(frame.at(5), tmp));

		//	config_db_.access([&](cyng::store::table* tbl) {

		//		auto const key = cyng::table::key_generator(frame.at(2));

		//		if (OBIS_DATA_USER_NAME == code) {
		//			tbl->modify(key, cyng::param_t("user", frame.at(6)), ctx.tag());
		//		}
		//		else if (OBIS_DATA_USER_PWD == code) {
		//			tbl->modify(key, cyng::param_t("pwd", frame.at(6)), ctx.tag());
		//		}
		//		else if (OBIS_DATA_PUBLIC_KEY == code) {
		//			tbl->modify(key, cyng::param_t("pubKey", frame.at(6)), ctx.tag());
		//		}
		//		else if (OBIS_DATA_AES_KEY == code) {
		//			tbl->modify(key, cyng::param_t("aes", frame.at(6)), ctx.tag());
		//		}
		//		else if (OBIS_CODE_ROOT_SENSOR_BITMASK == code) {
		//			tbl->modify(key, cyng::param_t("mask", frame.at(6)), ctx.tag());
		//		}
		//	}, cyng::store::write_access("mbus-devices"));

		//	//
		//	//	send attention code ATTENTION_OK
		//	//
		//	tmp.clear();
		//	sml_gen_.attention_msg(frame.at(1)	// trx
		//		, cyng::value_cast(frame.at(2), tmp)	//	server ID
		//		, OBIS_ATTENTION_OK.to_buffer()
		//		, "OK"
		//		, cyng::tuple_t());

		//}

	}	//	sml
}

