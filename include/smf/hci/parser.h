/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HCI_PARSER_H
#define SMF_HCI_PARSER_H

#include <smf/hci/msg.h>

#include <cyng/obj/intrinsics/buffer.h>

#include <functional>
#include <iterator>
#include <type_traits>
#include <algorithm>

namespace smf
{
	namespace hci
	{
		/** @brief HCI/CP210x protocol parser
		 *
		 * HCI is a wrapper protocol for the WM-Bus module used by
		 * iM871A USB stick.
		 * see: https://shop.imst.de/media/pdf/1b/e2/64/WMBus_HCI_Spec_V1_6.pdf
		 */
		class parser
		{
		public:
			using cb_t = std::function<void(msg const&)>;

		private:
			enum class state
			{
				SOF,					//!<	SOF Field (Start Of Frame = 0xA5)
				HEADER_CTRL_EP,			//!<	Control and Endpoint field
				HEADER_MSG_ID,			//!<	message ID field
				HEADER_LENGTH,			//!<	length in bytes
				PAYLOAD,				//!<	wireless M-Bus data
				TIME_STAMP,				//!<	optional
				RSSI,					//!<	optional
				FCS,					//!<	optional Frame Check Sequence (16 bit CCR-CCITT)

				STATE_LAST,
			} state_;

		public:
			parser(cb_t);

			/**
			 * parse the specified range
			 */
			template < typename I >
			void read(I start, I end)
			{
				static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");
				std::for_each(start, end, [this](char c)
					{
						//
						//	parse
						//
						this->put(c);
					});

			}

		private:

			/**
			 * read a single byte and update
			 * parser state.
			 * Implements the state machine
			 */
			void put(char);

			/**
			 * 0x5A must be the first byte.
			 * Every other value signals an error.
			 */
			state state_sof(char);
			state state_header_ctl_ep(char);
			state state_header_msg_id(char);
			state state_header_length(char);
			state state_payload(char);
			state state_timestamp(char);
			state state_rssi(char);
			state state_fcs(char);

		private:
			cb_t cb_;
			msg msg_;
			constexpr static char sof_c = static_cast<char>(0xA5);	//	0xA5 == -91;

		};
	}
}


#endif
