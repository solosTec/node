/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/sml/obis_io.h>
#include <iomanip>
#include <sstream>
#include <cyng/util/split.h>
#include <cyng/parser/buffer_parser.h>

#include <boost/io/ios_state.hpp>
#include <boost/algorithm/string.hpp>

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

		obis to_obis(std::string const& str)
		{
			//
			//	heuristic approach
			//
			const auto size = str.size();
			auto num_dots = std::count(str.begin(), str.end(), '.');
			auto num_dashes = std::count(str.begin(), str.end(), '-');
			auto num_colons = std::count(str.begin(), str.end(), ':');
			auto num_spaces = std::count(str.begin(), str.end(), ' ');
			if (size > 8 && num_dots == 2 && num_dashes == 1 && num_colons == 1 && num_spaces == 0) {
				//
				//	decimal representation
				//	example: 1-1:1.8.1
				//
				const auto res = cyng::split(str, ".-:");
				if (res.size() == 6) {
					try {
						return obis(std::stoi(res.at(0))
							, std::stoi(res.at(1))
							, std::stoi(res.at(2))
							, std::stoi(res.at(3))
							, std::stoi(res.at(4))
							, std::stoi(res.at(5)));
					}
					catch (std::exception const&) {

					}
				}
			}
			else if (size == 12 && num_dots == 0 && num_dashes == 0 && num_colons == 0 && num_spaces == 0) {
				//
				//	hex representation
				//	example: 0100010800ff
				//
				const auto buffer = cyng::parse_hex_string(str);
				if (buffer.second && buffer.first.size() == 6) {
					return obis(buffer.first.at(0)
						, buffer.first.at(1)
						, buffer.first.at(2)
						, buffer.first.at(3)
						, buffer.first.at(4)
						, buffer.first.at(5));
				}
			}
			else if (size == 17 && num_dots == 0 && num_dashes == 0 && num_colons == 0 && num_spaces == 5) {
				//
				//	hex representation
				//	example: 01 00 01 08 00 ff
				//
				const auto res = cyng::split(str, " ");
				if (res.size() == 6) {
					const auto buffer = cyng::parse_log_string(str);
					if (buffer.second && buffer.first.size() == 6) {
						return obis(buffer.first.at(0)
							, buffer.first.at(1)
							, buffer.first.at(2)
							, buffer.first.at(3)
							, buffer.first.at(4)
							, buffer.first.at(5));
					}
				}
			}

			return obis();
		}

		std::ostream& to_hex(std::ostream& os, obis_path const& path)
		{
			bool initialized{ false };
			for (auto const& code : path) {
				if (initialized) {
					os << ' ';
				}
				else {
					initialized = true;
				}
				os << code.to_str();
			}
			return os;
		}

		std::string to_hex(obis_path const& path)
		{
			std::stringstream ss;
			to_hex(ss, path);
			return ss.str();
		}

		obis_path to_obis_path(std::string const& path)
		{
			obis_path result;
			std::vector<std::string> parts;
			boost::split(parts, path, boost::is_any_of("\t\n "));
			for (auto const& code : parts) {
				result.push_back(to_obis(code));
			}
			return result;
		}
	}
}