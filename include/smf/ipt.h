/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_H
#define SMF_IPT_H

#include <cstdint>
#include <type_traits>

namespace smf {
	namespace ipt {

		//	special values
		constexpr std::uint8_t ESCAPE_SIGN = 0x1b;	//!<	27dec
		constexpr std::size_t HEADER_SIZE = 8;		//!<	header size in bytes

		//	define some standard sizes
		constexpr std::size_t NAME_LENGTH = 62;
		constexpr std::size_t NUMBER_LENGTH = 79;

		//	original limit was 30 bytes
		constexpr std::size_t PASSWORD_LENGTH = 36;
		constexpr std::size_t ADDRESS_LENGTH = 255;
		constexpr std::size_t TARGET_LENGTH = 255;
		constexpr std::size_t DESCRIPTION_LENGTH = 255;

		//	original limit of 19 bytes was exeeded by many implementations 
		//	i.g. "MUC-DSL-1.311_13220000__X022a_GWF3"
		constexpr std::size_t VERSION_LENGTH = 64;

		//enum { DEVICE_ID_LENGTH		= 19 };
		constexpr std::size_t DEVICE_ID_LENGTH = 64;	//	see VERSION_LENGTH
		constexpr std::size_t STATUS_LENGTH = 16;

		using sequence_t = std::uint8_t;		//!<	sequence (1 - 255)
		using response_t = std::uint8_t;		//!<	more readable
		using command_t = std::uint16_t;		//!<	part of IPT header definition

		constexpr sequence_t INVALID_SEQ = 0;	//!< valid values > 0
		/**
		 * value is interpreted in minutes.
		 * value 0 is no watchdog.
		 */
		constexpr std::uint16_t NO_WATCHDOG = 0;

	}
}

#endif
