/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/sml/obis_io.h>
#include <iomanip>
#include <sstream>
#include <boost/io/ios_state.hpp>

namespace node
{
	namespace sml
	{
		std::ostream& operator<<(std::ostream& os, obis const& v)
		{
			//	store and reset stream state
			boost::io::ios_flags_saver  ifs(os);

			os
				<< std::dec
				<< v.get_medium()
				<< '-'
				<< v.get_channel()
				<< ':'
				<< v.get_indicator()
				<< '.'
				<< v.get_mode()
				<< '.'
				<< v.get_quantities()
				<< '*'
				<< v.get_storage()
				;
			return os;
		}

		std::string to_string(obis const& v)
		{
			std::ostringstream ss;
			ss << v;
			return ss.str();
		}

		std::ostream& to_hex(std::ostream& os, obis const& v)
		{
			//	store and reset stream state
			boost::io::ios_flags_saver  ifs(os);

			os
				<< std::hex
				<< std::setfill('0')
				;

			os
				<< std::setw(2)
				<< v.get_medium()
				<< ' '
				<< std::setw(2)
				<< v.get_channel()
				<< ' '
				<< std::setw(2)
				<< v.get_indicator()
				<< ' '
				<< std::setw(2)
				<< v.get_mode()
				<< ' '
				<< std::setw(2)
				<< v.get_quantities()
				<< ' '
				<< std::setw(2)
				<< v.get_storage()
				;
			return os;
		}

		std::string to_hex(obis const& v)
		{
			std::ostringstream ss;
			to_hex(ss, v);
			return ss.str();
		}

		std::string to_hex(obis_path const& p)
		{
			std::ostringstream ss;
			bool init = false;
			for (auto o : p)
			{
				if (init)
				{
					ss << " => ";
				}
				else
				{
					init = true;
				}
				to_hex(ss, o);
			}
			return ss.str();
		}

	}
}