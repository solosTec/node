/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "config_sensor_params.h"
#include "../cache.h"

#include <smf/sml/protocol/generator.h>
#include <smf/sml/obis_db.h>

#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>

namespace node
{
	namespace sml
	{
		config_sensor_params::config_sensor_params(cyng::logging::log_ptr logger
			, res_generator& sml_gen
			, cache& cfg)
		: logger_(logger)
			, sml_gen_(sml_gen)
			, cache_(cfg)
		{}

		void config_sensor_params::get_proc_params(std::string trx, cyng::buffer_t srv_id) const
		{
			//	81 81 C7 86 00 FF
			auto msg = sml_gen_.empty_get_proc_param_response(trx, srv_id, OBIS_ROOT_SENSOR_PARAMS);

			//
			//	81 81 C7 86 00 FF
			//	"srv_id" is the meter ID
			//
			cache_.read_table("_DeviceMBUS", [&](cyng::store::table const* tbl) {

				//
				//	list parameters of a specific device/meter
				//
				CYNG_LOG_TRACE(logger_, tbl->size() << " M-Bus devices");

				auto rec = tbl->lookup(cyng::table::key_generator(srv_id));
				if (!rec.empty())
				{
					//
					//	repeat server id (81 81 C7 82 04 FF)
					//	column "serverID"
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_SERVER_ID
						}, make_value(srv_id));

					//
					//	device class (always "---" or empty)
					//	81 81 C7 82 02 FF
					//	column: "class"
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_DEVICE_CLASS
					}, make_value(cyng::make_buffer({ })));
					
					//
					//	81 81 C7 82 03 FF : OBIS_DATA_MANUFACTURER - descr
					//	FLAG code
					//	column: "descr"
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_DATA_MANUFACTURER
						}, make_value(rec["descr"]));

					//
					//	81 00 60 05 00 00 : OBIS_CLASS_OP_LOG_STATUS_WORD
					//	column: "status"
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_CLASS_OP_LOG_STATUS_WORD
						}, make_value(cyng::make_buffer({ 0x00 })));

					//
					//	Bitmaske zur Definition von Bits, deren Änderung zu einem Eintrag im Betriebslogbuch zum Datenspiegel führt
					//	81 81 C7 86 01 FF (u16)
					//	column: "mask"
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_ROOT_SENSOR_BITMASK
						}, make_value(rec["mask"]));

					//
					//	Durchschnittliche Zeit zwischen zwei empfangenen Datensätzen in Millisekunden
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_AVERAGE_TIME_MS
						}, make_value(cyng::numeric_cast<std::uint64_t>(rec["interval"], 0u)));

					//
					//	current time (UTC)
					//	01 00 00 09 0B 00 : OBIS_CURRENT_UTC - last seen timestamp
					//	column: "lastSeen"
					//
					std::chrono::system_clock::time_point const now = cyng::value_cast(rec["lastSeen"], std::chrono::system_clock::now());

					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_CURRENT_UTC
						}, make_value(now));

					//
					//	81 81 C7 82 05 FF : OBIS_DATA_PUBLIC_KEY - public key
					//
					cyng::buffer_t public_key(16, 0u);
					public_key = cyng::value_cast(rec["pubKey"], public_key);

					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_DATA_PUBLIC_KEY
						}, make_value(public_key));

					//
					//	81 81 C7 86 03 FF : OBIS_DATA_AES_KEY - AES key (128 bits)
					//
					cyng::crypto::aes_128_key	aes_key;
					aes_key = cyng::value_cast(rec["aes"], aes_key);

					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_DATA_AES_KEY
						}, make_value(aes_key));

					//
					//	user and password
					//
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_DATA_USER_NAME
						}, make_value(rec["user"]));

					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_DATA_USER_PWD
						}, make_value(rec["pwd"]));

					//
					//
					//	81 81 C7 86 04 FF : OBIS_TIME_REFERENCE
					//	[u8] 0 == UTC, 1 == UTC + time zone, 2 == local time
					//
					std::uint8_t const time_ref = 0;
						
					append_get_proc_response(msg, {
						OBIS_ROOT_SENSOR_PARAMS,
						OBIS_TIME_REFERENCE
						}, make_value(time_ref));

					//
					//	append to message queue
					//
					sml_gen_.append(std::move(msg));

				}
			});

			//
			//	append to message queue
			//
			sml_gen_.append(std::move(msg));

		}

		void config_sensor_params::set_param(node::sml::obis code
			, cyng::buffer_t srv_id
			, cyng::param_t const& param)
		{

			cache_.write_table("_DeviceMBUS", [&](cyng::store::table* tbl) {

				switch (code.to_uint64()) {

				case CODE_SERVER_ID:
				case CODE_DEVICE_CLASS:
				case CODE_CLASS_OP_LOG_STATUS_WORD:
					break;
				case CODE_ROOT_SENSOR_BITMASK:
					set_bitmask(tbl, srv_id, param.second);
					break;

				case CODE_AVERAGE_TIME_MS:
					//case OBIS_CURRENT_UTC:
				case CODE_DATA_PUBLIC_KEY:
					tbl->modify(cyng::table::key_generator(srv_id), cyng::param_factory("pubKey", param.second), cache_.get_tag());
					break;
				case CODE_DATA_AES_KEY:
					tbl->modify(cyng::table::key_generator(srv_id), cyng::param_factory("aes", param.second), cache_.get_tag());
					break;
				case CODE_DATA_USER_NAME:
					tbl->modify(cyng::table::key_generator(srv_id), cyng::param_factory("user", param.second), cache_.get_tag());
					break;
				case CODE_DATA_USER_PWD:
					tbl->modify(cyng::table::key_generator(srv_id), cyng::param_factory("pwd", param.second), cache_.get_tag());
					break;
				case CODE_TIME_REFERENCE:
					break;

				default:
					break;
				}
			});
		}

		void config_sensor_params::set_bitmask(cyng::store::table* tbl
			, cyng::buffer_t srv_id
			, cyng::object obj)
		{
			auto const key = cyng::table::key_generator(srv_id);

			//
			//	We get a buffer with hex values and have to convert to an unsigned integer.
			//
			auto buffer = cyng::to_buffer(obj);
			std::reverse(buffer.begin(), buffer.end());
			switch (buffer.size()) {
			case 1:
				tbl->modify(key, cyng::param_factory("mask", cyng::to_numeric<std::uint8_t>(buffer)), cache_.get_tag());
				break;
			case 2:
				tbl->modify(key, cyng::param_factory("mask", cyng::to_numeric<std::uint16_t>(buffer)), cache_.get_tag());
				break;
			case 3:
			case 4:
				tbl->modify(key, cyng::param_factory("mask", cyng::to_numeric<std::uint32_t>(buffer)), cache_.get_tag());
				break;
			case 5:
			case 6:
			case 7:
			case 8:
				tbl->modify(key, cyng::param_factory("mask", cyng::to_numeric<std::uint64_t>(buffer)), cache_.get_tag());
				break;
			default:
				break;
			}
		}

	}	//	sml
}

