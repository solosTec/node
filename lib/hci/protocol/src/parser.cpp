/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#include <smf/hci/parser.h>

#include <cyng/vm/generator.h>

#include <bitset>
#include <cmath>

#include <boost/algorithm/string.hpp>

namespace node	
{
	namespace hci
	{
		/*
		 * HCI parser
		 */
		parser::parser(vm_callback vm_cb)
			: vm_cb_(vm_cb)
			, code_()
			, stream_state_(state::SOF)
			, stream_buffer_()
			, input_(&stream_buffer_)
			, length_(0u)
			, ep_(0u)
			, crc16_field_(false)
			, rssi_field_(false)
			, time_stamp_field_(false)
			, msg_counter_(0u)
		{}

		parser::~parser()
		{}

		void parser::parse(char c)	
		{
			switch (stream_state_)
			{

			/*
			 *	gathering login data
			 */
			case state::SOF:
				if (c == sof)
				{
					//
					//	0x5A must be the first byte.
					//	Every other value signals an error
					//
					stream_state_ = state::CTRL_EP;
				}
				else 
				{
					stream_state_ = state::INVALID;
					vm_cb_(cyng::generate_invoke("log.msg.error", "invalid SOF", +c));
				}
				break;

			/*
			 *	Control and Enpoint field
			 */
			case state::CTRL_EP:
				stream_state_ = state::MSG_ID;
				{
					std::bitset<8> const bs(c);
					ep_ = c & 0x0F;
					crc16_field_ = bs.test(7);
					rssi_field_ = bs.test(6);
					time_stamp_field_ = bs.test(5);
					vm_cb_(cyng::generate_invoke("log.msg.trace", "HCI EP", ep_));
				}
				break;

			/*
			 *	message ID field
			 */
			case state::MSG_ID:
				vm_cb_(cyng::generate_invoke("log.msg.trace", "HCI MSG ID", static_cast<std::uint8_t>(c)));
				stream_state_ = state::LENGTH;
				break;

			/*
			 *	length field (1 byte)
			 */
			case state::LENGTH:
				length_ = static_cast<std::uint8_t>(c);
				if (length_ != 0u) {
					stream_state_ = state::PAYLOAD;
					//	is part of the payload
					input_.put(c);
				}
				else {
					//	This happens after sending an initilaization string
					if (crc16_field_) {
						length_ = 2;	//	2 bytes
						stream_state_ = state::FCS;
					}
					else {
						stream_state_ = state::SOF;
					}
				}
				break;

			case state::PAYLOAD:
				BOOST_ASSERT(length_ != 0);
				input_.put(c);
				--length_;
				if (length_ == 0) {

					//
					//	payload complete
					//
					vm_cb_(cyng::generate_invoke("hci.payload", cyng::buffer_t(std::istreambuf_iterator<char>(input_), {}), msg_counter_));

					//
					//	update message counter
					//
					++msg_counter_;

					if (time_stamp_field_) {
						stream_state_ = state::TIME_STAMP;
						length_ = 4;	//	4 bytes
					}
					else if (rssi_field_) {
						stream_state_ = state::RSSI;
					}
					else if (crc16_field_) {
						length_ = 2;	//	2 bytes
						stream_state_ = state::FCS;
					}
					else {
						stream_state_ = state::SOF;
					}
				}
				break;

			case state::TIME_STAMP:
				//	4 bytes
				BOOST_ASSERT(length_ != 0u);
				input_.put(c);
				--length_;
				if (length_ == 0) {

					//
					//	timestamp complete
					//
					auto ts = read_numeric<std::uint32_t>();
					vm_cb_(cyng::generate_invoke("log.msg.trace", "HCI timestamp", ts));

					if (rssi_field_) {
						stream_state_ = state::RSSI;
					}
					else if (crc16_field_) {
						length_ = 2;	//	2 bytes
						stream_state_ = state::FCS;
					}
					else {
						stream_state_ = state::SOF;
					}
				}
				break;

			case state::RSSI:
				//	98=-45.3dBm
				//	91=-49.3dBm
				//	81=-57.9dBm	
				//	approximation f(x)=3/4
				vm_cb_(cyng::generate_invoke("log.msg.trace", "HCI RSSI", static_cast<std::uint8_t>(c), (-120.0 + (0.75 * (static_cast<std::uint8_t>(c))))));
				if (crc16_field_) {
					length_ = 2;	//	2 bytes
					stream_state_ = state::FCS;
				}
				else stream_state_ = state::SOF;
				break;

			case state::FCS:
				BOOST_ASSERT(length_ != 0u);
				input_.put(c);
				--length_;
				if (length_ == 0) {
					auto crc = read_numeric<std::uint16_t>();
					vm_cb_(cyng::generate_invoke("log.msg.trace", "HCI CRC", crc));
					stream_state_ = state::SOF;
				}
				break;

			default:
				vm_cb_(cyng::generate_invoke("log.msg.error", "HCI parser in illegal state"));
				stream_state_ = state::SOF;
				break;
			}

		}

		void parser::post_processing()
		{}

	}
}	

