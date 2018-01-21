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

			void req_watchdog(cyng::context& ctx);
			void res_watchdog(cyng::context& ctx);

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
			 * Encrypting data stream
			 */
			scrambler_t	scrambler_;
			const scramble_key	def_key_;	//!< default scramble key
		};
	}
}

#endif
