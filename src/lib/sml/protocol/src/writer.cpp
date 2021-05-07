﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/sml/writer.hpp>
#include <smf/sml/value.hpp>

#include <limits>
//#include <cyng/io/swap.h>
#include <cyng/io/serializer/binary.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace cyng {
	namespace io {

		/**
		 * @return length of type length field
		 */
		std::uint32_t get_shift_count(std::uint32_t length)
		{
			return length / 0x0F;
		}

		void write_length_field(std::ostream& os, std::uint32_t length, std::uint8_t type)
		{
			// set the type
			std::uint8_t c = type;

			if (type != 0x70) {
				length++;
			}

			if (length > 0x0F) {

				//	init mask position
				int mask_pos = (sizeof(std::uint32_t) * 2) - 1;

				// the 4 most significant bits of l (1111 0000 0000 ...)
				std::uint32_t mask = 0xF0 << (8 * (sizeof(std::uint32_t) - 1));

				// select the next 4 most significant bits with a bit set until there
				// is something
				while (!(mask & length)) {
					mask >>= 4;
					mask_pos--;
				}

				// TL-field is also counted 
				length += mask_pos; 

				if ((0x0F << (4 * (mask_pos + 1))) & length) {
					// for the rare case that the addition of the number of TL-fields
					// result in another TL-field.
					mask_pos++;
					length++;
				}

				// copy 4 bits of the number to the buffer
				while (mask > 0x0F) {
					c |= 0x80;
					c |= ((mask & length) >> (4 * mask_pos));
					mask >>= 4;
					mask_pos--;
					os.put(c);
					c = 0;
				}
			}

			c |= (length & 0x0F);
			os.put(c);
		}

		std::size_t serializer <eod, SML>::write(std::ostream& os, cyng::eod v)
		{
			os.put(0x00);	//	EndOfMessage
			return 1u;
		}

		std::size_t serializer <bool, SML> ::write(std::ostream& os, bool v)
		{
			calc_size const cs(os);
			os.put(0x42);	//	TL field
			os.put(v ? 0x7f : 0);	//	anything but zero is true
			return cs;
		}

		std::size_t serializer <std::uint8_t, SML> ::write(std::ostream& os, std::uint8_t v)
		{
			calc_size const cs(os);
			os.put(0x62);	//	TL field
			os.put(v);
			return cs;
		}

		std::size_t serializer <std::uint16_t, SML> ::write(std::ostream& os, std::uint16_t v)
		{
			calc_size const cs(os);
			if (v < std::numeric_limits<std::uint8_t>::max())
			{
				serializer<std::uint8_t, SML>::write(os, boost::numeric_cast<std::uint8_t>(v));
			}
			else
			{
				os.put(0x63);	//	TL field
				write_binary(os, v);
			}
			return cs;
		}

		std::size_t serializer <std::uint32_t, SML> ::write(std::ostream& os, std::uint32_t v)
		{
			calc_size const cs(os);
			if (v < std::numeric_limits<std::uint16_t>::max())
			{
				serializer<std::uint16_t, SML>::write(os, boost::numeric_cast<std::uint16_t>(v));
			}	
			else if (v < std::numeric_limits<std::uint32_t>::max() / 0x100)
			{
				//	only 3 bytes required
				os.put(0x64);	//	TL field
				write_binary<std::uint32_t, 3, 1>(os, v);
			}
			else
			{
				os.put(0x65);	//	TL field
				write_binary(os, v);
			}
			return cs;
		}

		std::size_t serializer <std::uint64_t, SML> ::write(std::ostream& os, std::uint64_t v)
		{
			calc_size const cs(os);
			if (v < std::numeric_limits<std::uint32_t>::max())
			{
				serializer<std::uint32_t, SML>::write(os, boost::numeric_cast<std::uint32_t>(v));
			}
			else if (v < std::numeric_limits<std::uint64_t>::max() / 0x10000)
			{
				//	only 5 bytes required
				os.put(0x66);	//	TL field
				write_binary<std::uint64_t, 5, 3>(os, v);

			}
			else if (v < std::numeric_limits<std::uint64_t>::max() / 0x1000)
			{
				//	only 6 bytes required
				os.put(0x67);	//	TL field
				write_binary<std::uint64_t, 6, 2>(os, v);

			}
			else if (v < std::numeric_limits<std::uint64_t>::max() / 0x100)
			{
				//	only 7 bytes required
				os.put(0x68);	//	TL field
				write_binary<std::uint64_t, 7, 1>(os, v);

			}
			else
			{
				//	up to 18.446.744.073.709.551.615
				os.put(0x69);	//	TL field
				write_binary(os, v);
			}
			return cs;
		}

		std::size_t serializer <std::int8_t, SML> ::write(std::ostream& os, std::int8_t v)
		{
			calc_size const cs(os);
			os.put(0x52);	//	TL field
			os.put(v);
			return cs;
		}

		std::size_t serializer <std::int16_t, SML> ::write(std::ostream& os, std::int16_t v)
		{
			calc_size const cs(os);
			if ((v > std::numeric_limits<std::int8_t>::min()) && (v < std::numeric_limits<std::int8_t>::max()))
			{
				serializer<std::int8_t, SML>::write(os, boost::numeric_cast<std::int8_t>(v));
			}
			else
			{
				os.put(0x53);	//	TL field
				write_binary<std::int16_t, 2, 0>(os, v);
			}
			return cs;
		}

		std::size_t serializer <std::int32_t, SML> ::write(std::ostream& os, std::int32_t v)
		{
			calc_size const cs(os);
			if ((v > std::numeric_limits<std::int16_t>::min()) && (v < std::numeric_limits<std::int16_t>::max()))
			{
				serializer<std::int16_t, SML>::write(os, boost::numeric_cast<std::int16_t>(v));
			}
			else if (v > -0xFFFFFF && v < 0xFFFFFF)
			{
				os.put(0x54);	//	TL field
				write_binary<std::int32_t, 3, 1>(os, v);
			}
			else
			{
				os.put(0x55);	//	TL field
				write_binary<std::int32_t, 4, 0>(os, v);
			}
			return cs;
		}

		std::size_t serializer <std::int64_t, SML> ::write(std::ostream& os, std::int64_t v)
		{
			calc_size const cs(os);
			if ((v > std::numeric_limits<std::int32_t>::min()) && (v < std::numeric_limits<std::int32_t>::max()))
			{
				serializer<std::int32_t, SML>::write(os, boost::numeric_cast<std::int32_t>(v));
			}
			else
			{
				os.put(0x59);	//	TL field
				write_binary<std::int64_t, 8, 0>(os, v);
			}
			return cs;
		}

		std::size_t serializer <cyng::buffer_t, SML> ::write(std::ostream& os, cyng::buffer_t const& v)
		{
			calc_size const cs(os);
			//	length field
			write_length_field(os, static_cast<std::uint32_t>(v.size()), 0x00);

			//	data
			os.write(v.data(), v.size());

			return cs;
		}

		std::size_t serializer <std::string, SML> ::write(std::ostream& os, std::string const& v)
		{
			calc_size const cs(os);
			//	length field
			write_length_field(os, static_cast<std::uint32_t>(v.size()), 0x00);

			//	data
			os.write(v.data(), v.size());

			return cs;
		}

		std::size_t serializer <std::chrono::system_clock::time_point, SML> ::write(std::ostream& os, std::chrono::system_clock::time_point tp)
		{
			//	Cast 64 to 32 bit value to avoid compatibility problems
			//	but get a Y2K38 problem.
			return serializer<cyng::tuple_t, SML>::write(os, smf::sml::make_timestamp(tp));
		}

		std::size_t serializer <cyng::tuple_t, SML> ::write(std::ostream& os, cyng::tuple_t const& v)
		{
			calc_size const cs(os);

			//
			//	List of
			//
			write_length_field(os, static_cast<std::uint32_t>(v.size()), 0x70);

			//	serialize each element from the tuple
			for (const auto& obj : v)	{
				BOOST_ASSERT_MSG(false, "not implemented");
				//serialize(os, obj);
			}

			return cs;
		}

	}
}