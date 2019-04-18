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

namespace node
{
	namespace sml
	{
		cyng::tuple_t message(cyng::object trx
			, std::uint8_t groupNo
			, std::uint8_t abortCode
			, sml_messages_enum type
			, cyng::tuple_t body)
		{
			return cyng::tuple_factory( trx	//	transaction id
				, groupNo	//	group number
				, abortCode	//	abort code
							//	choice + body
				, cyng::tuple_factory(static_cast<std::uint16_t>(type), body)
				, static_cast<std::uint16_t>(0xff)	//	placeholder for CRC16 (2 bytes)
				, cyng::eod());	//	end of message
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

		cyng::tuple_t get_profile_list_response(cyng::object server_id
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

		cyng::tuple_t parameter_tree(obis code, cyng::tuple_t value)
		{
			return cyng::tuple_t({ cyng::make_object(code.to_buffer())
				, cyng::make_object(value)
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
				, cyng::make_object()
				, cyng::make_object(tpl) });
		}

		cyng::tuple_t tree(obis code, cyng::tuple_t param, cyng::tuple_t list)
		{
			return cyng::tuple_t({ cyng::make_object(code.to_buffer())
				, cyng::make_object(param)
				, cyng::make_object(list) });
		}

		cyng::tuple_t tree(obis code, cyng::tuple_t param, std::initializer_list<cyng::tuple_t> list)
		{
			cyng::tuple_t tpl;
			std::for_each(list.begin(), list.end(), [&tpl](cyng::tuple_t const& e) {
				tpl.push_back(cyng::make_object(e));
			});

			return cyng::tuple_t({ cyng::make_object(code.to_buffer())
				, cyng::make_object(param)
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
