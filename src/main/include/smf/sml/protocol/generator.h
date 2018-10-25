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
#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/mac.h>
#include <cyng/store/table.h>
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

		//protected:
			std::size_t append_msg(cyng::tuple_t&&);

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
			req_generator();

			std::size_t public_open(cyng::mac48 client_id
				, cyng::buffer_t const& server_id
				, std::string const& name
				, std::string const& pwd);

			std::size_t public_close();

			/**
			 * Restart system - 81 81 C7 83 82 01
			 */
			std::size_t set_proc_parameter_restart(cyng::buffer_t const& server_id
				, std::string const& username
				, std::string const& password);

			/**
			 * get list of visible servers/meters - 81 81 10 06 FF FF
			 */
			std::size_t get_proc_parameter_srv_visible(cyng::buffer_t const& server_id
				, std::string const& username
				, std::string const& password);

			/**
			 * get list of active servers/meters - 81 81 11 06 FF FF
			 */
			std::size_t get_proc_parameter_srv_active(cyng::buffer_t const& server_id
				, std::string const& username
				, std::string const& password);

			/**
			 * get list of available firmware versions - 81 81 C7 82 FF (CODE_ROOT_DEVICE_IDENT)
			 */
			std::size_t get_proc_parameter_firmware(cyng::buffer_t const& server_id
				, std::string const& username
				, std::string const& password);

			/**
			 * get status word - 81 00 60 05 00 00 (CLASS_OP_LOG_STATUS_WORD)
			 */
			std::size_t get_proc_status_word(cyng::buffer_t const& server_id
				, std::string const& username
				, std::string const& password);

			/**
			 * get list of available firmware versions - 81 81 C7 82 FF (CODE_ROOT_MEMORY_USAGE)
			 */
			std::size_t get_proc_parameter_memory(cyng::buffer_t const& server_id
				, std::string const& username
				, std::string const& password);

		private:
			trx	trx_;
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
			 * Generate an empty response with the specified tree path
			 */
			std::size_t empty(cyng::object trx
				, cyng::object server_id
				, obis);

			/**
			 * OBIS_CLASS_OP_LOG_STATUS_WORD
			 */
			std::size_t get_proc_parameter_status_word(cyng::object trx
				, cyng::object server_id
				, std::uint32_t);

			/**
			 * OBIS_CODE_ROOT_DEVICE_IDENT - 81 81 C7 82 01 FF
			 */
			std::size_t get_proc_parameter_device_id(cyng::object trx
				, cyng::object server_id
				, std::string const& manufacturer
				, cyng::buffer_t const& server_id2
				, std::string const& model_code
				, std::uint32_t serial);

			/**
			 * OBIS_CODE_ROOT_MEMORY_USAGE - 00 80 80 00 10 FF
			 */
			std::size_t get_proc_mem_usage(cyng::object trx
				, cyng::object server_id
				, std::uint8_t
				, std::uint8_t);

			std::size_t get_proc_device_time(cyng::object trx
				, cyng::object server_id
				, std::chrono::system_clock::time_point
				, std::int32_t
				, bool);

			std::size_t get_proc_active_devices(cyng::object trx
				, cyng::object server_id
				, const cyng::store::table*);

			std::size_t get_proc_visible_devices(cyng::object trx
				, cyng::object server_id
				, const cyng::store::table*);

			/**
			 * 81 81 C7 86 00 FF
			 * Deliver an overview of the system configuration and state.
			 */
			std::size_t get_proc_sensor_property(cyng::object trx
				, cyng::object server_id
				, const cyng::table::record&);

			std::size_t get_proc_push_ops(cyng::object trx
				, cyng::object server_id
				, const cyng::store::table*);

			std::size_t get_proc_ipt_params(cyng::object trx
				, cyng::object server_id
				, node::ipt::redundancy const& cfg);

			std::size_t get_proc_0080800000FF(cyng::object trx
				, cyng::object server_id
				, std::uint32_t);

			std::size_t get_proc_990000000004(cyng::object trx
				, cyng::object server_id
				, std::string const&);

			std::size_t get_proc_actuators(cyng::object trx
				, cyng::object server_id);

			/**
			 * Operation log data
			 */
			std::size_t get_profile_op_log(cyng::object trx
				, cyng::object client_id
				, std::chrono::system_clock::time_point act_time
				, std::uint32_t reg_period
				, std::chrono::system_clock::time_point val_time
				, std::uint64_t status
				, std::uint32_t evt
				, obis peer_address
				, std::chrono::system_clock::time_point tp
				, cyng::buffer_t const& server_id
				, std::string const& target
				, std::uint8_t push_nr);

			std::size_t get_profile_op_logs(cyng::object trx
				, cyng::object client_id
				, std::chrono::system_clock::time_point start_time
				, std::chrono::system_clock::time_point end_time
				, const cyng::store::table*);
		};

	}
}
#endif