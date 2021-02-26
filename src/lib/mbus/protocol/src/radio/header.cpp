/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/radio/header.h>

#include <iomanip>
#include <sstream>
#include <cstring>

#include <boost/assert.hpp>

namespace smf
{
	namespace mbus
	{
		namespace radio
		{
			header::header()
				: data_{ 0 }
			{}

			cyng::buffer_t header::get_manufacturer_code() const
			{
				return { 
					static_cast<cyng::buffer_t::value_type>(data_.at(2)),
					static_cast<cyng::buffer_t::value_type>(data_.at(3)) 
				};
			}

			std::uint32_t header::get_dev_id() const
			{
				std::uint32_t id{ 0 };

				//
				//	get the device ID as u32 value
				//
				std::uint32_t raw{ 0 };
				std::memcpy(&raw, &data_.at(4), sizeof(id));

				//
				//	read this value as a hex value
				//
				std::stringstream ss;
				ss.fill('0');
				ss
					<< std::setw(8)
					<< std::setbase(16)
					<< raw;

				//
				//	write this value as decimal value
				//
				ss
					>> std::setbase(10)
					>> id
					;

				return id;
			}

			void header::reset()
			{
				data_.fill(0);
			}
		}
	}
}


