/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <parser/obis_parser.hpp>
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

		obis_path_t parse_obis_path(std::string const& path, char sep)
		{
			obis_path_t op;
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

		std::string translate_obis_names(std::vector<std::string> const& vec, char sep)
		{
			std::string op;
			op.reserve(vec.size());
			for (auto const& s : vec) {
				auto const r = from_str(s);
				if (!op.empty())	op += ':';
				if (r.second) {
					op.append(r.first.to_str());
				}
				else {
					op.append(s);
				}
			}
			return op;
		}

	}	
}	//	namespace node
