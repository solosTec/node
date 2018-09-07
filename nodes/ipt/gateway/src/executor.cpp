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

#include <cyng/async/task/task_builder.hpp>
#include <cyng/intrinsics/buffer.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		executor::executor(cyng::logging::log_ptr logger
			, cyng::async::mux& mux
			, status& status_word
			, cyng::store::db& config_db
			, node::ipt::bus::shared_type bus
			, boost::uuids::uuid tag
			, cyng::mac48 mac)
		: logger_(logger)
			, mux_(mux)
			, status_word_(status_word)
			, config_db_(config_db)
			, bus_(bus)
			, rgn_()
		{
			BOOST_ASSERT(bus_->vm_.tag() != tag);

			subscribe("devices");
			subscribe("push.ops");

#ifdef _DEBUG
			init_db(tag, mac);
#endif
		}

		void executor::ipt_access(bool success, std::string address)
		{
			//
			//	auto key
			//
			auto pk = config_db_.size("op.log");

			config_db_.insert("op.log"
				, cyng::table::key_generator(pk)
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, static_cast<std::uint32_t>(900u)	//	reg period - 15 min
					, std::chrono::system_clock::now()	//	val time
					, status_word_.operator std::uint64_t()	//	status
					, (success ? evt_ipt_connect() : evt_ipt_disconnect())	//	event: 1232076810/1232076814
					, OBIS_CODE_PEER_ADDRESS_WANGSM.to_buffer()	//	81 81 00 00 00 13
					, std::chrono::system_clock::now()	//	val time
					, cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x04, 0x01, 0x02 })
					, address
					, static_cast<std::uint8_t>(0u))		//	push_nr
				, 1	//	generation
				, bus_->vm_.tag());


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
			if (boost::algorithm::equals(table->meta().get_name(), "devices")) {

			}
			else if (boost::algorithm::equals(table->meta().get_name(), "push.ops")) {

				//
				//	start push target task
				//
				const auto r = cyng::async::start_task_delayed<ipt::push_ops>(mux_
					, std::chrono::seconds(10)
					, logger_
					, status_word_
					, config_db_
					, bus_
					, key
					, rgn_());
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
			if (boost::algorithm::equals(table->meta().get_name(), "devices")) {

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
			if (boost::algorithm::equals(table->meta().get_name(), "devices")) {

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

			if (boost::algorithm::equals(table->meta().get_name(), "devices")) {

			}
			else if (boost::algorithm::equals(table->meta().get_name(), "push.ops")) {

			}
			else {
				CYNG_LOG_ERROR(logger_, "db update unknown table " << table->meta().get_name());
			}
		}

		void executor::init_db(boost::uuids::uuid tag, cyng::mac48 mac)
		{
			//	insert demo device
			config_db_.insert("devices"
				, cyng::table::key_generator(sml::to_gateway_srv_id(mac))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, "---"
					, true	//	visible
					, true	//	active
					, "demo entry"
					, 0ull	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, 26000ul	//	interval
					, cyng::make_buffer({ 0x18, 0x01, 0x16, 0x05, 0xE6, 0x1E, 0x0D, 0x02, 0xBF, 0x0C, 0xFA, 0x35, 0x7D, 0x9E, 0x77, 0x03 })	//	pubKey
					, cyng::buffer_t{}	//	aes
					, "user"
					, "pwd")
				, 1	//	generation
				, tag);

			config_db_.insert("devices"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x05, 0x01, 0x02 }))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, "---"
					, true	//	visible
					, true	//	active
					, "01 A8 15 74 31 45 05 01 02"
					, 0ull	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, 26000ul	//	interval
					, cyng::make_buffer({ 0x18, 0x01, 0x16, 0x05, 0xE6, 0x1E, 0x0D, 0x02, 0xBF, 0x0C, 0xFA, 0x35, 0x7D, 0x9E, 0x77, 0x03 })	//	pubKey
					, cyng::buffer_t{}	//	aes
					, "user"
					, "pwd")
				, 1	//	generation
				, tag);

			config_db_.insert("devices"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xE6, 0x1E, 0x74, 0x31, 0x45, 0x04, 0x01, 0x02 }))
				, cyng::table::data_generator(std::chrono::system_clock::now()
					, "---"
					, true	//	visible
					, false	//	active
					, "01 E6 1E 74 31 45 05 01 02"
					, 0ull	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, 26000ul	//	interval
					, cyng::make_buffer({ 0x18, 0x01, 0x16, 0x05, 0xE6, 0x1E, 0x0D, 0x02, 0xBF, 0x0C, 0xFA, 0x35, 0x7D, 0x9E, 0x77, 0x03 })	//	pubKey
					, cyng::buffer_t{}	//	aes
					, "user"
					, "pwd")
				, 1	//	generation
				, tag);

			boost::uuids::random_generator rgen;

			//	insert demo push.ops
			config_db_.insert("push.ops"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x05, 0x01, 0x02 }), 2u)
				, cyng::table::data_generator(static_cast<std::uint32_t>(900u)	//	15 min
					, static_cast<std::uint32_t>(4u)	//	delay
					, "power@solostec"
					, static_cast<std::uint8_t>(1u)		//	source
					, static_cast<std::uint8_t>(1u)		//	profile
					, 0)
				, 1	//	generation
				, tag);

			config_db_.insert("push.ops"
				, cyng::table::key_generator(cyng::make_buffer({ 0x01, 0xA8, 0x15, 0x74, 0x31, 0x45, 0x05, 0x01, 0x02 }), 3u)
				, cyng::table::data_generator(static_cast<std::uint32_t>(1800u)	//	30 min
					, static_cast<std::uint32_t>(12u)	//	delay
					, "water@solostec"
					, static_cast<std::uint8_t>(1u)		//	source
					, static_cast<std::uint8_t>(3u)		//	profile
					, 0)
				, 1	//	generation
				, tag);


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

