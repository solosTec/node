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

			/**
			 * Produce complete SML message from message buffer
			 */
			cyng::buffer_t boxing() const;

			/**
			 * Serialize message and append result to message buffer.
			 * 
			 * @return index in message buffer
			 */
			std::size_t append(cyng::tuple_t&&);

		protected:
			/**
			 * buffer for current SML message
			 */
			std::vector<cyng::buffer_t>	msg_;
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

			std::string public_open(cyng::mac48 client_id
				, cyng::buffer_t const& server_id);

			std::string public_close();

			/**
			 * Generic set process parameter function
			 *
			 * @return transaction ID
			 */
			template< typename T >
			std::string set_proc_parameter(cyng::buffer_t const& server_id
				, obis_path_t&& tree_path
				, T&& val)
			{
				++trx_;
				auto const trx = *trx_;
				append(message(cyng::make_object(trx)
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
					)
				);
				return trx;
			}

			/**
			 * Generate an empty set proc parameter request.
			 *
			 * @return transaction ID
			 */
			std::pair<cyng::tuple_t, std::string> empty_set_proc_param(cyng::buffer_t server_id, obis root);

			/**
			 * Restart system - 81 81 C7 83 82 01
			 */
			std::size_t set_proc_parameter_restart(cyng::buffer_t const& server_id);

			/**
			 * IP-T Host - 81 49 0D 07 00 FF
			 *
			 * @return transaction ID
			 */
			std::string set_proc_parameter_ipt_host(cyng::buffer_t const& server_id
				, std::uint8_t idx
				, std::string const& address);

			/**
			 * set local IP-T port
			 *
			 * @return transaction ID
			 */
			std::string set_proc_parameter_ipt_port_local(cyng::buffer_t const& server_id
				, std::uint8_t idx
				, std::uint16_t);

			/**
			 * set remote IP-T port
			 *
			 * @return transaction ID
			 */
			std::string set_proc_parameter_ipt_port_remote(cyng::buffer_t const& server_id
				, std::uint8_t idx
				, std::uint16_t);

			/**
			 * set IP-T user name
			 *
			 * @return transaction ID
			 */
			std::string set_proc_parameter_ipt_user(cyng::buffer_t const& server_id
				, std::uint8_t idx
				, std::string const&);

			/**
			 * set IP-T user password
			 *
			 * @return transaction ID
			 */
			std::string set_proc_parameter_ipt_pwd(cyng::buffer_t const& server_id
				, std::uint8_t idx
				, std::string const&);

			/**
			 * set wireless M-Bus protocol type
			 * <ul>
			 * <li>0 = T-Mode</li>
			 * <li>1 = S-Mode</li>
			 * <li>2 = S/T-Automatik (wechselnd)</li>
			 * <li>3 = S/T-Automatik  (Parallelbetrieb)</li>
			 * </ul>
			 *
			 * @return transaction ID
			 */
			std::string set_proc_parameter_wmbus_protocol(cyng::buffer_t const& server_id
				, std::uint8_t);


			/**
			 * Simple root query - BODY_GET_PROC_PARAMETER_REQUEST (0x500)
			 * @return transaction ID
			 */
			std::string get_proc_parameter(cyng::buffer_t const& server_id
				, obis);
			std::string get_proc_parameter(cyng::buffer_t const& server_id
				, obis_path_t);

			/**
			 * List query - BODY_GET_LIST_REQUEST (0x700)
			 */
			std::string get_list(cyng::buffer_t const& client_id
				, cyng::buffer_t const& server_id
				, obis);

			/**
			 * get last data record - 99 00 00 00 00 03 (CODE_LAST_DATA_RECORD)
			 *  SML_GetList_Req
			 */
			std::string get_list_last_data_record(cyng::buffer_t const& client_id
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
			std::string get_profile_list(cyng::buffer_t const& server_id
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

			std::size_t public_open(cyng::object trx
				, cyng::object	//	client id
				, cyng::object	//	req file id
				, cyng::object	//	server id
			);

			std::size_t public_close(cyng::object trx);

			/**
			 * Generate an empty BODY_GET_PROC_PARAMETER_RESPONSE with the specified tree path
			 *
			 * @return message number
			 */
			std::size_t empty(std::string trx
				, cyng::buffer_t server_id
				, obis);

			/**
			 * Generate an empty BODY_GET_PROC_PARAMETER_RESPONSE with the specified tree path
			 *
			 * @return message tuple
			 */
			cyng::tuple_t empty_get_proc_param(std::string trx
				, cyng::buffer_t server_id
				, obis);

			cyng::tuple_t empty_get_proc_param(std::string trx
				, cyng::buffer_t server_id
				, obis_path_t);


			/**
			 * Generate an empty SML_GetProfileList_Res with the specified tree path
			 *
			 * @return message tuple
			 */
			cyng::tuple_t empty_get_profile_list(std::string trx
				, cyng::buffer_t client_id
				, obis path
				, std::chrono::system_clock::time_point act_time
				, std::uint32_t reg_period
				, std::chrono::system_clock::time_point val_time
				, std::uint64_t status);

			std::size_t get_profile_list(std::string trx
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
			std::size_t get_proc_parameter_device_id(std::string trx
				, cyng::buffer_t server_id
				, std::string const& manufacturer
				, cyng::buffer_t const& server_id2
				, std::string const& model_code
				, std::uint32_t serial);

			/**
			 * OBIS_ROOT_MEMORY_USAGE - 00 80 80 00 10 FF
			 */
			std::size_t get_proc_mem_usage(std::string trx
				, cyng::buffer_t server_id
				, std::uint8_t
				, std::uint8_t);

			std::size_t get_proc_device_time(std::string trx
				, cyng::buffer_t server_id
				, std::chrono::system_clock::time_point
				, std::int32_t
				, bool);

			std::size_t get_status_word(std::string trx
				, cyng::buffer_t server_id
				, std::int32_t);

			std::size_t get_proc_actuators(std::string trx
				, cyng::buffer_t server_id);

			/**
			 * get current IP-T status - 81 49 0D 06 00 FF (OBIS_ROOT_IPT_STATE)
			 */
			std::size_t get_proc_parameter_ipt_state(std::string trx
				, cyng::buffer_t server_id
				, boost::asio::ip::tcp::endpoint remote_ep
				, boost::asio::ip::tcp::endpoint local_ep);

			/**
			 * Operation log data
			 */
			std::size_t get_profile_op_log(std::string trx
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

			std::size_t get_proc_w_mbus_status(std::string trx
				, cyng::buffer_t client_id
				, std::string const&	// manufacturer of w-mbus adapter
				, cyng::buffer_t const&	//	adapter id (EN 13757-3/4)
				, std::string const&	//	firmware version of adapter
				, std::string const&);	//	hardware version of adapter);

			/**
			 * @param protocol node::mbus::radio_protocol
			 * @param s_mode Zeitdauer, die fortlaufend im SMode empfangen wird (der Parameter 
			 *	wird nur benötigt, falls vorstehend die Variante 2 ==  S/T - Automatik gewählt ist).
			 * @param t_mode Zeitdauer, die fortlaufend im TMode empfangen wird (der Parameter 
			 *	wird nur benötigt, falls vorstehend die Variante 2 ==  S/T - Automatik gewählt ist).
			 * @param reboot Periode, anzugeben in Sekunden, nach deren Ablauf das W-MBUS-Modem 
			 *	im MUC-C neu initialisiert werden soll. Bei 0 ist der automatische Reboot inaktiv. 
			 * @param power transmision power
			 * @param timeout Maximales Inter Message Timeout in Sekunden 
			 */
			std::size_t get_proc_w_mbus_if(std::string trx
				, cyng::buffer_t client_id
				, cyng::object protocol	// radio protocol
				, cyng::object s_mode	// duration in seconds
				, cyng::object t_mode	// duration in seconds
				, cyng::object reboot	//	duration in seconds
				, cyng::object power	//	transmision power (transmission_power)
				, cyng::object	//	installation mode
			);

			/**
			 * generate act_sensor_time and act_gateway_time with make_timestamp()
			 * or make_sec_index() from value.hpp
			 */
			std::size_t get_list(std::string trx
				, cyng::buffer_t const& client_id
				, cyng::buffer_t const& server_id
				, obis list_name
				, cyng::tuple_t act_sensor_time
				, cyng::tuple_t act_gateway_time
				, cyng::tuple_t val_list);

			/**
			 * produce an attention message
			 */
			std::size_t attention_msg(cyng::object trx
				, cyng::buffer_t const& server_id
				, cyng::buffer_t const& attention_nr
				, std::string attention_msg
				, cyng::tuple_t attention_details);

		};

	}
}
#endif
