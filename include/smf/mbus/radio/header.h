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

			/**
			 * Takes header data and payload and restores the original (unparsed) message
			 */
			cyng::buffer_t restore_data(header const&, cyng::buffer_t const&);


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
				friend cyng::buffer_t restore_data(header const&, cyng::buffer_t const&);

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
				 * payload size = total size - 0x0A
				 */
				constexpr std::uint8_t payload_size() const noexcept {
					return static_cast<std::uint8_t>(total_size() - 0x0A);
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

		}

	}
}


#endif
