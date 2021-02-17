/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_HEADER_H
#define SMF_IPT_HEADER_H

#include <smf/ipt.h>

#include <cyng/obj/intrinsics/buffer.h>

#include <boost/assert.hpp>

//	set packing alignment to 1 (dense)
#pragma pack( push, 1 )

namespace smf
{
	namespace ipt	
	{
		/**
		 *	header
		 */
		struct header
		{
			command_t		command_;
			sequence_t		sequence_;
			std::uint8_t	reserved_;
			std::uint32_t	length_;

			void reset(sequence_t = 0);
		};

		/**
		 * Convert a range of bytes to an IP-T header.
		 * Precondition: buffer must have a size of 8 bytes.
		 */
		header to_header(cyng::buffer_t const& src);

		constexpr bool has_data(header const& h) {
			return h.length_ != HEADER_SIZE;
		}

		/**
		 *	@return message size in bytes without header
		 */
		constexpr std::size_t size(header const& h) {
			return (h.length_ < HEADER_SIZE)
				? HEADER_SIZE
				: (h.length_ - HEADER_SIZE)
				;
		}

		/**
		 *	Test bit 15.
		 *
		 *	@return <code>true</code> if the specified command is a request.
		 */
		constexpr bool is_request(command_t cmd)	{
			return ((cmd & 0x8000) == 0x8000);
		}

		//	Test bit 13, 14 and 15
		constexpr bool is_custom_layer(command_t cmd) {
			return ((cmd & 0x7000) == 0x7000);
		}

		/**
		 *	@return <code>true</code> if the specified command is a response.
		 */
		constexpr bool is_response(command_t cmd)	{
			return !is_request(cmd);
		}

		constexpr bool is_transport_layer(command_t cmd)	{
			return (is_custom_layer(cmd))
				? false
				: ((cmd & 0x1000) == 0x1000)
				;
		}

		constexpr bool is_application_layer(command_t cmd) {
			return (is_custom_layer(cmd))
				? false
				: ((cmd & 0x2000) == 0x2000)
				;
		}

		constexpr bool is_control_layer(command_t cmd) {
			return (is_custom_layer(cmd))
				? false
				: ((cmd & 0x4000) == 0x4000)
				;
		}


		/**
		 *	@return parameter count after header data.
		 */
		std::uint16_t arity(command_t);

	}	//	ipt
}

#pragma pack( pop )	//	reset packing alignment

#endif
