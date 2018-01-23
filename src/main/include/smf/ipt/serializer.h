/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_IPT_SERIALIZER_H
#define NODE_IPT_SERIALIZER_H

#include <smf/ipt/defs.h>
#include <smf/ipt/scramble_key.h>
#include <NODE_project_info.h>
#include <cyng/crypto/scrambler.hpp>
#include <cyng/vm/controller.h>
#include <cyng/crypto/rotating_counter.hpp>
#include <type_traits>
#include <boost/asio.hpp>

namespace node
{
	namespace ipt
	{
		class serializer
		{
		public:
			using scrambler_t = cyng::crypto::scrambler<char, scramble_key::SCRAMBLE_KEY_LENGTH>;
			using seq_generator = cyng::circular_counter< std::uint8_t, 1, 0xff >;

		public:
			serializer(boost::asio::ip::tcp::socket& s
				, cyng::controller& vm
				, scramble_key const&);

		private:
			void set_sk(cyng::context& ctx);

			void req_login_public(cyng::context& ctx);
			void req_login_scrambled(cyng::context& ctx);
			void res_login_public(cyng::context& ctx);
			void res_login_scrambled(cyng::context& ctx);

			void req_open_push_channel(cyng::context& ctx);
			void res_open_push_channel(cyng::context& ctx);

			void req_close_push_channel(cyng::context& ctx);
			void res_close_push_channel(cyng::context& ctx);

			void req_transfer_push_data(cyng::context& ctx);
			void res_transfer_push_data(cyng::context& ctx);

			void req_open_connection(cyng::context& ctx);
			void res_open_connection(cyng::context& ctx);

			void req_close_connection(cyng::context& ctx);
			void res_close_connection(cyng::context& ctx);

			void req_protocol_version(cyng::context& ctx);
			void res_protocol_version(cyng::context& ctx);

			void req_software_version(cyng::context& ctx);
			void res_software_version(cyng::context& ctx);

			void req_device_id(cyng::context& ctx);
			void res_device_id(cyng::context& ctx);

			void req_network_status(cyng::context& ctx);
			void res_network_status(cyng::context& ctx);

			void req_ip_statistics(cyng::context& ctx);
			void res_ip_statistics(cyng::context& ctx);

			void req_device_auth(cyng::context& ctx);
			void res_device_auth(cyng::context& ctx);

			void req_device_time(cyng::context& ctx);
			void res_device_time(cyng::context& ctx);

			void req_push_target_namelist(cyng::context& ctx);
			void res_push_target_namelist(cyng::context& ctx);

			void req_push_target_echo(cyng::context& ctx);
			void res_push_target_echo(cyng::context& ctx);

			void req_traceroute(cyng::context& ctx);
			void res_traceroute(cyng::context& ctx);

			void req_maintenance(cyng::context& ctx);
			void res_maintenance(cyng::context& ctx);

			void req_logout(cyng::context& ctx);
			void res_logout(cyng::context& ctx);

			void req_register_push_target(cyng::context& ctx);
			void res_register_push_target(cyng::context& ctx);

			void req_deregister_push_target(cyng::context& ctx);
			void res_deregister_push_target(cyng::context& ctx);

			void req_watchdog(cyng::context& ctx);
			void res_watchdog(cyng::context& ctx);

			void req_multi_ctrl_public_login(cyng::context& ctx);
			void res_multi_ctrl_public_login(cyng::context& ctx);

			void write_header(command_type cmd, sequence_type seq, std::size_t length);

			/**
			 * Write a bunch of data scrambled to output stream.
			 */
			void put(const char* p, std::size_t size);
			void put(char c);

			/**
			 * Append a '\0' value to terminate string
			 */
			void write(std::string const&);
			void write(scramble_key::key_type const&);

			template <typename T >
			void write_numeric(T v)
			{
				static_assert(std::is_arithmetic<T>::value, "arithmetic type required");
				put(reinterpret_cast<const char*>(&v), sizeof(T));
			}


		private:
			boost::asio::streambuf buffer_;
			std::ostream ostream_;

			/**
			 * produces consecutive sequence numbers
			 * between 1 and 0xff (0 is excluded)
			 */
			seq_generator	sgen_;
			sequence_type	last_seq_;

			/**
			 * Encrypting data stream
			 */
			scrambler_t	scrambler_;
			const scramble_key	def_key_;	//!< default scramble key
		};
	}
}

#endif
