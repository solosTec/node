/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/value.hpp>
#include <smf/sml/crc16.h>

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

		cyng::tuple_t close_response(cyng::object signature)
		{
			return cyng::tuple_t({ signature });
		}

		cyng::tuple_t get_proc_parameter_response(cyng::object server_id
			, obis code
			, cyng::tuple_t params)
		{
			return cyng::tuple_factory( server_id
				, cyng::tuple_factory(code.to_buffer())	//	path entry
				, params);
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

	}
}
