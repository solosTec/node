/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/crc16.h>
#include <smf/sml/obis_db.h>
#include <NODE_project_info.h>
#include <cyng/vm/generator.h>
#include <cyng/chrono.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/io/swap.h>
#include <cyng/sys/info.h>
#include <sstream>
#include <iomanip>
#include <boost/core/ignore_unused.hpp>

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

		std::size_t generator::append(cyng::tuple_t&& msg)
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


		req_generator::req_generator(std::string const& name
			, std::string const& pwd)
		: generator()
			, trx_()
			, name_(name)
			, pwd_(pwd)
		{}

		std::string req_generator::public_open(cyng::mac48 client_id
			, cyng::buffer_t const& server_id)
		{
			BOOST_ASSERT_MSG(msg_.empty(), "pending SML data");
			auto const trx = *trx_;
			append(message(cyng::make_object(trx)	//	trx
				, group_no_++	//	group
				, 0 // abort code
				, BODY_OPEN_REQUEST

				//
				//	generate public open request
				//
				, open_request(cyng::make_object()	// codepage
					, cyng::make_object(client_id.to_buffer())	// clientId
					, cyng::make_object(gen_file_id())	//	req file id
					, cyng::make_object(server_id)	//	server id
					, cyng::make_object(name_)	//	name
					, cyng::make_object(pwd_)	//	pwd
					, cyng::make_object()	// sml-Version 
				)
			));

			return trx;
		}

		std::string req_generator::public_close()
		{
			++trx_;
			auto const trx = *trx_;
			append(message(cyng::make_object(trx)
				, 0	//	group is 0 for CLOSE REQUEST
				, 0 //	abort code
				, BODY_CLOSE_REQUEST

				//
				//	generate public open response
				//
				, close_response(cyng::make_object()
				)
			));

			return trx;
		}

		std::size_t req_generator::set_proc_parameter_restart(cyng::buffer_t const& server_id)
		{
			++trx_;
			return append(message(cyng::make_object(*trx_)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_SET_PROC_PARAMETER_REQUEST	//	0x600

				//
				//	generate reboot request
				//
				, set_proc_parameter_request(cyng::make_object(server_id)
					, name_
					, pwd_
					, OBIS_REBOOT
					, empty_tree(OBIS_REBOOT))
				)
			);
		}

		std::string req_generator::set_proc_parameter_ipt_host(cyng::buffer_t const& server_id
			, std::uint8_t idx
			, std::string const& address)
		{
			return set_proc_parameter(server_id
				, obis_path{ OBIS_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx) }
				, address);
		}

		std::string req_generator::set_proc_parameter_ipt_port_local(cyng::buffer_t const& server_id
			, std::uint8_t idx
			, std::uint16_t port)
		{
			return set_proc_parameter(server_id
				, obis_path{ OBIS_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx) }
				, port);
		}

		std::string req_generator::set_proc_parameter_ipt_port_remote(cyng::buffer_t const& server_id
			, std::uint8_t idx
			, std::uint16_t port)
		{
			return set_proc_parameter(server_id
				, obis_path{ OBIS_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx) }
				, port);
		}

		std::string req_generator::set_proc_parameter_ipt_user(cyng::buffer_t const& server_id
			, std::uint8_t idx
			, std::string const& user)
		{
			return set_proc_parameter(server_id
				, obis_path{ OBIS_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx) }
				, user);
		}

		std::string req_generator::set_proc_parameter_ipt_pwd(cyng::buffer_t const& server_id
			, std::uint8_t idx
			, std::string const& pwd)
		{
			return set_proc_parameter(server_id
				, obis_path{ OBIS_ROOT_IPT_PARAM, make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx), make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx) }
				, pwd);
		}

		std::string req_generator::set_proc_parameter_wmbus_protocol(cyng::buffer_t const& server_id
			, std::uint8_t val)
		{
			BOOST_ASSERT_MSG(val < 4, "wireless M-Bus protocol type out of range");

			return set_proc_parameter(server_id
				, obis_path{ OBIS_IF_wMBUS, OBIS_W_MBUS_PROTOCOL }
				, val);
		}

		std::string req_generator::get_proc_parameter(cyng::buffer_t const& server_id
			, obis code)
		{
			++trx_;
			auto const trx = *trx_;
			append(message(cyng::make_object(trx)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_REQUEST	//	0x500

				//
				//	generate get process parameter request
				//
				, get_proc_parameter_request(cyng::make_object(server_id)
					, name_
					, pwd_
					, code)
				)
			);
			return trx;
		}

		std::string req_generator::get_proc_parameter(cyng::buffer_t const& server_id
			, obis_path path)
		{
			++trx_;
			auto const trx = *trx_;
			append(message(cyng::make_object(trx)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_REQUEST	//	0x500

				//
				//	generate get process parameter request
				//
				, get_proc_parameter_request(cyng::make_object(server_id)
					, name_
					, pwd_
					, path)
				)
			);
			return trx;
		}

		std::string req_generator::get_list(cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id
			, obis code)
		{
			++trx_;
			auto const trx = *trx_;
			append(message(cyng::make_object(trx)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_GET_LIST_REQUEST	//	0x700

				//
				//	generate get list request
				//
				, get_list_request(cyng::make_object(client_id)
					, cyng::make_object(server_id)
					, name_
					, pwd_
					, code)
				)
			);
			return trx;
		}

		std::string req_generator::get_list_last_data_record(cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id)
		{
			return get_list(client_id, server_id, OBIS_CODE(99, 00, 00, 00, 00, 03));
		}

		std::string req_generator::get_profile_list(cyng::buffer_t const& server_id
			, std::chrono::system_clock::time_point begin_time
			, std::chrono::system_clock::time_point end_time
			, obis code)
		{
			++trx_;
			auto const trx = *trx_;
			append(message(cyng::make_object(trx)
				, group_no_++	//	group
				, 0 //	abort code
				, BODY_GET_PROFILE_LIST_REQUEST	//	0x400

				//
				//	generate get profile list request
				//
				, cyng::tuple_factory(server_id
					, name_
					, pwd_
					, cyng::make_object()	// withRawdata 
					, begin_time
					, end_time
					, cyng::tuple_factory(code.to_buffer())	//	path entry
					, cyng::make_object()	// object_List
					, cyng::make_object()	// dasDetails
				)
			));
			return trx;
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

			return append(message(trx	//	trx
				, group_no_++	//	group
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
			return append(message(trx
				, 0	//	group is 0 for CLOSE RESPONSE
				, 0 //	abort code
				, BODY_CLOSE_RESPONSE

				//
				//	generate public open response
				//
				, close_response(cyng::make_object()
				)
			));
		}

		std::size_t res_generator::empty(std::string trx, cyng::buffer_t server_id, obis root)
		{
			return append(empty_get_proc_param_response(trx, server_id, root));
		}

		cyng::tuple_t res_generator::empty_get_proc_param_response(std::string trx
			, cyng::buffer_t server_id
			, obis root)
		{
			return message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, root	//	root entry
					, child_list_tree(root, {})));
		}

		cyng::tuple_t res_generator::empty_get_profile_list(std::string trx
			, cyng::buffer_t client_id
			, obis path
			, std::chrono::system_clock::time_point act_time
			, std::uint32_t reg_period
			, std::chrono::system_clock::time_point val_time
			, std::uint64_t status)
		{
			return message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROFILE_LIST_RESPONSE	//	0x0401

				//
				//	generate get process parameter response
				//
				, get_profile_list_response(client_id
					, act_time
					, reg_period
					, path	//	path entry
					, val_time
					, status
					, cyng::tuple_t{}));
		}

		std::size_t res_generator::get_profile_list(std::string trx
			, cyng::buffer_t client_id
			, obis path
			, std::chrono::system_clock::time_point act_time
			, std::uint32_t reg_period
			, std::chrono::system_clock::time_point val_time
			, std::uint64_t status
			, cyng::tuple_t&& period_list)
		{
			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROFILE_LIST_RESPONSE	//	0x0401

				//
				//	generate get process parameter response
				//
				, get_profile_list_response(client_id
					, act_time
					, reg_period
					, path	//	path entry
					, val_time
					, status
					, std::move(period_list))));
		}

		std::size_t res_generator::get_proc_parameter_device_id(std::string trx
			, cyng::buffer_t server_id
			, std::string const& manufacturer
			, cyng::buffer_t const& server_id2
			, std::string const& model_code
			, std::uint32_t serial)
		{
			//
			//	reference implementation used a octet string as serial number - so we do
			//
			std::stringstream ss;
			ss
				<< std::setw(8)
				<< std::setfill('0')
				<< serial
				;
			const std::string serial_str = ss.str();
			
			const auto os_name = cyng::sys::get_os_name();

			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_ROOT_DEVICE_IDENT	//	path entry - 81 81 C7 82 01 FF

					//
					//	generate get process parameter response
					//	const static obis	DEFINE_OBIS_CODE(81, 81, C7, 82, 01, FF, CODE_ROOT_DEVICE_IDENT);	
					//	see: 7.3.2.9 Datenstruktur zur Abfrage der Geräte-Identifikation) 
					//
					, child_list_tree(OBIS_ROOT_DEVICE_IDENT, {

						//	device class (81 81 C7 82 53 FF == MUC-LAN/DSL)
						parameter_tree(OBIS_DEVICE_CLASS, make_value(OBIS_CODE(81, 81, C7, 82, 53, FF))),
						parameter_tree(OBIS_DATA_MANUFACTURER, make_value(manufacturer)),	// manufacturer
						parameter_tree(OBIS_SERVER_ID, make_value(server_id2)),	// server id

						//	firmware
						child_list_tree(OBIS_ROOT_FIRMWARE, {
							//	section 1
							child_list_tree(OBIS_CODE(81, 81, c7, 82, 07, 01), {
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 08, ff), make_value("CURRENT_VERSION")),
									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(NODE_VERSION)),
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 0e, ff), make_value(true)) 	//	activated
								}),
							//	section 2
							child_list_tree(OBIS_CODE(81, 81, c7, 82, 07, 02),	{
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 08, ff), make_value("KERNEL")),
									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(os_name)),
// 									parameter_tree(OBIS_CODE(81, 81, 00, 02, 00, 00), make_value(NODE_PLATFORM)),
									parameter_tree(OBIS_CODE(81, 81, c7, 82, 0e, ff), make_value(true)) 	//	activated
								})

							}),

						//	hardware
						child_list_tree(OBIS_CODE(81, 81, c7, 82, 09, ff), {
							//	Typenschlüssel
							parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 01), make_value(model_code)),
							parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 02), make_value(serial_str))
							//parameter_tree(OBIS_CODE(81, 81, c7, 82, 0a, 02), make_value(serial))	// works too
						})
					}))));

		}

		std::size_t res_generator::get_proc_mem_usage(std::string trx
			, cyng::buffer_t server_id
			, std::uint8_t mirror
			, std::uint8_t tmp)
		{
			//76                                                SML_Message(Sequence): 
			//  81063137303531313135343334303434363535382D32    transactionId: 170511154340446558-2
			//  6201                                            groupNo: 1
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		070080800010FF                            path_Entry: 00 80 80 00 10 FF 
			//	  73                                          parameterTree(Sequence): 
			//		070080800010FF                            parameterName: 00 80 80 00 10 FF 
			//		01                                        parameterValue: not set
			//		72                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			070080800011FF                        parameterName: 00 80 80 00 11 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  620E                                smlValue: 14 (std::uint8_t)
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			070080800012FF                        parameterName: 00 80 80 00 12 FF 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6200                                smlValue: 0
			//			01                                    child_List: not set
			//  631AA8                                          crc16: 6824
			//  00                                              endOfSmlMsg: 00 

			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_ROOT_MEMORY_USAGE		//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, child_list_tree(OBIS_ROOT_MEMORY_USAGE, {

						parameter_tree(OBIS_CODE(00, 80, 80, 00, 11, FF), make_value(mirror)),	//	mirror
						parameter_tree(OBIS_CODE(00, 80, 80, 00, 12, FF), make_value(tmp))	// tmp
				}))));

		}

		std::size_t res_generator::get_proc_device_time(std::string trx
			, cyng::buffer_t server_id
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

			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id
					, OBIS_ROOT_DEVICE_TIME		//	path entry

					//
					//	generate get process parameter response
					//	2 uint8 values 
					//
					, child_list_tree(OBIS_ROOT_DEVICE_TIME, {

						parameter_tree(OBIS_CURRENT_UTC, make_value(now)),	//	timestamp (01 00 00 09 0B 00 )
						parameter_tree(OBIS_CODE(00, 00, 60, 08, 00, FF), make_sec_index_value(now)),
						parameter_tree(OBIS_CODE(81, 00, 00, 09, 0B, 01), make_value(tz)),
						parameter_tree(OBIS_CODE(81, 00, 00, 09, 0B, 02), make_value(sync_active))
			}))));
		}

		std::size_t res_generator::get_proc_actuators(std::string trx
			, cyng::buffer_t server_id)
		{
			//00 80 80 11 00 FF                Not set
			//   00 80 80 11 01 FF             Not set
			//	  00 80 80 11 01 01          Not set
			//		 00 80 80 11 10 FF       ____ (0A E0 00 01 )
			//		 00 80 80 11 12 FF       11 (31 31 )
			//		 00 80 80 11 13 FF       True (54 72 75 65 )
			//		 00 80 80 11 14 FF       0 (30 )
			//		 00 80 80 11 15 FF       0 (30 )
			//		 00 80 80 11 16 FF       Einschalten Manuell 0-50% (45 69 6E 73 63 68 61 6C 74 65 6E 20 4D 61 6E 75 65 6C 6C 20 30 2D 35 30 25 )
			//	  00 80 80 11 01 02          Not set
			//		 00 80 80 11 10 FF       ____ (0A E0 00 02 )
			//		 00 80 80 11 12 FF       12 (31 32 )
			//		 00 80 80 11 13 FF       True (54 72 75 65 )
			//		 00 80 80 11 14 FF       0 (30 )
			//		 00 80 80 11 15 FF       0 (30 )
			//		 00 80 80 11 16 FF       Einschalten_Manuell 51-100% (45 69 6E 73 63 68 61 6C 74 65 6E 5F 4D 61 6E 75 65 6C 6C 20 35 31 2D 31 30 30 25 )

			//cyng::buffer_t actor_1{ 0x0A, (char)0xE0, 0x00, 0x01 }, actor_2{ 0x0A, (char)0xE0, 0x00, 0x02 };

			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_ACTUATORS	//	path entry: 00 80 80 11 00 FF
					, child_list_tree(OBIS_CODE(00, 80, 80, 11, 01, FF), {

						child_list_tree(OBIS_CODE(00, 80, 80, 11, 01, 01), {
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 10, FF), make_value(cyng::make_buffer({10, 0xE0, 0x00, 0x01}))),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 12, FF), make_value(11)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 13, FF), make_value(true)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 14, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 15, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 16, FF), make_value("Einschalten Manuell 0-50%"))
						}),
						child_list_tree(OBIS_CODE(00, 80, 80, 11, 01, 02),{
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 10, FF), make_value(cyng::make_buffer({10, 0xE0, 0x00, 0x02}))),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 12, FF), make_value(11)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 13, FF), make_value(true)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 14, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 15, FF), make_value(30)),
							parameter_tree(OBIS_CODE(00, 80, 80, 11, 16, FF), make_value("Einschalten_Manuell 51-100%"))
						})

					}))));

		}

		std::size_t res_generator::get_proc_parameter_ipt_state(std::string trx
			, cyng::buffer_t server_id
			, boost::asio::ip::tcp::endpoint remote_ep
			, boost::asio::ip::tcp::endpoint local_ep)
		{
			//76                                                SML_Message(Sequence): 
			//  81063137303531313136303831363537393537312D33    transactionId: 170511160816579571-3
			//  6202                                            groupNo: 2
			//  6200                                            abortOnError: 0
			//  72                                              messageBody(Choice): 
			//	630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
			//	73                                            SML_GetProcParameter_Res(Sequence): 
			//	  080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46 
			//	  71                                          parameterTreePath(SequenceOf): 
			//		0781490D0600FF                            path_Entry: 81 49 0D 06 00 FF 
			//	  73                                          parameterTree(Sequence): 
			//		0781490D0600FF                            parameterName: 81 49 0D 06 00 FF 
			//		01                                        parameterValue: not set
			//		73                                        child_List(SequenceOf): 
			//		  73                                      tree_Entry(Sequence): 
			//			07814917070000                        parameterName: 81 49 17 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  652A96A8C0                          smlValue: 714516672
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			0781491A070000                        parameterName: 81 49 1A 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  6368EF                              smlValue: 26863
			//			01                                    child_List: not set
			//		  73                                      tree_Entry(Sequence): 
			//			07814919070000                        parameterName: 81 49 19 07 00 00 
			//			72                                    parameterValue(Choice): 
			//			  6201                                parameterValue: 1 => smlValue (0x01)
			//			  631019                              smlValue: 4121
			//			01                                    child_List: not set
			//  639CB8                                          crc16: 40120
			//  00                                              endOfSmlMsg: 00 


			//
			//	network ordering 
			//
			std::uint32_t const ip_address = cyng::swap_num(remote_ep.address().to_v4().to_uint());

			std::uint16_t const target_port = remote_ep.port()
				, source_port = local_ep.port();

			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id	//	server id  
					, OBIS_ROOT_IPT_STATE	//	path entry - 81 49 0D 06 00 FF 
					, child_list_tree(OBIS_ROOT_IPT_STATE, {

						parameter_tree(OBIS_CODE(81, 49, 17, 07, 00, 00), make_value(ip_address)),
						parameter_tree(OBIS_CODE(81, 49, 1A, 07, 00, 00), make_value(target_port)),
						parameter_tree(OBIS_CODE(81, 49, 19, 07, 00, 00), make_value(source_port))
					}))));
		}


		std::size_t res_generator::get_profile_op_log(std::string trx
			, cyng::buffer_t client_id
			, std::chrono::system_clock::time_point act_time
			, std::uint32_t reg_period
			, std::chrono::system_clock::time_point val_time
			, std::uint64_t status
			, std::uint32_t evt
			, obis peer_address
			, std::chrono::system_clock::time_point tp
			, cyng::buffer_t server_id
			, std::string target
			, std::uint8_t push_nr
			, std::string details)
		{
			//	seconds since epoch
			std::uint64_t sec = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();

			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROFILE_LIST_RESPONSE	//	0x0401

				//
				//	generate get process parameter response
				//
				, get_profile_list_response(client_id
					, act_time
					, reg_period
					, OBIS_CLASS_OP_LOG	//	path entry: 81 81 C7 89 E1 FF
					, val_time
					, status
					, cyng::tuple_t{

						//	81 81 C7 89 E2 FF - OBIS_CLASS_EVENT
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  64800008                                value: 8388616
						period_entry(OBIS_CLASS_EVENT, 0xFF, 0, cyng::make_object(evt)),

						//	81 81 00 00 00 FF - PEER_ADDRESS
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  078146000002FF                          value: 81 46 00 00 02 FF 
						period_entry(OBIS_PEER_ADDRESS, 0xFF, 0, cyng::make_object(peer_address.to_buffer())),

						//  0781042B070000                          objName: 81 04 2B 07 00 00 
						//  62FE                                    unit: 254
						//  5200                                    scaler: 0
						//  5500000000                              value: 0
						period_entry(OBIS_CLASS_OP_LOG_FIELD_STRENGTH, 0xFE, 0, cyng::make_object(0u)),

						//  81 04 1A 07 00 00                       objName: CLASS_OP_LOG_CELL
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6200                                    value: 0
						period_entry(OBIS_CLASS_OP_LOG_CELL, 0xFF, 0, cyng::make_object(0u)),

						//  81 04 17 07 00 00                       objName:  CLASS_OP_LOG_AREA_CODE
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6200                                    value: 0
						period_entry(OBIS_CLASS_OP_LOG_AREA_CODE, 0xFF, 0, cyng::make_object(0u)),

						//  81 04 0D 06 00 00                       objName: CLASS_OP_LOG_PROVIDER
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6200                                    value: 0
						period_entry(OBIS_CLASS_OP_LOG_PROVIDER, 0xFF, 0, cyng::make_object(0u)),

						//  01 00 00 09 0B 00                       objName:  CURRENT_UTC
						//  6207                                    unit: 7
						//  5200                                    scaler: 0
						//  655B8E3F04                              value: 1536048900
						period_entry(OBIS_CURRENT_UTC, 0x07, 0, cyng::make_object(sec)),

						//  81 81 C7 82 04 FF                       objName:  SERVER_ID
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  0A01A815743145040102                    value: 01 A8 15 74 31 45 04 01 02 
						period_entry(OBIS_SERVER_ID, 0xFF, 0, cyng::make_object(server_id)),

						//  81 47 17 07 00 FF                       objName:  PUSH_TARGET
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  0F706F77657240736F6C6F73746563          value: power@solostec
						period_entry(OBIS_PUSH_TARGET, 0xFF, 0, cyng::make_object(target)),

						//  81 81 C7 8A 01 FF                       objName:  PUSH_OPERATIONS
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						//  6201                                    value: 1
						period_entry(OBIS_PUSH_OPERATIONS, 0xFF, 0, cyng::make_object(push_nr)),

						//	81 81 C7 81 23 FF
						//  62FF                                    unit: 255
						//  5200                                    scaler: 0
						period_entry(OBIS_DATA_PUSH_DETAILS, 0xFF, 0, cyng::make_object(details))

				})));

     
		}

		std::size_t res_generator::get_proc_w_mbus_status(std::string trx
			, cyng::buffer_t server_id
			, std::string const& manufacturer	// manufacturer of w-mbus adapter
			, cyng::buffer_t const&	id	//	adapter id (EN 13757-3/4)
			, std::string const& firmware	//	firmware version of adapter
			, std::string const& hardware)	//	hardware version of adapter);
		{
			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_ROOT_W_MBUS_STATUS	//	path entry - 81 06 0F 06 00 FF
					, child_list_tree(OBIS_ROOT_W_MBUS_STATUS, {

						parameter_tree(OBIS_W_MBUS_ADAPTER_MANUFACTURER, make_value(manufacturer)),
						parameter_tree(OBIS_W_MBUS_ADAPTER_ID, make_value(id)),
						parameter_tree(OBIS_W_MBUS_FIRMWARE, make_value(firmware)),
						parameter_tree(OBIS_W_MBUS_HARDWARE, make_value(hardware))

			}))));
		}

		std::size_t res_generator::get_proc_w_mbus_if(std::string trx
			, cyng::buffer_t server_id
			, cyng::object protocol	// radio protocol
			, cyng::object s_mode	// duration in seconds
			, cyng::object t_mode	// duration in seconds
			, cyng::object reboot	//	duration in seconds
			, cyng::object power	//	transmision power (transmission_power)
			, cyng::object install_mode)
		{
			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_PROC_PARAMETER_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_proc_parameter_response(server_id
					, OBIS_IF_wMBUS	//	path entry - 81 06 19 07 00 FF
					, child_list_tree(OBIS_IF_wMBUS, {

						parameter_tree(OBIS_W_MBUS_PROTOCOL, make_value(protocol)),
						parameter_tree(OBIS_W_MBUS_MODE_S, make_value(s_mode)),
						parameter_tree(OBIS_W_MBUS_MODE_T, make_value(t_mode)),
						parameter_tree(OBIS_W_MBUS_REBOOT, make_value(reboot)),
						parameter_tree(OBIS_W_MBUS_POWER, make_value(power)),
						parameter_tree(OBIS_W_MBUS_INSTALL_MODE, make_value(install_mode))

			}))));
		}

		std::size_t res_generator::get_list(cyng::object trx
			, cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id
			, obis list_name
			, cyng::tuple_t act_sensor_time
			, cyng::tuple_t act_gateway_time
			, cyng::tuple_t val_list)
		{
			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_GET_LIST_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_list_response(client_id
					, server_id
					, list_name
					, act_sensor_time
					, act_gateway_time
					, val_list)));
		}

		std::size_t res_generator::attention_msg(cyng::object trx
			, cyng::buffer_t const& server_id
			, cyng::buffer_t const& attention_nr
			, std::string attention_msg
			, cyng::tuple_t attention_details)
		{
			return append(message(trx	//	trx
				, ++group_no_	//	group
				, 0 //	abort code
				, BODY_ATTENTION_RESPONSE

				//
				//	generate get process parameter response
				//
				, get_attention_response(server_id
					, attention_nr
					, attention_msg
					, attention_details)));
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
			return value_ + "-" + std::to_string(num_);
		}

	}
}
