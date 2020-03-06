/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "obis_parser.hpp"
#include <smf/sml/obis_db.h>

#include <cyng/util/split.h>

namespace node	
{
	namespace sml
	{

		template struct obis_parser<std::string::const_iterator>;

		std::pair<obis, bool> parse_obis(std::string const& inp)
		{
			obis_parser< std::string::const_iterator >	g;
			std::string::const_iterator first = inp.begin();
			std::string::const_iterator last = inp.end();
			obis result;
			const bool b = boost::spirit::qi::parse(first, last, g, result);
			return std::make_pair(result, b);
		}

		obis_path parse_obis_path(std::string const& path, char sep)
		{
			obis_path op;
			auto const r =  cyng::split(path, std::string(1, sep));
			op.reserve(r.size());
			for (auto const& segment : r) {
				auto const pr = parse_obis(segment);
				if (pr.second) {
					op.push_back(pr.first);
				}
			}
			return op;
		}

		std::string translate_obis_path(std::string const& path, char sep)
		{
			std::string op;
			auto const r = cyng::split(path, std::string(1, sep));
			op.reserve(r.size());
			for (auto const& segment : r) {
				auto const pr = parse_obis(segment);
				if (!op.empty())	op += ':';
				if (pr.second) {
					op.append(get_name(pr.first));
				}
				else {
					op.append(segment);
				}
			}
			return op;
		}

	}	
}	//	namespace node
