/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/units.h>
#include <boost/assert.hpp>

namespace node
{
	namespace sml
	{
		obis::obis()
			: value_{0}
		{
			//value_.fill(0);
		}

		obis::obis(octet_type const& buffer)
		: value_()
		{
			BOOST_ASSERT(buffer.size() == size());
			if (buffer.size() == size())
			{
				std::copy(buffer.begin()
					, buffer.end()
					, value_.begin());
			}
		}

		obis::obis(obis const& other)
			: value_(other.value_)
		{}

		obis::obis(data_type const& vec)
			: value_(vec)
		{}

		obis::obis(std::uint8_t a
			, std::uint8_t b
			, std::uint8_t c
			, std::uint8_t d
			, std::uint8_t e
			, std::uint8_t f)
		{
			value_[0] = a;
			value_[1] = b;
			value_[2] = c;
			value_[3] = d;
			value_[4] = e;
			value_[5] = f;
		}

		void obis::swap(obis& other)	
		{
			value_.swap(other.value_);
		}

		void obis::clear()
		{
			*this = obis();
		}

		bool obis::equal(obis const& other) const	
		{
			return 	value_[0] == other.value_[0]
				&& value_[1] == other.value_[1]
				&& value_[2] == other.value_[2]
				&& value_[3] == other.value_[3]
				&& value_[4] == other.value_[4]
				&& value_[5] == other.value_[5]
				;

			//return std::equal(value_.begin(), value_.end(), other.value_.begin());
		}

		bool obis::less(obis const& other) const	
		{
			return std::lexicographical_compare(value_.begin()
				, value_.end()
				, other.value_.begin()
				, other.value_.end());
		}

		bool obis::is_matching(obis const& other) const
		{
			return this->equal(other);
		}

		bool obis::is_matching(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d) const
		{
			return (value_[0] == a
				&& value_[1] == b
				&& value_[2] == c
				&& value_[3] == d);
		}

		std::pair<std::uint8_t, bool> obis::is_matching(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d, std::uint8_t e) const
		{
			return std::make_pair(value_[5], is_matching(a, b, c, d) && (value_[4] == e));
		}

		bool obis::match(std::initializer_list<std::uint8_t> il) const
		{
			return (il.size() > size())
				? false
				: std::mismatch(il.begin(), il.end(), value_.begin()).first == il.end()
				;
		}

		obis& obis::operator=(obis const& other)	
		{
			if (this != &other)	
			{
				value_ = other.value_;
			}
			return *this;
		}

		std::uint32_t obis::get_medium() const	
		{
			return value_[0];
		}

		const char* obis::get_medium_name() const 
		{
			switch (get_medium())
			{
				case 0:	return "Abstract objects";
				case 1:	return "Electricity";
				case 4:	return "Heat Cost Allocators";
				case 5:	return "Cooling";
				case 6:	return "Heat";
				case 7:	return "Gas";
				case 8:	return "Water (cold)";
				case 9:	return "Water (hot)";
				case 16: return "Oil";
				case 17: return "Compressed air";
				case 18: return "Nitrogen";
				default:
				break;
			}
			return "reserved";
		}

		bool obis::is_abstract() const	
		{
			return get_medium() == 0;
		}


		std::uint32_t obis::get_channel() const
		{
				return value_[1];
		}

		const char* obis::get_channel_name() const
		{
			if (get_channel() < 128)
			{
				return "Device individual channel";
			}

			switch (get_channel())
			{
				case 128:	return "Savings";
				case 129:	return "Setvalue";
				case 130:	return "Prediction";
				case 131:	return "Working day max";
				case 132:	return "Holiday max.";
				case 133:	return "Working day min.";
				case 134:	return "Holiday min.";
				case 135:	return "SLP Monday";
				case 136:	return "SLP Tuesday";
				case 137:	return "SLP Wednesday";
				case 138:	return "SLP Thursday";
				case 139:	return "SLP Friday";
				case 140:	return "SLP Saturday";
				case 141:	return "SLP Working day";
				case 142:	return "SLP Holiday";
				case 143:	return "Currency";
				case 144:	return "Currency prediction";
				case 145:	return "CO2";
				case 146:	return "CO2 Prediction";
				default:
					break;
			}
			return "reserved";
		}

		std::uint32_t obis::get_indicator() const 
		{
			return value_[2];
		}

		std::uint32_t obis::get_mode() const 
		{
			return value_[3];
		}

		std::uint32_t obis::get_quantities() const 
		{
			return value_[4];
		}

		std::uint32_t obis::get_storage() const 
		{
			return value_[5];
		}

		bool obis::is_private() const 
		{
			return 	(get_medium() >= 0x80 && get_medium() <= 0xC7)
				|| (get_indicator() >= 0x80 && get_indicator() <= 0xC7)
				|| (get_indicator() == 0xF0)
				|| (get_mode() >= 0x80 && get_mode() <= 0xFE)
				|| (get_quantities() >= 0x80 && get_quantities() <= 0xFE)
				|| (get_storage() >= 0x80 && get_storage() <= 0xFE)
				;
		}

		bool obis::is_nil() const
		{
			for (const auto c : value_)
			{
				if (c != 0U)
				{
					return false;
				}
			}
			return true;
		}

		bool obis::is_physical_unit() const
		{
			return get_medium() < UNIT_RESERVED;
		}

		octet_type obis::to_buffer() const
		{
			return octet_type(value_.begin(), value_.end());
		}

		//
		//	global operations
		//
		
		bool operator==(const obis& x, const obis& y)	
		{
			return x.equal(y);
		}
		
		bool operator!= (const obis& x, const obis& y)
		{
			return !x.equal(y);
		}

		bool operator< (const obis& lhs, const obis& rhs)
		{
			return lhs.less(rhs);
		}

		bool operator> (const obis& lhs, const obis& rhs)
		{
			return rhs.less(lhs);
		}

		bool operator<= (const obis& lhs, const obis& rhs)
		{
			return !(rhs.less(lhs));
		}

		bool operator>= (const obis& lhs, const obis& rhs)
		{
			return !(lhs.less(rhs));
		}

		void swap(obis& a, obis& b) {
			a.swap(b);
		}

	}	//	sml
}	//	node



