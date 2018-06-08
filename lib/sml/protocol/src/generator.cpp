/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <NODE_project_info.h>
#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/crc16.h>
#include <smf/sml/obis_db.h>
#include <cyng/vm/generator.h>
#include <cyng/chrono.h>
#include <sstream>
#include <iomanip>

namespace node
{
	namespace sml
	{
		generator::generator()
			: msg_()
			, group_no_(0)
		{}

		void generator::reset()
		{
			msg_.clear();
			group_no_ = 0;
		}

		cyng::buffer_t generator::boxing() const
		{
			return node::sml::boxing(msg_);
		}

		std::string generator::gen_file_id()
		{
			const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			const std::tm utc = cyng::chrono::convert_utc(now);
			std::stringstream ss;
			ss
				<< std::setfill('0')
				<< cyng::chrono::year(utc)
				<< std::setw(2)
				<< cyng::chrono::month(utc)
				<< std::setw(2)
				<< cyng::chrono::day(utc)
				<< std::setw(2)
				<< cyng::chrono::hour(utc)
				<< std::setw(2)
				<< cyng::chrono::minute(utc)
				<< std::setw(2)
				<< cyng::chrono::second(utc)
				;
			return ss.str();
		}

		std::size_t generator::append_msg(cyng::tuple_t&& msg)
		{
#ifdef SMF_IO_DEBUG
			CYNG_LOG_TRACE(logger_, "msg: " << cyng::io::to_str(msg));
#endif

			//
			//	linearize and set CRC16
			//
			cyng::buffer_t b = linearize(msg);
			sml_set_crc16(b);

			//
			//	append to current SML message
			//
			msg_.push_back(b);
			return msg_.size();
		}


		req_generator::req_generator()
			: generator()
			, trx_()
		{}

		std::size_t req_generator::public_open(cyng::mac48 client_id
			, cyng::buffer_t const& server_id
			, std::string const& name
			, std::string const& pwd)
		{
			BOOST_ASSERT_MSG(msg_.empty(), "pending SML data");
			return append_msg(message(cyng::make_object(*trx_)	//	trx
				, 0	//	group
				, 0 // abort code
				, BODY_OPEN_REQUEST

				//
				//	generate public open request
				//
				, open_request(cyng::make_object()	// codepage
					, cyng::make_object(client_id.to_buffer())	// clientId
					, cyng::make_object(gen_file_id())	//	req file id
					, cyng::make_object(server_id)	//	server id
					, cyng::make_object(name)	//	name
					, cyng::make_object(pwd)	//	pwd
					, cyng::make_object()	// sml-Version 
				)
			));
		}

		std::size_t req_generator::public_close()
		{
			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, 0	//	group
				, 0 //	abort code
				, BODY_CLOSE_REQUEST

				//
				//	generate public open response
				//
				, close_response(cyng::make_object()
				)
			));
		}

		std::size_t req_generator::set_proc_parameter_restart(cyng::buffer_t const& server_id
			, std::string const& username
			, std::string const& password)
		{
			++trx_;
			return append_msg(message(cyng::make_object(*trx_)
				, 0	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST

				//
				//	generate public open response
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, username
					, password
					, OBIS_CODE_REBOOT
					, empty_tree(OBIS_CODE_REBOOT))
				)
			);
		}

		res_generator::res_generator()
			: generator()
		{}

		std::size_t res_generator::public_open(cyng::object trx
			, cyng::object client_id
			, cyng::object req
			, cyng::object server_id)	//	server id
		{
			BOOST_ASSERT_MSG(msg_.empty(), "pending SML data");

			return append_msg(message(trx	//	trx
				, 0	//	group
				, 0 // abort code
				, BODY_OPEN_RESPONSE

				//
				//	generate public open response
				//
				, open_response(cyng::make_object()	// codepage
					, client_id
					, req	//	req file id
					, server_id	//	server id
					, cyng::make_object()	//	 ref time
					, cyng::make_object()
				)
			));
		}

		std::size_t res_generator::public_close(cyng::object trx)
		{
			return append_msg(message(trx
				, 0	//	group
				, 0 //	abort code
				, BODY_CLOSE_RESPONSE

				//
				//	generate public open response
				//
				, close_response(cyng::make_object()
				)
			));
		}

		std::size_t res_generator::get_proc_parameter_status_word(cyng::object trx
			, cyng::object server_id
			, std::uint64_t status)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_CODE(81, 00, 60, 05, 00, 00)	//	path entry
					, parameter_tree(OBIS_CLASS_OP_LOG_STATUS_WORD, make_value(status)))));
		}

		std::size_t res_generator::get_proc_parameter_device_id(cyng::object trx
			, cyng::object server_id
			, std::string const& manufacturer
			, cyng::buffer_t const& server_id2
			, std::string const& model_code
			, std::string const& serial)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_DEVICE_IDENT	//	path entry - 81 81 C7 82 01 FF

					//
					//	generate get process parameter response
					//	const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 01, FF, CODE_ROOT_DEVICE_IDENT);	
					//	see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation) 
					//
					, child_list_tree(OBIS_CODE_ROOT_DEVICE_IDENT, {

						//	device class (81 81 C7 82 53 FF == MUC-LAN/DSL)
						parameter_tree(OBIS_CODE(81, 81, c7, 82, 02, ff), make_value(OBIS_CODE(81, 81, C7, 82, 53, FF))),
						parameter_tree(OBIS_CODE(81, 81, c7, 82, 03, ff), make_value(manufacturer)),	// manufacturer
						parameter_tree(OBIS_CODE(81, 81, c7, 82, 04, ff), make_value(server_id2)),	// server id

						//	firmware
						child_list_tree(OBIS_CODE(81, 81, c7, 82, 06, ff), {
							//	section 1
							child_list_tree(OBIS_CODE(81, 81, c7, 82, 07, 01), {
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 08, ff), make_value("CURRENT_VERSION")),
									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(NODE_VERSION)),
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 0e, ff), make_value(true)) 	//	activated
								}),
							//	section 2
							child_list_tree(OBIS_CODE(81, 81, c7, 82, 07, 02),	{
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 08, ff), make_value("KERNEL")),
									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(NODE_PLATFORM)),
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 0e, ff), make_value(true)) 	//	activated
								})

							}),

						//	hardware
						child_list_tree(OBIS_CODE(81, 81, c7, 82, 09, ff), {
							//	Typenschlüssel
							parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 01), make_value(model_code)),
							parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 02), make_value(serial))
						})
					}))));

		}

		trx::trx()
			: rng_()
			, gen_(rng_())
			, value_()
			, num_(1)
		{
			shuffle(7);
		}

		trx::trx(trx const& other)
			: rng_()
			, gen_(rng_())
			, value_(other.value_)
			, num_(other.num_)
		{}


		void trx::shuffle(std::size_t length)
		{
			std::uniform_int_distribution<int> dis('0', '9');
			value_.resize(length);
			std::generate(value_.begin(), value_.end(), [&]() {
				return dis(gen_);
			});
		}

		trx& trx::operator++()
		{
			++num_;
			return *this;
		}

		trx trx::operator++(int)
		{
			trx tmp(*this);
			++num_;
			return tmp;
		}

		std::string trx::operator*() const
		{
			std::stringstream ss;
			ss
				<< value_
				<< '-'
				<< num_
				;
			return ss.str();

		}

	}
}
