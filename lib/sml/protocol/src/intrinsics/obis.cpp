/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/intrinsics/obis.h>
#include <smf/mbus/units.h>

#include <cyng/io/io_buffer.h>

#include <algorithm>

#include <boost/assert.hpp>
#include <boost/functional/hash.hpp>

#pragma pack(1)

namespace node
{
	namespace sml
	{
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
			return value_.at(VG_MEDIUM);
		}

		const char* obis::get_medium_name() const 
		{
			switch (get_medium())
			{
				case 0:	return "Abstract objects";
				case 1:	return "Electricity";
					//	HCA
				case 4:	return "Heat Cost Allocators";	

					//	termal energy related objects
				case 5:	return "Cooling";
				case 6:	return "Heat";

				case 7:	return "Gas";
				case 8:	return "Water (cold)";
				case 9:	return "Water (hot)";
				case 15: return "other";
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
				return value_.at(VG_CHANNEL);
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
			return value_.at(VG_INDICATOR);
		}

		const char* obis::get_indicator_name() const
		{
			switch (get_medium()) {
			case 0:	return get_indicator_name_abstract();
			case 1:	return get_indicator_name_electricity();
			case 4:	return get_indicator_name_hca();	
			case 5:	return "Cooling";
			case 6:	return "Heat";
			case 7:	return get_indicator_name_gas();
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

		const char* obis::get_indicator_name_abstract() const
		{
			switch (get_indicator()) {
			case 0:	return "General purpose COSEM objects";
			case 1: return "Instances of IC Clock";
			case 2: return "Instances of IC 'Modem configuration' and related IC-s";
			case 10: return "Instances of IC Script table";
			case 11: return "Instances of IC Special days table";
			case 12: return "Instances of IC Schedule";
			case 13: return "Instances of IC Activity calendar";
			case 14: return "Instances of IC Register activation";
			case 15: return "Instances of IC Single action schedule";
			case 16: return "Instances of IC 'Register monitor', 'Parameter monitor'";
			case 17: return "Instances of IC Limiter";
			case 20: return "Instances of IC IEC local port setup";
			case 21: return "Standard readout definitions";
			case 22: return "Instances of IC IEC HDLC setup";
			case 23: return "Instances of IC IEC twisted pair (1) setup";
			case 24: return "COSEM objects related to M-Bus";
			case 25: return "Instances of IC 'TCP-UDP setup', 'IPv4 setup', 'IPv6 setup', 'MAC address setup', 'PPP setup', 'GPRS modem setup', 'SMTP setup', 'GSM diagnostic', 'FTP setup', 'Push setup'";
			case 26: return "COSEM objects for data exchange using S - FSK PLC";
			case 27: return "COSEM objects for ISO/IEC 8802 - 2 LLC layer setup";
			case 28: return "COSEM objects for data exchange using narrow - band OFDM PLC for PRIME networks";
			case 29: return "COSEM objects for data exchange using narrow - band OFDM PLC for G3-PLC networks";
			case 30: return "COSEM objects for data exchange using ZigBee®";
			case 31: return "Instances of IC 'Wireless Mode Q'";
			case 40: return "Instances of IC 'Association SN/LN'";
			case 41: return "Instances of IC 'SAP assignment'";
			case 42: return "COSEM logical device name";
			case 43: return "Instances of IC 'Security setup'";
			case 44: return "Instances of IC 'Image transfer'";
			case 65: return "Instances of IC 'Utility tables'";
			case 67: return "Instances of 'Sensor manager'";
			default:
				break;
			}
			return "reserved (abstract)";
		}

		const char* obis::get_indicator_name_electricity() const
		{
			switch (get_indicator()) {
			case 1:	return "Active power+ (QI+QIV)";
			case 2:	return "Active power- (QII+QIII)";
			case 3:	return "Reactive power+ (QI+QII)";
			case 4:	return "Reactive power- (QIII+QIV)";
			case 5:	return "Reactive power QI";
			case 6:	return "Reactive power QII";
			case 7:	return "Reactive power QIII";
			case 8:	return "Reactive power QVI";

			case 100: return "Reactive Power inductive (QI+QIII)";
			case 104: return "Reactive Power capacitiv (QI+QIII)";
			default:
				break;
			}
			return "reserved (electricity)";
		}

		const char* obis::get_indicator_name_hca() const
		{
			switch (get_indicator()) {
			case 0:	return "General purpose obejcts";
			case 1:	return "Unrated integral";
			case 2:	return "Rated integral";
			case 3:	return "Radiator surface temperature";
			case 4:	return "Heating medium temperature";
			case 5:	return "Flow (forward) temperature";
			case 6:	return "Return remperature";
			case 7:	return "Room temmerature";

			case 93: return "Consortia specific indentifiers";
			case 94: return "Country specific indentifiers";
			default:
				break;
			}
			return "reserved (HCA)";
		}

		const char* obis::get_indicator_name_gas() const
		{
			switch (get_indicator()) {
			case 0:	return "General purpose objects";
			case 1: return "Forward undisturbed meter volume";
			case 2: return "Forward disturbed meter volume";
			case 3: return "Forward absolute meter volume";
			case 4: return "ToDo";
			case 5: return "ToDo";
			case 6: return "ToDo";
			case 7: return "ToDo";
			case 8: return "ToDo";

			case 11: return "ToDo";
			case 12: return "ToDo";
			case 13: return "ToDo";
			case 14: return "ToDo";
			case 15: return "ToDo";
			case 16: return "ToDo";

			case 21: return "ToDo";
			case 22: return "ToDo";
			case 23: return "ToDo";
			case 24: return "ToDo";
			case 25: return "ToDo";
			case 26: return "ToDo";
			default:
				break;
			}
			return "reserved (gas)";
		}

		std::uint32_t obis::get_mode() const 
		{
			return value_.at(VG_MODE);
		}

		std::uint32_t obis::get_quantities() const 
		{
			return value_.at(VG_QUANTITY);
		}

		std::uint32_t obis::get_storage() const 
		{
			return value_.at(VG_STORAGE);
		}

		std::uint16_t obis::get_number() const
		{
			union {
				std::uint8_t val_[2];
				std::uint16_t num_;
			} u;

			//
			//	map values
			//
			u.val_[0] = value_[VG_STORAGE];
			u.val_[1] = value_[VG_QUANTITY] - 1;

			return u.num_;
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
			return std::all_of(value_.begin(), value_.end(), [](std::uint8_t c) {
				return c == 0u;
			});
			//for (const auto c : value_)
			//{
			//	if (c != 0U)
			//	{
			//		return false;
			//	}
			//}
			//return true;
		}

		bool obis::is_physical_unit() const
		{
			return (get_medium() < static_cast<std::uint8_t>(node::mbus::units::UNIT_RESERVED)) && (get_medium() != 0);
		}

		octet_type obis::to_buffer() const
		{
			return octet_type(value_.begin(), value_.end());
		}

		std::string obis::to_str() const
		{
			std::string s;
			s.reserve(6);
			for (char const c : value_) {
				s += cyng::io::to_hex(c);
			}
			return s;
		}

		std::size_t obis::hash() const
		{
			std::size_t seed = 0;
			for (auto c : value_) {
				boost::hash_combine(seed, c);
			}
			return seed;
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

		obis make_obis(std::uint32_t a
			, std::uint32_t b
			, std::uint32_t c
			, std::uint32_t d
			, std::uint32_t e
			, std::uint32_t f)
		{
			return obis(static_cast<std::uint8_t>(a & 0xFF)
				, static_cast<std::uint8_t>(b & 0xFF)
				, static_cast<std::uint8_t>(c & 0xFF)
				, static_cast<std::uint8_t>(d & 0xFF)
				, static_cast<std::uint8_t>(e & 0xFF)
				, static_cast<std::uint8_t>(f & 0xFF));
		}

	}	//	sml
}	//	node

#pragma pack()

namespace std
{
	size_t hash<node::sml::obis>::operator()(node::sml::obis const& code) const noexcept
	{
		return code.hash();
	}

	bool equal_to<node::sml::obis>::operator()(node::sml::obis const& c1, node::sml::obis const& c2) const noexcept
	{
		return c1.equal(c2);
	}

	bool less<node::sml::obis>::operator()(node::sml::obis const& c1, node::sml::obis const& c2) const noexcept
	{
		return c1.less(c2);
	}

}
