/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_GENERATOR_H
#define NODE_LIB_SML_GENERATOR_H

#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <smf/ipt/config.h>
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/value.hpp>

#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/mac.h>
#include <random>
#include <chrono>

namespace node
{
	namespace sml
	{
		/**
		 * Generate SML transaction is with the following format:
		 * @code
		 * [7 random numbers] [1 number in ascending order].
		 * nnnnnnnnnnnnnnnnn-m
		 * @endcode
		 *
		 * The numbers as the ASCII codes from 48 up to 57.
		 * 
		 * In a former version the following format was used:
		 * @code
		 * [17 random numbers] ['-'] [1 number in ascending order].
		 * nnnnnnnnnnnnnnnnn-m
		 * @endcode
		 *
		 * This is to generate a series of continuous transaction IDs
		 * and after reshuffling to start a new series.
		 */
		class trx
		{
		public:
			trx();
			trx(trx const&);

			/**
			 *	generate new transaction id (reshuffling)
			 */
			void shuffle(std::size_t = 7);

			/**
			 * Increase internal transaction number. 
			 */
			trx& operator++();
			trx operator++(int);

			/**
			 *	@return value of last generated (shuffled) transaction id
			 *	as string.
			 */
			std::string operator*() const;


		private:
			/**
			 * Generate a random string
			 */
			std::random_device rng_;
			std::mt19937 gen_;

			/**
			 * The fixed part of a series
			 */
			std::string		value_;

			/**
			 * Ascending number
			 */
			std::uint16_t	num_;

		};

		/**
		 * Base class for all SML generators
		 */
		class generator
		{
		public:
			generator();

			/**
			 * @return a file id generated from current time stamp
			 */
			static std::string gen_file_id();

			/**
			 * Clear message buffer
			 */
			void reset();

		protected:
			/**
			 * buffer for current SML message
			 */
			std::uint8_t	group_no_;

		};

		/**
		 * Generate SML requests
		 */
		class req_generator : public generator
		{
		public:
			req_generator(std::string const& name
				, std::string const& pwd);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t public_open(cyng::mac48 client_id
				, cyng::buffer_t const& server_id);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t public_close();

			/**
			 * Generic set process parameter function
			 *
			 * @return transaction ID
			 */
			template< typename T >
			CYNG_ATTR_NODISCARD
			cyng::tuple_t set_proc_parameter(cyng::buffer_t const& server_id
				, obis_path_t&& tree_path
				, T&& val)
			{
				auto const trx = *++trx_;

				return message(cyng::make_object(trx)
					, group_no_++	//	group
					, 0 //	abort code
					, message_e::SET_PROC_PARAMETER_REQUEST	//	0x600 (1536)

					//
					//	generate process parameter request
					//
					, set_proc_parameter_request(cyng::make_object(server_id)
						, name_
						, pwd_
						, tree_path
						, parameter_tree(tree_path.back(), make_value(val)))
					);
			}

			/**
			 * Generate an empty set proc parameter request.
			 *
			 * @return transaction ID
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t empty_set_proc_param(cyng::buffer_t server_id, obis root);

			/**
			 * Restart system - 81 81 C7 83 82 01
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t set_proc_parameter_reboot(cyng::buffer_t const& server_id);

			/**
			 * Simple root query - BODY_GET_PROC_PARAMETER_REQUEST (0x500)
			 * @return transaction ID
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_parameter(cyng::buffer_t const& server_id
				, obis);
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_parameter(cyng::buffer_t const& server_id
				, obis_path_t);

			/**
			 * List query - BODY_GET_LIST_REQUEST (0x700)
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_list(cyng::buffer_t const& client_id
				, cyng::buffer_t const& server_id
				, obis);

			/**
			 * get last data record - 99 00 00 00 00 03 (CODE_LAST_DATA_RECORD)
			 *  SML_GetList_Req
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_list_last_data_record(cyng::buffer_t const& client_id
				, cyng::buffer_t const& server_id);

			/**
			 * @param serverId Octet String OPTIONAL,
			 * @param username Octet String OPTIONAL,
			 * @param password Octet String OPTIONAL,
			 * @param withRawdata Boolean OPTIONAL,
			 * @param begin_time SML_Time OPTIONAL,
			 * @param end_time SML_Time OPTIONAL,
			 * @param parameterTreePath SML_TreePath,
			 * @param object_List List_of_SML_ObjReqEntry OPTIONAL,
			 * @param dasDetails SML_Tree OPTIONAL
			 *
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_profile_list(cyng::buffer_t const& server_id
				, std::chrono::system_clock::time_point begin_time
				, std::chrono::system_clock::time_point end_time
				, obis path);

		private:
			trx	trx_;
			std::string const name_;
			std::string const pwd_;
		};

		/**
		 * Generate SML responses
		 */
		class res_generator : public generator
		{
		public:
			res_generator();

			CYNG_ATTR_NODISCARD
			cyng::tuple_t public_open(cyng::object trx
				, cyng::object	//	client id
				, cyng::object	//	req file id
				, cyng::object	//	server id
			);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t public_close(cyng::object trx);

			/**
			 * Generate an empty BODY_GET_PROC_PARAMETER_RESPONSE with the specified tree path
			 *
			 * @return message tuple
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t empty_get_proc_param(std::string trx
				, cyng::buffer_t server_id
				, obis);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t empty_get_proc_param(std::string trx
				, cyng::buffer_t server_id
				, obis_path_t);


			/**
			 * Generate an empty SML_GetProfileList_Res with the specified tree path
			 *
			 * @return message tuple
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t empty_get_profile_list(std::string trx
				, cyng::buffer_t client_id
				, obis path
				, std::chrono::system_clock::time_point act_time
				, std::uint32_t reg_period
				, std::chrono::system_clock::time_point val_time
				, std::uint64_t status);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_profile_list(std::string trx
				, cyng::buffer_t client_id
				, obis path
				, std::chrono::system_clock::time_point act_time
				, std::uint32_t reg_period
				, std::chrono::system_clock::time_point val_time
				, std::uint64_t status
				, cyng::tuple_t&& period_list);

			/**
			 * OBIS_CODE_ROOT_DEVICE_IDENT - 81 81 C7 82 01 FF
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_parameter_device_id(std::string trx
				, cyng::buffer_t server_id
				, std::string const& manufacturer
				, cyng::buffer_t const& server_id2
				, std::string const& model_code
				, std::uint32_t serial);

			/**
			 * OBIS_ROOT_MEMORY_USAGE - 00 80 80 00 10 FF
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_mem_usage(std::string trx
				, cyng::buffer_t server_id
				, std::uint8_t
				, std::uint8_t);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_device_time(std::string trx
				, cyng::buffer_t server_id
				, std::chrono::system_clock::time_point
				, std::int32_t
				, bool);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_status_word(std::string trx
				, cyng::buffer_t server_id
				, std::int32_t);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_actuators(std::string trx
				, cyng::buffer_t server_id);

			/**
			 * get current IP-T status - 81 49 0D 06 00 FF (OBIS_ROOT_IPT_STATE)
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_parameter_ipt_state(std::string trx
				, cyng::buffer_t server_id
				, boost::asio::ip::tcp::endpoint remote_ep
				, boost::asio::ip::tcp::endpoint local_ep);

			/**
			 * Operation log data
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_profile_op_log(std::string trx
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
				, std::string details);

			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_proc_w_mbus_status(std::string trx
				, cyng::buffer_t client_id
				, std::string const&	// manufacturer of w-mbus adapter
				, cyng::buffer_t const&	//	adapter id (EN 13757-3/4)
				, std::string const&	//	firmware version of adapter
				, std::string const&);	//	hardware version of adapter);

			/**
			 * generate act_sensor_time and act_gateway_time with make_timestamp()
			 * or make_sec_index() from value.hpp
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t get_list(std::string trx
				, cyng::buffer_t const& client_id
				, cyng::buffer_t const& server_id
				, obis list_name
				, cyng::tuple_t act_sensor_time
				, cyng::tuple_t act_gateway_time
				, cyng::tuple_t val_list);

			/**
			 * produce an attention message
			 */
			CYNG_ATTR_NODISCARD
			cyng::tuple_t attention_msg(cyng::object trx
				, cyng::buffer_t const& server_id
				, cyng::buffer_t const& attention_nr
				, std::string attention_msg
				, cyng::tuple_t attention_details);

		};

	}
}
#endif
