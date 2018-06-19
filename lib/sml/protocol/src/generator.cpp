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
#include <cyng/value_cast.hpp>
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

		std::size_t res_generator::empty(cyng::object trx, cyng::object server_id, obis path)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, path	//	path entry
					, parameter_tree(path, empty_tree(path)))));
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
				//const static obis	DEFINE_OBIS_CODE(81, 00, 60, 05, 00, 00, CLASS_OP_LOG_STATUS_WORD);
				//
				, get_proc_parameter_response(server_id
					, OBIS_CLASS_OP_LOG_STATUS_WORD	//	path entry
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

		std::size_t res_generator::get_proc_mem_usage(cyng::object trx
			, cyng::object server_id
			, std::uint8_t mirror
			, std::uint8_t tmp)
		{
			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_MEMORY_USAGE		//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, child_list_tree(OBIS_CODE_ROOT_MEMORY_USAGE, {

						parameter_tree(OBIS_CODE(00, 80, 80, 00, 11, FF), make_value(mirror)),	//	mirror
						parameter_tree(OBIS_CODE(00, 80, 80, 00, 12, FF), make_value(tmp))	// tmp
			}))));

		}

		std::size_t res_generator::get_proc_device_time(cyng::object trx
			, cyng::object server_id
			, std::chrono::system_clock::time_point now
			, std::int32_t tz
			, bool sync_active)
		{

			//76                                                SML_Message(Sequence): 
			//  81063137303531323136353635313836333431352D33    transactionId: 170512165651863415-3
			//  6202                                            groupNo: 2
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		078181C78810FF                            path_Entry: 81 81 C7 88 10 FF 
			//	  73                                          parameterTree(Sequence): 
			//		078181C78810FF                            parameterName: 81 81 C7 88 10 FF 
			//		01                                        parameterValue: not set
			//		74                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			07010000090B00                        parameterName: 01 00 00 09 0B 00 
			//			72                                    parameterValue(Choice): 
			//			  6204                                parameterValue: 4 => smlTime (0x04)
			//			  72                                  smlTime(Choice): 
			//				6202                              smlTime: 2 => timestamp (0x02)
			//				655915CD3C                        timestamp: 1494601020
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070000600800FF                        parameterName: 00 00 60 08 00 FF 
			//			72                                    parameterValue(Choice): 
			//			  6204                                parameterValue: 4 => smlTime (0x04)
			//			  72                                  smlTime(Choice): 
			//				6201                              smlTime: 1 => secIndex (0x01)
			//				6505E5765F                        secIndex: 98924127
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07810000090B01                        parameterName: 81 00 00 09 0B 01 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  550000003C                          smlValue: 60
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07810000090B02                        parameterName: 81 00 00 09 0B 02 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  4200                                smlValue: False
			//			01                                    child_List: not set
			//  633752                                          crc16: 14162
			//  00                                              endOfSmlMsg: 00 

			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_DEVICE_TIME		//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, child_list_tree(OBIS_CODE_ROOT_DEVICE_TIME, {

						parameter_tree(OBIS_CURRENT_UTC, make_value(now)),	//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(OBIS_CODE(00, 00, 60, 08, 00, FF), make_sec_index_value(now)),
						parameter_tree(OBIS_CODE(81, 00, 00, 09, 0B, 01), make_value(tz)),
						parameter_tree(OBIS_CODE(81, 00, 00, 09, 0B, 02), make_value(sync_active))
			}))));
		}

		std::size_t res_generator::get_proc_active_devices(cyng::object trx
			, cyng::object server_id
			, const cyng::store::table* tbl)
		{
			//	1.list: 81, 81, 11, 06, 01, FF
			//	2.list: 81, 81, 11, 06, 02, FF
			//	3.list: 81, 81, 11, 06, 03, FF
			cyng::tuple_t list;	// list of lists
			cyng::tuple_t tpl;	// current list
			
			const cyng::buffer_t tmp;
			tbl->loop([&](cyng::table::record const& rec)->bool {

				if (cyng::value_cast(rec["active"], false)) {

					tpl.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x81, 0x11, 0x06, 0x01, tpl.size() + 1), {
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 04, FF), make_value(cyng::value_cast(rec["serverID"], tmp))),
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 02, FF), make_value(cyng::value_cast<std::string>(rec["class"], ""))),
						//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(OBIS_CURRENT_UTC, make_value(cyng::value_cast(rec["lastSeen"], std::chrono::system_clock::now())))
					})));

					//
					//	start new list
					//
					if (tpl.size() == 0xFE)
					{
						list.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x81, 0x11, 0x06, list.size() + 1, 0xFF), tpl)));
						tpl.clear();
					}
					BOOST_ASSERT_MSG(list.size() < 0xFA, "active device list to large");
				}

				//continue
				return true;
			});

			//
			//	append last pending list
			//
			list.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x81, 0x11, 0x06, list.size() + 1, 0xFF), tpl)));


			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_ACTIVE_DEVICES		//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_ACTIVE_DEVICES, list)
					//, child_list_tree(OBIS_CODE_ROOT_ACTIVE_DEVICES, {
					//	child_list_tree(OBIS_CODE_LIST_1_ACTIVE_DEVICES, tpl)
					//}
			)));
		}

		std::size_t res_generator::get_proc_visible_devices(cyng::object trx
			, cyng::object server_id
			, const cyng::store::table* tbl)
		{
			//	1. list: 81, 81, 10, 06, 01, FF
			//	2. list: 81, 81, 10, 06, 02, FF
			//	3. list: 81, 81, 10, 06, 03, FF
			cyng::tuple_t list;	// list of lists
			cyng::tuple_t tpl;	// current list

			const cyng::buffer_t tmp;
			tbl->loop([&](cyng::table::record const& rec)->bool {

				if (cyng::value_cast(rec["visible"], false)) {

					tpl.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x81, 0x10, 0x06, 0x01, tpl.size() + 1), {
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 04, FF), make_value(cyng::value_cast(rec["serverID"], tmp))),
						parameter_tree(OBIS_CODE(81, 81, C7, 82, 02, FF), make_value(cyng::value_cast<std::string>(rec["class"], ""))),
						//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(OBIS_CURRENT_UTC, make_value(cyng::value_cast(rec["lastSeen"], std::chrono::system_clock::now())))
						})));

					//
					//	start new list
					//
					if (tpl.size() == 0xFE)
					{
						list.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x81, 0x10, 0x06, list.size() + 1, 0xFF), tpl)));
						tpl.clear();
					}
					BOOST_ASSERT_MSG(list.size() < 0xFA, "visible device list to large");
				}

				//continue
				return true;
			});

			//
			//	append last pending list
			//
			list.push_back(cyng::make_object(child_list_tree(obis(0x81, 0x81, 0x10, 0x06, list.size() + 1, 0xFF), tpl)));


			return append_msg(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_CODE_ROOT_VISIBLE_DEVICES		//	path entry

					//
					//	generate get process parameter response
					//
					, child_list_tree(OBIS_CODE_ROOT_VISIBLE_DEVICES, list)
				)));
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
