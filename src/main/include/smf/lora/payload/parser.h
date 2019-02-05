/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_LORA_PAYLOAD_PARSER_H
#define NODE_LIB_LORA_PAYLOAD_PARSER_H

#include <cyng/intrinsics/buffer.h>
#include <string>
#include <cstdint>

namespace node
{
	namespace lora
	{
		/**
		 * Payload has a fixed size of 20 bytes
		 */
		constexpr unsigned PAYLOAD_SIZE = 20;

		/**
		 * Convert a string into a binary buffer with 20 bytes length
		 */
		cyng::buffer_t parse_payload(std::string const&);

		/**
		 * @return protocol version
		 */
		std::uint8_t	version(cyng::buffer_t const&);

		/**
		 * @return encoded manufacturer name (3 characters)
		 */
		std::string		manufacturer(cyng::buffer_t const&);

		/**
		 * @return serial number as string with a length of 8 bytes.
		 */
		std::string		meter_id(cyng::buffer_t const&);

		/**
		 * Same as meter id but with binary values
		 */
		cyng::buffer_t	server_id(cyng::buffer_t const&);

		/**
		 * @return medium code
		 * @see http://www.m-bus.com/mbusdoc/md8.php
		 */
		std::uint8_t	medium(cyng::buffer_t const&);

		/**
		 * @return M-Bus state
		 */
		std::uint8_t	state(cyng::buffer_t const&);

		/**
		 * @return actuality duration in minutes
		 */
		std::uint16_t	actuality(cyng::buffer_t const&);

		/**
		 * @return Volume VIF (Value Information Field)
		 */
		std::uint8_t	vif(cyng::buffer_t const&);
		std::uint32_t	volume(cyng::buffer_t const&);

		/**
		 * @return calculate the value from VIF and volume as string.
		 */
		std::string		value(std::uint8_t, std::uint32_t);

		/**
		 * @return bitfield with additional functions
		 */
		std::uint8_t	functions(cyng::buffer_t const&);

		/**
		 * @return the bitfield with the encoded battery lifetime
		 */
		std::uint8_t	lifetime(cyng::buffer_t const&);

		/**
		 * Maximim value is 0x1f (31)
		 * @return decoded battery lifetime as semesters.
		 */
		std::uint32_t	semester(std::uint8_t);

		/**
		 * return true if link error (bit 2) was set
		 */
		bool	link_error(cyng::buffer_t const&);

		/**
		 * return CRC16 value
		 */
		std::uint16_t	crc(cyng::buffer_t const&);

		/**
		 * return calculated CRC16 from the first 18 bytes
		 */
		std::uint16_t	crc_calculated(cyng::buffer_t const&);

		/**
		 * return true if CRC is ok.
		 */
		bool	crc_ok(cyng::buffer_t const&);

		/**
		 * Helper function. Takes the VIF and calulates the
		 * resulting scaler.
		 */
		std::int8_t	scaler(std::uint8_t vif);
	}
}

#endif
