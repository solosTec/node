/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_RADIO_HEADER_H
#define SMF_MBUS_RADIO_HEADER_H

#include <cyng/obj/intrinsics/buffer.h>
#include <smf/mbus/server_id.h>

#include <type_traits>
#include <iostream>
#include <iomanip>

#include <boost/assert.hpp>

namespace smf
{
	namespace mbus	
	{
		/**
		 * Convert a device id into a string.
		 * Example: 684279 => "00684279"
		 */
		std::string dev_id_to_str(std::uint32_t);

		namespace radio
		{
			class parser;
			class header;
			class tpl;

			/**
			 * Takes header data and payload and restores the original (unparsed) message
			 */
			cyng::buffer_t restore_data(header const&, tpl const&, cyng::buffer_t const&);


			/**
			 * [0] - total size (length field excluded)
			 * [1] - packet type (C field)
			 * [2-3] - manufacturer 
			 * [4-7] - meter address
			 * [8] - version 
			 * [9] - medium 
			 * [10] - application type (CI)
			 */
			class header
			{
				friend class parser;
				friend cyng::buffer_t restore_data(header const&, tpl const&, cyng::buffer_t const&);

				using value_type = std::uint8_t;
				using SIZE = std::integral_constant<std::size_t, 11>;
				//	internal data type
				using data_type = std::array< value_type, SIZE::value >;

			public:

				constexpr static std::size_t size() noexcept {
					return SIZE::value;
				}

				header();

				void reset();

				/**
				 * total size
				 */
				constexpr std::uint8_t total_size() const noexcept {
					return data_.at(0);
				}

				/**
				 * payload size = total size - 0x09
				 */
				constexpr std::uint8_t payload_size() const noexcept {
					return static_cast<std::uint8_t>(total_size() - 0x09);
				}

				/**
				 * @return packet type
				 */
				constexpr std::uint8_t get_c_field() const noexcept {
					 return data_.at(1);
				}

				/**
				 * @return 2 byte code
				 */
				std::pair<char, char> get_manufacturer_code() const;

				constexpr std::uint8_t get_version() const noexcept {
					return data_.at(8);
				}

				constexpr std::uint8_t get_medium() const noexcept {
					return data_.at(9);
				}

				std::string get_id() const;
				std::uint32_t get_dev_id() const;

				/**
				 * application type (CI)
				 * <li><0x72, 0x7A: response M-Bus/li>
				 * <li><0x7C, 0x7D: response DLMS/li>
				 * <li><0x7E, 0x7F: response SML/li>
				 */
				constexpr std::uint8_t get_frame_type() const noexcept {
					return data_.at(10);
				}

				/**
				 * [0] 1 byte 01/02 01 == wireless, 02 == wired
				 * [1-2] 2 bytes manufacturer ID
				 * [3-6] 4 bytes serial number
				 * [7] 1 byte device type / media
				 * [8] 1 byte product revision
				 *
				 * 9 bytes in total
				 * example: 01-e61e-13090016-3c-07
				 *
				 */
				constexpr srv_id_t get_server_id() const noexcept {

					return srv_id_t	{
						1,	//	wireless M-Bus

						//	[1-2] 2 bytes manufacturer ID
						data_.at(2),
						data_.at(3),

						//	[3-6] 4 bytes serial number (reversed)
						data_.at(4),
						data_.at(5),
						data_.at(6),
						data_.at(7),

						data_.at(8),
						data_.at(9)
					};
				}

			private:
				data_type	data_;
			};

			enum class security_mode : std::uint8_t {
				NONE = 0, 
				SYMMETRIC = 5, //	AES128 with CBC
				ADVANCED = 7,
				ASSYMETRIC = 13,
			};

			/**
			 * Context data required to decrypt wireless mBus data.
			 * Part of the mBus wireless header.
			 * Same for short and long TPL headers
			 * 
			 * The Access Number together with the transmitter address
			 * is used to identify a datagram.
			 * 
			 * It will be distinguished between "Gateway Status" and "Meter Status" depending from
			 * the CI field.
			 *
			 * Security Mode
			 *
			 * length of encrypted data
			 */
			class tpl 
			{
				friend class parser;
				friend cyng::buffer_t restore_data(header const&, tpl const&, cyng::buffer_t const&);

				using value_type = std::uint8_t;
				using SIZE = std::integral_constant<std::size_t, 4>;
				//	internal data type
				using data_type = std::array< value_type, SIZE::value >;

			public:

				constexpr static std::size_t size() noexcept {
					return SIZE::value;
				}

				tpl();

				void reset();

				constexpr security_mode get_security_mode() const noexcept {
					//	take only the first 5 bits
					switch (data_.at(3) & 0x1F) {
					case 0: return security_mode::NONE;
					case 5:	return security_mode::SYMMETRIC;
					case 7:	return security_mode::ADVANCED;
					case 13: return security_mode::ASSYMETRIC;
					default: break;
					}
					BOOST_ASSERT_MSG(false, "security mode not supported");
					return security_mode::NONE;
				}

				constexpr std::uint8_t get_access_no() const noexcept {
					return data_.at(0);
				}

				constexpr std::uint8_t get_meter_stats() const noexcept {
					return data_.at(1);
				}

				constexpr std::uint8_t get_block_count() const noexcept {
					switch (get_security_mode()) {
					case security_mode::ASSYMETRIC:
						return data_.at(2);
					default:
						break;
					}
					return (data_.at(2) & 0xF0) >> 4;
				}

			private:
				data_type	data_;
			};

		}

		template <typename ch, typename char_traits>
		std::basic_ostream<ch, char_traits>& operator<<(std::basic_ostream<ch, char_traits>& os, radio::header const& h)
		{
			//	"01-e61e-08646300-36-03"
			auto const id = h.get_server_id();
			os
				<< "01-"
				<< std::setfill('0')
				<< std::hex
				<< std::setw(2)
				<< +id.at(1)
				<< std::setw(2)
				<< +id.at(2)
				<< '-'
				<< std::setw(2)
				<< +id.at(3)
				<< std::setw(2)
				<< +id.at(4)
				<< std::setw(2)
				<< +id.at(5)
				<< std::setw(2)
				<< +id.at(6)
				<< '-'
				<< std::setw(2)
				<< +id.at(7)
				<< '-'
				<< std::setw(2)
				<< +id.at(8)
				;
			return os;
		}

		std::string to_str(radio::header const& h);
	}
}


#endif
