/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_MESSAGE_H
#define NODE_LIB_SML_MESSAGE_H

#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/intrinsics/sets.h>

#include <ostream>
#include <initializer_list>
#include <chrono>

namespace node
{
	namespace sml
	{
		/**
		 * list of messages ready to serialize (see to_sml() function)
		 */
		using messages_t = std::vector < cyng::tuple_t >;

		/**
		 * Convert a list of messages into a SML message
		 */
		cyng::buffer_t to_sml(sml::messages_t const&);


		/**
		 * Each message is a tuple consiting of 6 elements.
		 * The last element is EOD (0).
		 * 
		 * @param trx transaction ID
		 * @param group_no group number
		 * @param abort_code abort code
		 * @param type message type
		 * @param value message body
		 */
		cyng::tuple_t message(cyng::object trx
			, std::uint8_t group_no
			, std::uint8_t abort_code
			, message_e type
			, cyng::tuple_t value);

		/**
		 * @param trx transaction ID
		 * @param group_no group number
		 * @param abort_code abort code
		 * @param type message type
		 * @param value message body
		 */
		cyng::tuple_t message(std::string trx
			, std::uint8_t group_no
			, std::uint8_t abort_code
			, message_e type
			, cyng::tuple_t value);

		/**
		 * @param codepage optional codepage
		 * @param client_id optional client ID
		 * @param req_file_id request file id
		 * @param server_id server ID
		 * @param ref_time optional reference timepoint (creating this message)
		 * @param version optional SML version
		 */
		cyng::tuple_t open_response(cyng::object codepage
			, cyng::object client_id
			, cyng::object req_file_id
			, cyng::object server_id
			, cyng::object ref_time
			, cyng::object version);

		/**
		 * @param codepage optional codepage
		 * @param client_id optional client ID
		 * @param req_file_id request file id
		 * @param server_id server ID
		 * @param username optional login name
		 * @param password optional password
		 * @param version optional SML version
		 */
		cyng::tuple_t open_request(cyng::object codepage
			, cyng::object client_id
			, cyng::object req_file_id
			, cyng::object server_id
			, cyng::object username
			, cyng::object password
			, cyng::object version);

		/**
		 * @param signature optional global signature
		 */
		cyng::tuple_t close_response(cyng::object signature);

		/**
		 * @param server_id
		 * @param code parameter tree path
		 * @param params parameter tree
		 */
		cyng::tuple_t get_proc_parameter_response(cyng::object server_id
			, obis code
			, cyng::tuple_t params);

		/**
		 * @param server_id
		 * @param code parameter tree path
		 * @param params parameter tree
		 */
		cyng::tuple_t get_proc_parameter_response(cyng::buffer_t server_id
			, obis code
			, cyng::tuple_t params);

		cyng::tuple_t get_proc_parameter_response(cyng::buffer_t server_id
			, obis_path_t path
			, cyng::tuple_t params);



		/** @brief append a child list (SML_Tree) at the specified path.
		 *
		 * msg must be of type SML_GetProcParameter.Res
		 *
		 * @param msg SML_GetProcParameter.Res
		 * @param code path
		 * @param val SML_ProcParValue (value, periodEntry, tupleENtry, time or listEntry)
		 * @param child List_of_SML_Tree
		 */
		bool merge_msg(cyng::tuple_t& msg
			, std::initializer_list<obis> path
			, cyng::tuple_t&& val
			, cyng::tuple_t&& child);

		/** @brief append a child list (SML_Tree) at the specified path.
		 *
		 * msg must be of type SML_GetProcParameter.Res
		 *
		 * @param msg SML_GetProcParameter.Res
		 * @param code path
		 * @param val SML_ProcParValue (value, periodEntry, tupleENtry, time or listEntry)
		 */
		bool merge_msg(cyng::tuple_t& msg
			, std::initializer_list<obis> path
			, cyng::tuple_t&& val);

		bool set_get_proc_response_attribute(cyng::tuple_t& msg
			, std::initializer_list<obis> path
			, cyng::tuple_t&& val);

		/**
		 * msg must be of type SML_GetProfileList.Res
		 *
		 * @param msg SML_GetProfileList.Res 
		 * @param code path
		 * @param obj result of period_entry() function
		 */
		void append_period_entry(cyng::tuple_t& msg
			, obis code
			, cyng::object&& obj);

		/**
		 * @param server_id
		 * @param code parameter tree path
		 * @param params parameter tree
		 */
		cyng::tuple_t get_proc_parameter_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis code);
		cyng::tuple_t get_proc_parameter_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis_path_t path);

		/**
		 * @param server_id
		 * @param username
		 * @param password
		 * @param code parameter tree path
		 * @param params parameter tree
		 */
		cyng::tuple_t set_proc_parameter_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis code
			, cyng::tuple_t params);

		/**
		 * @param server_id
		 * @param username
		 * @param password
		 * @param tree_path parameter tree path
		 * @param params parameter tree
		 */
		cyng::tuple_t set_proc_parameter_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis_path_t tree_path
			, cyng::tuple_t params);

		cyng::tuple_t get_profile_list_request(cyng::object server_id
			, std::string const& username
			, std::string const& password
			, bool with_raw_data
			, std::chrono::system_clock::time_point begin_time
			, std::chrono::system_clock::time_point end_time
			, obis code //	tree path
			, cyng::tuple_t& obj_list
			, cyng::tuple_t& details);

		cyng::tuple_t get_profile_list_response(cyng::buffer_t server_id
			, std::chrono::system_clock::time_point act_time
			, std::uint32_t reg_period
			, obis code //	tree path
			, std::chrono::system_clock::time_point val_time
			, std::uint64_t status
			, cyng::tuple_t&& data);

		/**
		 * @param client_id this is the MAC address of the inquirer (example:  00 50 56 C0 00 08)
		 * @param server_id meter id (example: 01 E6 1E 13 09 00 16 3C 07)
		 * @param code parameter tree path
		 * @param params parameter tree
		 */
		cyng::tuple_t get_list_request(cyng::object client_id
			, cyng::object server_id
			, std::string const& username
			, std::string const& password
			, obis code);

		cyng::tuple_t get_list_response(cyng::buffer_t const& client_id
			, cyng::buffer_t const& server_id
			, obis list_name
			, cyng::tuple_t act_sensor_time
			, cyng::tuple_t act_gateway_time
			, cyng::tuple_t val_list);

		/**
		 * Body of an attention response message
		 */
		cyng::tuple_t get_attention_response(cyng::buffer_t const& server_id
			, cyng::buffer_t const& attention_nr
			, std::string attention_msg
			, cyng::tuple_t attention_details);

		/**
		 * Add SML trailer and tail
		 */
		cyng::buffer_t boxing(std::vector<cyng::buffer_t> const&);

		/*
		 * Parameter value and child list are mutual (I haven't seen it otherwise).
		 * @param value typically the result of make_value<T>(...)
		 */
		cyng::tuple_t empty_tree(obis code);

		/**
		 * This is a single attribute
		 */
		cyng::tuple_t parameter_tree(obis, cyng::tuple_t&& value);
		cyng::tuple_t child_list_tree(obis, cyng::tuple_t value);
		cyng::tuple_t child_list_tree(obis, std::initializer_list<cyng::tuple_t> list);

		/**
		 * Helper function to transform a list of tuples into an single tuple
		 * of objects.
		 */
		cyng::tuple_t to_tuple(std::initializer_list<cyng::tuple_t> list);

		cyng::object period_entry(obis
			, std::uint8_t unit
			, std::int8_t scaler
			, cyng::object value);

		/**
		 * SML_ListEntry  - part of 5.1.15. SML_GetList.Res
		 * A SML_ListEntry is part of an SML_List that holds all values
		 * in a SML_GetList.Res.
		 */
		cyng::object list_entry(obis obj_name
			, std::uint64_t status
			, std::chrono::system_clock::time_point val_time
			, std::uint8_t unit
			, std::int8_t scaler
			, cyng::object value);

		/**
		 * SML_ListEntry  - part of 5.1.15. SML_GetList.Res
		 * 81, 81, C7, 82, 03, FF
		 */
		cyng::object list_entry_manufacturer(std::string);

		/**
		 * SML_ListEntry  - part of 5.1.15. SML_GetList.Res
		 */
		cyng::object list_entry(obis obj_name
			, cyng::object status
			, cyng::object val_time
			, cyng::object unit
			, cyng::object scaler
			, cyng::object value);

		/**
		 * Extract the transaction id from an SML message
		 */
		std::string get_trx(cyng::tuple_t const&);

		/**
		 * Extract the message type from an SML message
		 */
		message_e get_msg_type(cyng::tuple_t const&);

		/**
		 * @return the message type and an iterator to the message body.
		 * 
		 */
		std::pair<message_e, cyng::tuple_t::const_iterator> get_msg_body(cyng::tuple_t const&);
	}
}
#endif