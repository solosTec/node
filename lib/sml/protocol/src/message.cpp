/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/crc16.h>
#include <smf/sml/obis_db.h>

#include <cyng/factory.h>
#include <cyng/buffer_cast.h>

namespace node
{
	namespace sml
	{
		//
		//	prototypes of functions only visible in this compilation unit
		//
		bool merge_child_list(obis const* obis_begin
			, obis const* obis_end
			, cyng::tuple_t::iterator cl_begin
			, cyng::tuple_t::iterator cl_end
			, cyng::tuple_t&& val);
		bool merge_child_list(obis const* obis_begin
			, obis const* obis_end
			, cyng::tuple_t&
			, cyng::tuple_t&& val);


		cyng::tuple_t message(cyng::object trx
			, std::uint8_t group_no
			, std::uint8_t abort_code
			, sml_messages_enum type
			, cyng::tuple_t body)
		{
			return cyng::tuple_factory( trx	//	transaction id
				, group_no	//	group number
				, abort_code	//	abort code
							//	choice + body
				, cyng::tuple_factory(static_cast<std::uint16_t>(type), body)
				, static_cast<std::uint16_t>(0xff)	//	placeholder for CRC16 (2 bytes)
				, cyng::eod());	//	end of message
		}

		cyng::tuple_t message(std::string trx
			, std::uint8_t group_no
			, std::uint8_t abort_code
			, sml_messages_enum type
			, cyng::tuple_t body)
		{
			return message(cyng::make_object(trx), group_no, abort_code, type, body);
		}

		cyng::tuple_t open_response(cyng::object codepage
			, cyng::object client_id
			, cyng::object req_file_id
			, cyng::object server_id
			, cyng::object ref_time
			, cyng::object version)
		{
			return cyng::tuple_t({ codepage, client_id, req_file_id, server_id, ref_time, version });
		}

		cyng::tuple_t open_request(cyng::object codepage
			, cyng::object client_id
			, cyng::object req_file_id
			, cyng::object server_id
			, cyng::object username
			, cyng::object password
			, cyng::object version)
		{
			return cyng::tuple_t({ codepage, client_id, req_file_id, server_id, username, password, version });
		}

		cyng::tuple_t close_response(cyng::object signature)
		{
			return cyng::tuple_t({ signature });
		}

		cyng::tuple_t get_proc_parameter_response(cyng::object server_id
			, obis code
			, cyng::tuple_t params)
		{
			return cyng::tuple_factory(server_id
				, cyng::tuple_factory(code.to_buffer())	//	path entry
				, params);
		}

		cyng::tuple_t get_proc_parameter_response(cyng::buffer_t server_id
			, obis code
			, cyng::tuple_t params)
		{
			return cyng::tuple_factory(server_id
				, cyng::tuple_factory(code.to_buffer())	//	path entry
				, params);
		}

		/**
		 * locale the child list in a GetProcParameter.Res message
		 */
		cyng::tuple_t& locate_child_list(cyng::tuple_t& msg)
		{
			BOOST_ASSERT(msg.size() == 6);
			if (msg.size() == 6) {

				//
				//	navigate to message body
				//
				auto pos = msg.begin();
				std::advance(pos, 3);

				auto body = const_cast<cyng::tuple_t*>(cyng::object_cast<cyng::tuple_t>(*pos));
				if (body != nullptr) {
					BOOST_ASSERT(body->size() == 2);
					if (body->size() == 2) {

						//
						//	navigate to GetProcParameter.Res
						//
						pos = body->begin();
						auto const msg_type = cyng::value_cast<std::uint16_t>(*pos, 0u);
						if (msg_type == BODY_GET_PROC_PARAMETER_RESPONSE) {

							//
							//	navigate to message body
							//
							std::advance(pos, 1);

							auto res = const_cast<cyng::tuple_t*>(cyng::object_cast<cyng::tuple_t>(*pos));
							if (res != nullptr) {
								BOOST_ASSERT(res->size() == 3);
								if (res->size() == 3) {

									//
									//	navigate to parameter tree
									//
									pos = res->begin();
									std::advance(pos, 2);

									auto param_tree = const_cast<cyng::tuple_t*>(cyng::object_cast<cyng::tuple_t>(*pos));
									BOOST_ASSERT(param_tree->size() == 3);
									if (param_tree->size() == 3) {

										return *param_tree;
									}

								}
							}
						}

						//
						//	navigate to SML_GetProfileList.Res
						//
						else if (msg_type == BODY_GET_PROFILE_LIST_RESPONSE) {

							//
							//	navigate to message body
							//
							std::advance(pos, 1);

							auto res = const_cast<cyng::tuple_t*>(cyng::object_cast<cyng::tuple_t>(*pos));
							if (res != nullptr) {
								BOOST_ASSERT(res->size() == 9);
								if (res->size() == 9) {

									//
									//	navigate to List_of_SML_PeriodEntry 
									//
									pos = res->begin();
									std::advance(pos, 6);

									auto period_list = const_cast<cyng::tuple_t*>(cyng::object_cast<cyng::tuple_t>(*pos));
									if (period_list != nullptr) {
										return *period_list;
									}
								}
							}
						}
					}
				}
			}
			return msg;
		}

		bool merge_child_list(obis const* obis_begin
			, obis const* obis_end
			, cyng::tuple_t& tpl
			, cyng::tuple_t&& val)
		{
			//	there is more to come
			BOOST_ASSERT((obis_begin + 1) != obis_end);

			for (auto& childs : tpl) {

				auto entry = const_cast<cyng::tuple_t*>(cyng::object_cast<cyng::tuple_t>(childs));
				if (entry != nullptr) {

					//
					//	parameter name
					//
					cyng::buffer_t code;
					code = cyng::value_cast(entry->front(), code);

					if (*obis_begin == obis(code)) {

						//
						//	tree entry exists
						//
						return merge_child_list(obis_begin
							, obis_end
							, entry->begin()
							, entry->end()
							, std::move(val));
					}
				}
			}

			//
			//	tree entry not found - create one
			//
			auto entry = child_list_tree(*obis_begin, {});

			auto b = merge_child_list(obis_begin
				, obis_end
				, entry.begin()
				, entry.end()
				, std::move(val));
			tpl.push_back(cyng::make_object(entry));
			return b;
		}

		bool merge_child_list(obis const * obis_begin
			, obis const* obis_end
			, cyng::tuple_t::iterator cl_begin
			, cyng::tuple_t::iterator cl_end
			, cyng::tuple_t&& val)
		{
			BOOST_ASSERT(obis_begin != nullptr);
			BOOST_ASSERT(obis_end != nullptr);
			BOOST_ASSERT(obis_begin != obis_end);
			BOOST_ASSERT(cl_begin != cl_end);

			if (obis_begin == nullptr)	return false;
			if (obis_end == nullptr)	return false;
			if (obis_begin == obis_end)	return false;
			if (cl_begin == cl_end) return false;

			//
			//	tree entry expected (OBIS, attribute, child list)
			//
			auto const size = std::distance(cl_begin, cl_end);
			BOOST_ASSERT(size == 3);
			if (size == 3) {

				//
				//	parameter name
				//
				cyng::buffer_t const code = cyng::to_buffer(*cl_begin);

				BOOST_ASSERT(*obis_begin == obis(code));
				if ((obis_begin + 1) != obis_end) ++obis_begin;

				//
				//	navigate to child list
				//
				std::advance(cl_begin, 2);

				if (cl_begin->is_null()) {

					//
					//	no child list (tuple) 
					//	Create a tuple(of tree entries) with one entry
					//	containing the required parameter name (OBIS code)
					//
					if ((obis_begin + 1) == obis_end) {

						//
						//	make the final attribute entry
						//
						auto attr = parameter_tree(*obis_begin, std::move(val));
						auto tpl = cyng::tuple_factory(attr);
						*cl_begin = cyng::make_object(tpl);
						return true;
					}
					else {

						//
						//	walk down the child list
						//
						auto tpl = cyng::tuple_factory(empty_tree(*obis_begin));
						*cl_begin = cyng::make_object(tpl);

						return merge_child_list(obis_begin
							, obis_end
							, tpl
							, std::move(val));
					}
				}
				else {

					//
					//	get the child list (tuple)
					//
					auto child_list = const_cast<cyng::tuple_t*>(cyng::object_cast<cyng::tuple_t>(*cl_begin));
					BOOST_ASSERT(child_list != nullptr);

					if ((obis_begin + 1) == obis_end) {

						//
						//	make the final attribute entry
						//
						auto attr = parameter_tree(*obis_begin, std::move(val));
						child_list->push_back(cyng::make_object(attr));
						return true;
					}
					else {

						//
						//	walk down the child list
						//
						return merge_child_list(obis_begin
							, obis_end
							, *child_list
							, std::move(val));

					}
				}
			}
			return false;
		}

		bool append_get_proc_response(cyng::tuple_t& msg, std::initializer_list<obis> path, cyng::tuple_t&& val) 
		{
			if (path.size() == 0u)	return false;
			if (val.size() != 2u)	return false;

			auto & child_list = locate_child_list(msg);
			return (child_list.size() == 3)
				? merge_child_list(path.begin(), path.end(), child_list.begin(), child_list.end(), std::move(val))
				: false;
		}

		void append_period_entry(cyng::tuple_t& msg
			, obis code
			, cyng::object&& obj)
		{
			auto& child_list = locate_child_list(msg);
			child_list.push_back(std::move(obj));
		}


		cyng::tuple_t get_proc_parameter_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis code)
		{
			//
			//	5 elements:
			//
			//	* server ID
			//	* username
			//	* password
			//	* parameter tree (OBIS)
			//	* attribute (not set = 01)
			//
			return cyng::tuple_factory(server_id
				, username
				, password
				, cyng::tuple_factory(code.to_buffer())	//	path entry
				, cyng::make_object());

		}

		cyng::tuple_t set_proc_parameter_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis code
			, cyng::tuple_t params)
		{
			return cyng::tuple_factory(server_id
				, username
				, password
				, cyng::tuple_factory(code.to_buffer())	//	path entry
				, params);
		}

		cyng::tuple_t set_proc_parameter_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis_path tree_path
			, cyng::tuple_t params)
		{
			cyng::tuple_t seq;
			for (auto const& code : tree_path) {
				seq.push_back(cyng::make_object(code.to_buffer()));
			}

			return cyng::tuple_factory(server_id
				, username
				, password
				, seq	//	path entry
				, params);
		}


		cyng::tuple_t get_profile_list_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, bool with_raw_data
			, std::chrono::system_clock::time_point begin_time
			, std::chrono::system_clock::time_point end_time
			, obis code //	tree path
			, cyng::tuple_t& obj_list
			, cyng::tuple_t& details)
		{
			return cyng::tuple_factory(server_id
				, username
				, password
				, with_raw_data
				, begin_time
				, end_time
				, cyng::tuple_factory(code.to_buffer())	//	path entry
				, obj_list
				, details);
		}

		cyng::tuple_t get_profile_list_response(cyng::buffer_t server_id
			, std::chrono::system_clock::time_point act_time
			, std::uint32_t reg_period
			, obis code //	tree path
			, std::chrono::system_clock::time_point val_time
			, std::uint64_t status
			, cyng::tuple_t&& data)
		{
			return cyng::tuple_factory(server_id
				, act_time
				, reg_period
				, cyng::tuple_factory(code.to_buffer())	//	path entry
				, val_time
				, status
				, std::move(data)
				, cyng::null()		//	rawdata
				, cyng::null());	//	signature
		}

		cyng::tuple_t get_list_request(cyng::object client_id
			, cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis code)
		{
			return cyng::tuple_factory(client_id
				, server_id
				, username
				, password
				, code.to_buffer());
		}

		cyng::tuple_t get_list_response(cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id
			, obis list_name
			, cyng::tuple_t act_sensor_time
			, cyng::tuple_t act_gateway_time
			, cyng::tuple_t val_list)
		{
			if (client_id.empty()) {
				return cyng::tuple_factory(cyng::null()
					, server_id
					, list_name.to_buffer()
					, act_sensor_time
					, val_list
					, cyng::null()	//	no list signature
					, act_gateway_time);

			}
			return cyng::tuple_factory(client_id
				, server_id
				, list_name.to_buffer()
				, act_sensor_time
				, val_list
				, cyng::null()	//	no list signature
				, act_gateway_time);
		}

		cyng::tuple_t get_attention_response(cyng::buffer_t const& server_id
			, cyng::buffer_t const& attention_nr
			, std::string attention_msg
			, cyng::tuple_t attention_details)
		{
			return (attention_details.empty())	
				? cyng::tuple_factory(server_id
					, attention_nr
					, attention_msg
					, cyng::null())
				: cyng::tuple_factory(server_id
					, attention_nr
					, attention_msg
					, attention_details)
				;
		}

		cyng::buffer_t boxing(std::vector<cyng::buffer_t> const& inp)
		{
			//
			//	trailer v1.0
			//
			cyng::buffer_t buf{ 0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01 };

			//
			//	append messages
			//
			std::for_each(inp.begin(), inp.end(), [&buf](cyng::buffer_t const& msg) {
				buf.insert(buf.end(), msg.begin(), msg.end());
			});

			//
			//	padding bytes
			//
			const char pad = ((buf.size() % 4) == 0)
				? 0
				: (4 - (buf.size() % 4))
				;
			for (std::size_t pos = 0; pos < pad; ++pos)
			{
				buf.push_back(0x0);
			}

			//
			//	tail v1.0
			//
			buf.insert(buf.end(), { 0x1b, 0x1b, 0x1b, 0x1b, 0x1a, pad });


			//
			//	CRC calculation over complete buffer
			//
			crc_16_data crc;
			//crc.crc_ = sml_crc16_calculate(buf);
			crc.crc_ = sml_crc16_calculate((const unsigned char*)buf.data(), buf.size());

			//	network order
			buf.push_back(crc.data_[1]);
			buf.push_back(crc.data_[0]);

			return buf;
		}

		cyng::tuple_t empty_tree(obis code)
		{
			return cyng::tuple_t({ cyng::make_object(code.to_buffer())
				, cyng::make_object()
				, cyng::make_object() });
		}

		cyng::tuple_t parameter_tree(obis code, cyng::tuple_t&& value)
		{
			return cyng::tuple_t({ cyng::make_object(code.to_buffer())
				, cyng::make_object(std::move(value))
				, cyng::make_object() });
		}

		cyng::tuple_t child_list_tree(obis code, cyng::tuple_t value)
		{
			return cyng::tuple_t({ cyng::make_object(code.to_buffer())
				, cyng::make_object()
				, cyng::make_object(value) });
		}

		cyng::tuple_t child_list_tree(obis code, std::initializer_list<cyng::tuple_t> list)
		{
			cyng::tuple_t tpl;
			std::for_each(list.begin(), list.end(), [&tpl](cyng::tuple_t const& e) {
				tpl.push_back(cyng::make_object(e));
			});

			return cyng::tuple_t({ cyng::make_object(code.to_buffer())
				, cyng::make_object()	//	no attribute
				, cyng::make_object(tpl) });
		}

		cyng::object period_entry(obis code
			, std::uint8_t unit
			, std::int8_t scaler
			, cyng::object value)
		{
			return cyng::make_object(cyng::tuple_factory(code.to_buffer()
				, unit
				, scaler
				, value
				, cyng::null()));	//	signature
		}

		cyng::object list_entry(obis code
			, std::uint64_t status
			, std::chrono::system_clock::time_point val_time
			, std::uint8_t unit
			, std::int8_t scaler
			, cyng::object value)
		{
			return cyng::make_object(cyng::tuple_factory(code.to_buffer()
				, status
				, val_time
				, unit
				, scaler
				, value
				, cyng::null()));	//	signature
		}

		cyng::object list_entry_manufacturer(std::string manufacturer)
		{
			return cyng::make_object(cyng::tuple_factory(OBIS_DATA_MANUFACTURER.to_buffer()
				, cyng::null()	//	status
				, cyng::null()	//	val_time
				, cyng::null()	//	unit
				, cyng::null()	//	scaler
				, manufacturer
				, cyng::null()));	//	signature
		}

		cyng::object list_entry(obis code
			, cyng::object status
			, cyng::object val_time
			, cyng::object unit
			, cyng::object scaler
			, cyng::object value)
		{
			return cyng::make_object(cyng::tuple_factory(code.to_buffer()
				, status
				, val_time
				, unit
				, scaler
				, value
				, cyng::null()));	//	signature
		}

	}
}
