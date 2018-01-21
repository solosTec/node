/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_HEADER_H
#define NODE_IPT_HEADER_H

#include <smf/ipt/defs.h>
#include <iterator>
#include <boost/assert.hpp>

//	set packing alignment to 1 (dense)
#pragma pack( push, 1 )

namespace node
{
	namespace ipt	
	{
		/**
		 *	header
		 */
		struct header
		{
			command_type		command_;
			sequence_type		sequence_;
			std::uint8_t		reserved_;
			std::uint32_t		length_;

			//header();
			void reset(sequence_type = 0);
		};

		/*
		 *	header_overlay
		 */
		template < typename I >
		header convert_to_header(I start, I end)
		{
			static_assert(HEADER_SIZE == sizeof(header), "alignment error");
			union
			{
				char	raw_[HEADER_SIZE];
				header	data_;
			} overlay_;
			BOOST_ASSERT_MSG(std::distance(start, end) == HEADER_SIZE, "invalid range");
			std::copy(start, end, std::begin(overlay_.raw_));
			return overlay_.data_;
		}

		/**
		 *	@return message size in bytes without header
		 */
		std::size_t size(header const&);

		/**
		 *	Test bit 15.
		 *
		 *	@return <code>true</code> if the specified command is a request.
		 */
		bool is_request(command_type);

		/**
		 *	@return <code>true</code> if the specified command is a response.
		 */
		bool is_response(command_type);

		bool is_transport_layer(command_type);
		bool is_application_layer(command_type);
		bool is_control_layer(command_type);
		bool is_custom_layer(command_type);

		/**
		 *	@return parameter count after header data.
		 */
		std::uint16_t arity(command_type);

	}	//	ipt
}	//	node

#pragma pack( pop )	//	reset packing alignment

#endif	//	NODE_IPT_HEADER_H
