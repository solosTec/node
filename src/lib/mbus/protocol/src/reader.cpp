#include <smf/mbus/reader.h>

#include <iterator> // for back_inserter 
#include <ctime>

#include <boost/assert.hpp>
#include <boost/predef.h>

namespace smf
{
	namespace mbus
	{
		/**
		 * read a buffer from the specified offset and return the next vif
		 */
		std::size_t read(cyng::buffer_t const& data, std::size_t offset) {
			BOOST_ASSERT(data.size() > offset);
			if (data.size() > offset) {

				//	check dummy bytes
				if (data.at(offset) == static_cast<char>(0x2f))	return data.size();

				//
				//	read dif
				//
				dif const d(data.at(offset));
				++offset;
				BOOST_ASSERT(data.size() > offset);

				if (d.is_extended()) {
					//
					//	read dife
					//
					dife const de(data.at(offset));
					++offset;
					BOOST_ASSERT(data.size() > offset);

				}

				//
				//	read vif
				//
				vif const v(data.at(offset));
				++offset;
				BOOST_ASSERT(data.size() > offset);

				if (v.is_extended()) {
					//
					//	read vife
					//
					++offset;
					BOOST_ASSERT(data.size() > offset);
				}

				auto const size = calculate_size(d.get_data_field_code(), data.at(offset));
				BOOST_ASSERT(data.size() > offset + size);

				if (data_field_code::DFC_VAR == d.get_data_field_code()) {
					++offset;
					BOOST_ASSERT(data.size() > offset + size);
				}

				//
				//	read value
				//
				if (data.size() > offset + size) {

					cyng::buffer_t value;
					value.reserve(size);
					std::copy_n(data.begin() + offset, size, std::back_inserter(value));
					offset += size;
				}


				return offset;
			}
			return data.size();
		}

		std::size_t calculate_size(data_field_code dfc, char c) {

			if (data_field_code::DFC_VAR == dfc) {
				if (c < 0xC0) {
					//	max 192 characters
					return static_cast<std::size_t>(c);
				}
				else if (c < 0xD0) {
					//	BCD_POSITIVE
					return static_cast<std::size_t>((c - 0xC0) * 2);
				}
				else if (c < 0xE0) {
					//	BCD_NEGATIVE_;
					return static_cast<std::size_t>((c - 0xD0) * 2);
				}
				else if (c < 0xF0) {
					//	DATA_BINARY
					return static_cast<std::size_t>(c - 0xE0);
				}
				else if (c < 0xFB) {
					//	DATA_FP
					return static_cast<std::size_t>(8);
				}
				//	reserved
				return static_cast<std::size_t>(c - 0xFB);

			}
			else if (data_field_code::DFC_READOUT == dfc) {
				BOOST_ASSERT_MSG(false, "not implemented");

			}
			else if (data_field_code::DFC_SPECIAL == dfc) {
				BOOST_ASSERT_MSG(false, "not implemented");
			}
			return get_length(dfc);
		}

		std::chrono::system_clock::time_point convert_to_tp(char a, char b, char c, char d) {

			std::tm t;
			t.tm_wday = 0;  // days since Sunday - [0, 6]
			t.tm_yday = 0;  // days since January 1 - [0, 365]
			t.tm_isdst = -1; // daylight savings time flag [-1/0/1]

			t.tm_min = (a & 0x3f);
			t.tm_hour = (b & 0x1f);
			int yearh = (0x60 & b) >> 5;
			t.tm_mday = (c & 0x1f);
			int year1 = (0xe0 & c) >> 5;
			t.tm_mon = (d & 0x0f) + 1;
			int year2 = (0xf0 & d) >> 1;

			if (yearh == 0) {
				yearh = 1;
			}

			//	years since 1900
			t.tm_year = yearh + year1 + year2;
			t.tm_sec = 0;

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
			return std::chrono::system_clock::from_time_t(::_mkgmtime(&t));
#else
			//	nonstandard GNU extension, also present on the BSDs
			return std::chrono::system_clock::from_time_t(::timegm(&t));
#endif
		}

	}
}


