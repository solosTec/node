/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#include <smf/hci/parser.h>
#include <cyng/vm/generator.h>
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
			, msg_id_(0u)
			, crc16_field_(false)
			, rssi_field_(false)
			, time_stamp_field_(false)
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
					CTRL_EP field;
					field.raw_= c;
					msg_id_ = field.msg_id;
					crc16_field_ = field.crc16_field;
					rssi_field_ = field.rssi_field;
					time_stamp_field_ = field.time_stamp_field;
				}
				break;

			/*
			 *	message ID field
			 */
			case state::MSG_ID:
				stream_state_ = state::LENGTH;
				break;

			/*
			 *	watchdog mode <ALIVE>
			 */
			case state::LENGTH:
				length_ = static_cast<std::uint8_t>(c);
				BOOST_ASSERT(length_ != 0u);
				stream_state_ = (length_ != 0u)
					? state::PAYLOAD
					: state::SOF
					;
				break;

			case state::PAYLOAD:
				input_.put(c);
				--length_;
				if (length_ == 0) {

					//
					//	payload complete
					//
					//data_cb_(cyng::buffer_t(std::istreambuf_iterator<char>(input_), {}));
					vm_cb_(cyng::generate_invoke("hci.payload", cyng::tuple_factory(cyng::buffer_t(std::istreambuf_iterator<char>(input_), {}))));

					if (time_stamp_field_) {
						stream_state_ = state::TIME_STAMP;
						length_ = 4;	//	4 bytes
					}
					else if (rssi_field_)	stream_state_ = state::RSSI;
					else if (crc16_field_)	stream_state_ = state::FCS;
					else stream_state_ = state::SOF;
				}
				break;

			case state::TIME_STAMP:
				//	4 bytes
				--length_;
				if (length_ == 0) {

					//
					//	timestamp complete
					//

					if (rssi_field_)	stream_state_ = state::RSSI;
					else if (crc16_field_)	stream_state_ = state::FCS;
					else stream_state_ = state::SOF;
				}
				break;

			case state::RSSI:
				vm_cb_(cyng::generate_invoke("log.msg.trace", "HCI RSSI", static_cast<std::uint8_t>(c)));
				if (crc16_field_)	stream_state_ = state::FCS;
				else stream_state_ = state::SOF;
				break;

			case state::FCS:
				stream_state_ = state::SOF;
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

