/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "writer.hpp"
#include <smf/sml/protocol/value.hpp>

#include <limits>
#include <cyng/io/swap.h>
#include <cyng/io/serializer/binary.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace node
{
	namespace sml
	{
		void write_length_field(std::ostream& os, std::uint32_t length, std::uint8_t type)
		{
			BOOST_ASSERT_MSG((type == 0x70) || (type == 0x00), "invalid type mask");

			if (length > 0xF)
			{
				//
				// maximal count of shifts that are necesarry
				//
				int shifts = (sizeof(std::uint32_t) * 2) - 1;

				//
				//	get upper limit - this is log8()
				//
				std::uint32_t mask = 0xF0000000;
				while (shifts != 0)
				{
					if ((length & mask) != 0)	break;

					mask >>= 4;
					shifts--;
				}

				//std::cout << std::dec << shifts << " shift(s) required, mask = " << std::hex << mask << std::endl;

				while (shifts-- != 0)
				{
					//
					//	get bits for TL field
					//
					std::uint8_t tl = (length & mask);
					if (tl > 0x0F)
					{
						tl >>= 4;
					}
					tl |= 0x80;	//	continuation bit
					//	ToDo: type mask 0x70 == list of..., 0x00 == octet
					tl |= type;
					os.put(tl);

					//std::cout << "write " << std::hex << (+tl) << std::endl;

					mask >>= 4;
					length++;
				}
			}

			//
			//	write last element + type info
			//
			std::uint8_t tl = (length & 0x0000000F);
			tl |= type;
			os.put(tl);
			//std::cout << "write " << std::hex << (+tl) << std::endl;

		}

		std::ostream& serializer <cyng::eod>::write(std::ostream& os, cyng::eod v)
		{
			os.put(0x00);	//	EndOfMessage
			return os;
		}

		std::ostream& serializer <bool> ::write(std::ostream& os, bool v)
		{
			os.put(0x42);	//	TL field
			os.put(v ? std::numeric_limits<char>::max() : 0);	//	anything but zero is true
			return os;
		}

		std::ostream& serializer <std::uint8_t> ::write(std::ostream& os, std::uint8_t v)
		{
			os.put(0x62);	//	TL field
			os.put(v);
			return os;
		}

		std::ostream& serializer <std::uint16_t> ::write(std::ostream& os, std::uint16_t v)
		{
			if (v < std::numeric_limits<std::uint8_t>::max())
			{
				serializer<std::uint8_t>::write(os, boost::numeric_cast<std::uint8_t>(v));
			}
			else
			{
				os.put(0x63);	//	TL field
				cyng::io::write_binary(os, cyng::swap_num(v));
			}
			return os;
		}

		std::ostream& serializer <std::uint32_t> ::write(std::ostream& os, std::uint32_t v)
		{
			if (v < std::numeric_limits<std::uint16_t>::max())
			{
				serializer<std::uint16_t>::write(os, boost::numeric_cast<std::uint16_t>(v));
			}	
			else if (v < std::numeric_limits<std::uint32_t>::max() / 0x100)
			{
				//	only 3 bytes required
				os.put(0x64);	//	TL field
				//	write the first 3 bytes to stream
				cyng::io::write_binary<std::uint32_t, 3>(os, cyng::swap_num(v));
			}
			else
			{
				os.put(0x65);	//	TL field
				cyng::io::write_binary(os, cyng::swap_num(v));
			}
			return os;
		}

		std::ostream& serializer <std::uint64_t> ::write(std::ostream& os, std::uint64_t v)
		{
			if (v < std::numeric_limits<std::uint32_t>::max())
			{
				serializer<std::uint32_t>::write(os, boost::numeric_cast<std::uint32_t>(v));
			}
			else if (v < std::numeric_limits<std::uint64_t>::max() / 0x10000)
			{
				//	only 5 bytes required
				os.put(0x66);	//	TL field
				cyng::io::write_binary<std::uint64_t, 5>(os, cyng::swap_num(v));
			}
			else if (v < std::numeric_limits<std::uint64_t>::max() / 0x1000)
			{
				//	only 6 bytes required
				os.put(0x67);	//	TL field
				cyng::io::write_binary<std::uint64_t, 6>(os, cyng::swap_num(v));
			}
			else if (v < std::numeric_limits<std::uint64_t>::max() / 0x100)
			{
				//	only 7 bytes required
				os.put(0x68);	//	TL field
				cyng::io::write_binary<std::uint64_t, 7>(os, cyng::swap_num(v));
			}
			else
			{
				//	up to 18.446.744.073.709.551.615
				os.put(0x69);	//	TL field
				cyng::io::write_binary(os, cyng::swap_num(v));
			}
			return os;
		}

		std::ostream& serializer <std::int8_t> ::write(std::ostream& os, std::int8_t v)
		{
			os.put(0x52);	//	TL field
			os.put(v);
			return os;
		}

		std::ostream& serializer <std::int16_t> ::write(std::ostream& os, std::int16_t v)
		{
			if ((v > std::numeric_limits<std::int8_t>::min()) && (v < std::numeric_limits<std::int8_t>::max()))
			{
				serializer<std::int8_t>::write(os, boost::numeric_cast<std::int8_t>(v));
			}
			else
			{
				os.put(0x53);	//	TL field
				os.put(cyng::swap_num(v));
			}
			return os;
		}

		std::ostream& serializer <std::int32_t> ::write(std::ostream& os, std::int32_t v)
		{
			if ((v > std::numeric_limits<std::int16_t>::min()) && (v < std::numeric_limits<std::int16_t>::max()))
			{
				serializer<std::int16_t>::write(os, boost::numeric_cast<std::int16_t>(v));
			}
			else if (v > -0xFFFFFF && v < 0xFFFFFF)
			{
				os.put(0x54);	//	TL field
				cyng::io::write_binary<std::int32_t, 3>(os, cyng::swap_num(v));
			}
			else
			{
				os.put(0x55);	//	TL field
				os.put(cyng::swap_num(v));
			}
			return os;
		}

		std::ostream& serializer <std::int64_t> ::write(std::ostream& os, std::int64_t v)
		{
			if ((v > std::numeric_limits<std::int32_t>::min()) && (v < std::numeric_limits<std::int32_t>::max()))
			{
				serializer<std::int32_t>::write(os, boost::numeric_cast<std::int32_t>(v));
			}
			else
			{
				os.put(0x59);	//	TL field
				os.put(cyng::swap_num(v));
			}
			return os;
		}

		std::ostream& serializer <cyng::buffer_t> ::write(std::ostream& os, cyng::buffer_t const& v)
		{
			//	length field
			write_length_field(os, v.size() + 1, 0x00);

			//	data
			os.write(v.data(), v.size());

			return os;
		}

		std::ostream& serializer <std::string> ::write(std::ostream& os, std::string const& v)
		{
			//	length field
			write_length_field(os, v.size() + 1, 0x00);

			//	data
			os.write(v.data(), v.size());

			return os;
		}

		std::ostream& serializer <std::chrono::system_clock::time_point> ::write(std::ostream& os, std::chrono::system_clock::time_point tp)
		{
			//	Cast 64 to 32 bit value to avoid compatibility problems
			//	but get a Y2K38 problem.
			return serializer<cyng::tuple_t>::write(os, make_timestamp(tp));
		}

		std::ostream& serializer <cyng::tuple_t> ::write(std::ostream& os, cyng::tuple_t const& v)
		{
			//	List of
			//const std::uint32_t count = v.size();
			//BOOST_ASSERT(count < 0xF);
			//os.put(0x70 | (0x0F & count));
			//
			//	write TL field for lists
			//
			write_length_field(os, v.size(), 0x70);

			//	serialize each element from the tuple
			for (const auto& obj : v)
			{
				serialize(os, obj);
			}

			return os;
		}

	}
}
