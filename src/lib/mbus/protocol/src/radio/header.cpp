/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
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

			std::pair<char, char> header::get_manufacturer_code() const
			{
				return { 
					static_cast<char>(data_.at(2)),
					static_cast<char>(data_.at(3))
				};
			}

			std::string header::get_id() const {
				std::uint32_t id{ 0 };

				//
				//	get the device ID as u32 value
				//
				std::memcpy(&id, &data_.at(4), sizeof(id));

				//
				//	read this value as a hex value
				//
				std::stringstream ss;
				ss.fill('0');
				ss
					<< std::setw(8)
					<< std::setbase(16)
					<< id;

				return ss.str();
			}

			std::uint32_t header::get_dev_id() const
			{
				std::uint32_t id{ 0 };
				std::stringstream ss(get_id());
				ss >> id;
				return id;
			}

			void header::reset()
			{
				data_.fill(0);
			}

			cyng::buffer_t restore_data(header const& h, tpl const& t, cyng::buffer_t const& payload) {
				cyng::buffer_t res;
				res.reserve(h.total_size() + 1);
				res.insert(res.end(), h.data_.begin(), h.data_.end());	//	header
				res.insert(res.end(), t.data_.begin(), t.data_.end());	//	tpl
				res.insert(res.end(), payload.begin(), payload.end());		//	payload
				return res;
			}

			tpl::tpl()
				: data_{ 0 }
			{}

			void tpl::reset() {
				data_.fill(0);
			}

		}

		std::string dev_id_to_str(std::uint32_t id) {
			std::stringstream ss;
			ss.fill('0');
			ss
				<< std::setw(8)
				<< id
				;
			return ss.str();
		}

		std::string to_str(radio::header const& h) {
			std::stringstream ss;
			ss << h;
			return ss.str();
		}


	}
}


