/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HCI_PARSER_H
#define NODE_HCI_PARSER_H

#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/buffer.h>
//#include <cyng/intrinsics/version.h>
#include <boost/asio.hpp>
//#include <boost/variant.hpp>

#include <chrono>
#include <boost/assert.hpp>

namespace node	
{
	namespace hci
	{

		/** @brief HCI/CP210x protocol parser
		 *	
		 * HCI is a wrapper protocol for the WM-Bus module used by 
		 * iM871A USB stick.
		 * see: https://www.wireless-solutions.de/downloads/Gateways-and-USB-Adapter/iM871A-usb/WMBus_HCI_Spec_V1_6.pdf
		 */
		class parser
		{
		public:
			using vm_callback = std::function<void(cyng::vector_t&&)>;

		private:
			/**
			 *	Each session starts in login mode. The incoming
			 *	data are expected to be an iMEGA login command. After a successful
			 *	login, parser switch to stream mode.
			 */
			enum class state
			{
				INVALID,				//!>	error
				SOF,					//!<	SOF Field (Start Of Frame = 0xA5)
				CTRL_EP,				//!<	Control and Enpoint field
				MSG_ID,					//!<	message ID field
				LENGTH,					//!<	length in bytes
				PAYLOAD,				//!<	wireless M-Bus data
				TIME_STAMP,				//!<	optional
				RSSI,					//!<	optional
				FCS,					//!<	optional Frame Check Sequence (16 bit CCR-CCITT)

				STATE_LAST,
			};

			//	doesn't work as expected
			union CTRL_EP {
				char raw_;
				std::uint8_t 
					msg_id : 4,
					reserved : 1,
					time_stamp_field : 1,
					rssi_field : 1,
					crc16_field : 1
					;
			};

		public:
			parser(vm_callback);
			virtual ~parser();

			template < typename I >
			auto read(I start, I end) -> typename std::iterator_traits<I>::difference_type
			{
				//	loop over input buffer
				std::for_each(start, end, [this](const char c)	
				{
					this->parse(c);
				});

				post_processing();
				input_.clear();
				return std::distance(start, end);
			}

		private:
			/**
			 *	Parse data
			 */
			void parse(char);

			/**
			 * Probe if parsing is completed and
			 * inform listener.
			 */
			void post_processing();


		private:
			/**
			 * call this method if parsing is complete
			 */
			vm_callback		vm_cb_;

			/**
			 * instruction buffer
			 */
			cyng::vector_t	code_;

			/*
			 * internal state
			 */
			state stream_state_;

			/**
			 *	input stream buffer
			 */
			boost::asio::streambuf	stream_buffer_;
			std::iostream			input_;

			/**
			 * payload length
			 */
			std::uint8_t length_;

			std::uint8_t msg_id_;
			bool crc16_field_;
			bool rssi_field_;
			bool time_stamp_field_;

			static const char sof = (char)0xA5;	//	0xA5 == -91;
		};
	}
}	

#endif	
