/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "executor.h"
#include "tasks/push_ops.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/event.h>
#include <smf/sml/obis_db.h>
#include <smf/shared/db_cfg.h>
#include <smf/sml/status.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/intrinsics/buffer.h>
#include <cyng/crypto/aes.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		executor::executor(cyng::logging::log_ptr logger
			, cyng::async::mux& mux
			, cyng::store::db& config_db
			, cyng::controller& vm)
		: logger_(logger)
			, mux_(mux)
			, config_db_(config_db)
			, vm_(vm)
		{
			subscribe("mbus-devices");
			subscribe("push.ops");

#ifdef _DEBUG
			init_db(vm_.tag());
#endif
		}

		void executor::ipt_access(bool success, std::string address)
		{
			//
			//	authorization successful / failed
			//	update status word atomically
			//
			config_db_.access([&](cyng::store::table* tbl_cfg, cyng::store::table* tbl_log) {

				auto word = cyng::value_cast<std::uint64_t>(get_config(tbl_cfg, OBIS_CLASS_OP_LOG_STATUS_WORD.to_str()), 0u);
				status status(word);

				status.set_authorized(success);
				update_config(tbl_cfg, OBIS_CLASS_OP_LOG_STATUS_WORD.to_str(), word, vm_.tag());

				CYNG_LOG_DEBUG(logger_, "status word: " << word);

				//
				//	auto key
				//
				auto pk = tbl_log->size();

				tbl_log->insert(cyng::table::key_generator(pk)
					, cyng::table::data_generator(std::chrono::system_clock::now()
						, static_cast<std::uint32_t>(900u)	//	reg period - 15 min
						, std::chrono::system_clock::now()	//	val time
						, word	//	status
						, (success ? evt_ipt_connect() : evt_ipt_disconnect())	//	event: 1232076810/1232076814
						, OBIS_CODE_PEER_ADDRESS_WANGSM.to_buffer()	//	81 81 00 00 00 13
						, std::chrono::system_clock::now()	//	val time
						, cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x04, 0x01, 0x02 })
						, address
						, static_cast<std::uint8_t>(0u))		//	push_nr
					, 1	//	generation
					, vm_.tag());

			}	, cyng::store::write_access("_Config")
				, cyng::store::write_access("op.log"));
		}

		void executor::start_wireless_lmn(bool available)
		{
			//
			//	atomic status update
			//
			config_db_.access([&](cyng::store::table* tbl) {

				auto word = cyng::value_cast<std::uint64_t>(get_config(tbl, node::sml::OBIS_CLASS_OP_LOG_STATUS_WORD.to_str()), 0u);
				node::sml::status status(word);

				//
				//	Wireless M-Bus interface is available
				//
				status.set_mbus_if_available(available);
				update_config(tbl, node::sml::OBIS_CLASS_OP_LOG_STATUS_WORD.to_str(), word, vm_.tag());
				CYNG_LOG_DEBUG(logger_, "status word: " << word);

			}, cyng::store::write_access("_Config"));
		}

		void executor::subscribe(std::string const& table)
		{
			config_db_.access([&](cyng::store::table* tbl)->void {
				//
				//	One set of callbacks for multiple tables.
				//	Store connections array for clean disconnect
				//
				cyng::store::add_subscription(subscriptions_
					, table
					, tbl->get_listener(std::bind(&executor::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
						, std::bind(&executor::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
						, std::bind(&executor::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
						, std::bind(&executor::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)));
			}, cyng::store::write_access(table));

		}

		void executor::sig_ins(cyng::store::table const* table
			, cyng::table::key_type const& key
			, cyng::table::data_type const& body
			, std::uint64_t gen
			, boost::uuids::uuid source)
		{
			if (boost::algorithm::equals(table->meta().get_name(), "mbus-devices")) {

				//
				//	ToDo: handle M-Bus devices
				//
				;
			}
			else if (boost::algorithm::equals(table->meta().get_name(), "push.ops")) {

				//
				//	start push target task
				//
				const auto r = cyng::async::start_task_delayed<ipt::push_ops>(mux_
					, std::chrono::seconds(10)
					, logger_
					//, status_word_
					, config_db_
					, vm_
					, key
					, vm_.tag());
				if (r.second) {
					CYNG_LOG_INFO(logger_, "push.ops task #" << r.first << " started");

				}
				else {
					CYNG_LOG_FATAL(logger_, "start push.ops task failed");
				}
			}
			else {
				CYNG_LOG_ERROR(logger_, "db insert unknown table " << table->meta().get_name());
			}
		}

		void executor::sig_del(cyng::store::table const* table
			, cyng::table::key_type const& key
			, boost::uuids::uuid source)
		{
			if (boost::algorithm::equals(table->meta().get_name(), "mbus-devices")) {

				//
				//	ToDo: handle M-Bus devices
				//
				;
			}
			else if (boost::algorithm::equals(table->meta().get_name(), "push.ops")) {

			}
			else {
				CYNG_LOG_ERROR(logger_, "db delete unknown table " << table->meta().get_name());
			}
		}

		void executor::sig_clr(cyng::store::table const* table
			, boost::uuids::uuid source)
		{
			if (boost::algorithm::equals(table->meta().get_name(), "mbus-devices")) {

				//
				//	ToDo: handle M-Bus devices
				//
				;
			}
			else if (boost::algorithm::equals(table->meta().get_name(), "push.ops")) {

			}
			else {
				CYNG_LOG_ERROR(logger_, "db clear unknown table " << table->meta().get_name());
			}
		}

		void executor::sig_mod(cyng::store::table const* table
			, cyng::table::key_type const& key
			, cyng::attr_t const& attr
			, std::uint64_t gen
			, boost::uuids::uuid source)
		{
			
			CYNG_LOG_TRACE(logger_, "db update " 
				<< table->meta().get_name()
				<< " "
				<< table->meta().to_param(attr).first);

			if (boost::algorithm::equals(table->meta().get_name(), "mbus-devices")) {

				//
				//	ToDo: handle M-Bus devices
				//
				;
			}
			else if (boost::algorithm::equals(table->meta().get_name(), "push.ops")) {

			}
			else {
				CYNG_LOG_ERROR(logger_, "db update unknown table " << table->meta().get_name());
			}
		}

		void executor::init_db(boost::uuids::uuid tag)
		{
#ifdef _DEBUG

			cyng::buffer_t server_id;
			server_id = cyng::value_cast(get_config(config_db_, OBIS_CODE_SERVER_ID.to_str()), server_id);

			cyng::crypto::aes_128_key aes_key;
			cyng::crypto::aes::randomize(aes_key);

			//	insert demo device
			config_db_.insert("mbus-devices"
				, cyng::table::key_generator(server_id)
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, "---"
					//, true	//	visible
					, false	//	active
					, "demo entry"
					, 0ull	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, 26000ul	//	interval
					, cyng::make_buffer({ 0x18, 0x01, 0x16, 0x05, 0xE6, 0x1E, 0x0D, 0x02, 0xBF, 0x0C, 0xFA, 0x35, 0x7D, 0x9E, 0x77, 0x03 })	//	pubKey
					, aes_key	//	random AES key
					, "user"
					, "pwd")
				, 1	//	generation
				, tag);

			//
			//	01-a815-74314504-01-02 - Electricity (ED300L)
			//	AES Key : 23 A8 4B 07 EB CB AF 94 88 95 DF 0E 91 33 52 0D
			//	IV: a8 15 74 31 45 04 01 02 ....
			//
			aes_key.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };
			config_db_.insert("mbus-devices"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x05, 0x01, 0x02 }))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, "---"
					//, true	//	visible
					, true	//	active
					, "01 A8 15 74 31 45 05 01 02"
					, 0ull	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, 26000ul	//	interval
					, cyng::make_buffer({ 0x18, 0x01, 0x16, 0x05, 0xE6, 0x1E, 0x0D, 0x02, 0xBF, 0x0C, 0xFA, 0x35, 0x7D, 0x9E, 0x77, 0x03 })	//	pubKey
					, aes_key	//	random AES key
					, "user"
					, "pwd")
				, 1	//	generation
				, tag);

			cyng::crypto::aes::randomize(aes_key);
			config_db_.insert("mbus-devices"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xE6, 0x1E, 0x73, 0x31, 0x45, 0x04, 0x01, 0x02 }))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, "---"
					//, true	//	visible
					, false	//	active
					, "01 E6 1E 74 31 45 05 01 02"
					, 0ull	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, 26000ul	//	interval
					, cyng::make_buffer({ 0x18, 0x01, 0x16, 0x05, 0xE6, 0x1E, 0x0D, 0x02, 0xBF, 0x0C, 0xFA, 0x35, 0x7D, 0x9E, 0x77, 0x03 })	//	pubKey
					, aes_key	//	random AES key
					, "user"
					, "pwd")
				, 1	//	generation
				, tag);

			//
			//	GWF: Gas - 01-e61e-29436587-bf-03 over 01-2423-96072000-63-0e - Bus / System component
			//	Hydrometer Funkmodule (Gas-, Wasserfunkmodule) AES Key: 51 72 89 10 E6 6D 83 F8 51 72 89 10 E6 6D 83 F8
			//	IV: e6 1e 29 43 65 87 bf 03 ....
			//
			aes_key.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };
			config_db_.insert("mbus-devices"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xE6, 0x1E, 0x29, 0x43, 0x65, 0x87, 0xbf, 0x03 }))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, "---"
					//, true	//	visible
					, true	//	active
					, "GWF gas meter 87654329"
					, 0ull	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, 26000ul	//	interval
					, cyng::make_buffer({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 })	//	pubKey
					, aes_key	//	random AES key
					, "user"
					, "pwd")
				, 1	//	generation
				, tag);


			auto const b = config_db_.insert("iec62056-21-devices"
				, cyng::table::key_generator("1EMH0005513895")
				, cyng::table::data_generator("1EMH0005513895"
					, "demo device"
					, 9600u
					, "87654321"
					, "00000000")
				, 1	//	generation
				, tag);

			boost::uuids::random_generator uidgen;

			//	insert demo push.ops
			config_db_.insert("push.ops"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x05, 0x01, 0x02 }), static_cast<std::uint8_t>(1u))
				, cyng::table::data_generator(static_cast<std::uint32_t>(900u)	//	15 min interval
					, static_cast<std::uint32_t>(4u)	//	delay in seconds
					, "pushStore"	//	target name
					, OBIS_PUSH_SOURCE_PROFILE.to_buffer()	//	source
					, OBIS_PROFILE_15_MINUTE.to_buffer()	//	profile
					, static_cast<std::uint64_t>(cyng::async::NO_TASK))
				, 1	//	generation
				, tag);

			CYNG_LOG_INFO(logger_, "table push.ops has " << config_db_.size("push.ops") << " entries");

#endif
			//
			//	startup
			//
			config_db_.insert("op.log"
				, cyng::table::key_generator(0UL)
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, static_cast<std::uint32_t>(900u)	//	reg period - 15 min
					, std::chrono::system_clock::now()	//	val time
					, static_cast<std::uint64_t>(0x070202)	//	status
					//, static_cast<std::uint64_t>(status_word_.operator std::uint32_t())	//	status
					, static_cast<std::uint32_t>(0x800008)	//	event
					, cyng::make_buffer({ 0x81, 0x46, 0x00, 0x00, 0x02, 0xFF })
					, std::chrono::system_clock::now()	//	val time
					, cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x04, 0x01, 0x02 })
					, "power@solostec"
					, static_cast<std::uint8_t>(1u))		//	push_nr
				, 1	//	generation
				, tag);


		}


	}	//	sml
}

