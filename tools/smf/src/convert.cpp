/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "convert.h"
#include "cli.h"
#include <smf/sml/exporter/abl_sml_exporter.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>

#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>
#include <cyng/csv.h>
#include <cyng/util/split.h>
#include <cyng/parser/chrono_parser.h>
#include <cyng/parser/buffer_parser.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	convert::convert(cli* cp)
		: cli_(*cp)
	{
		cli_.vm_.register_function("convert", 1, std::bind(&convert::csv_abl, this, std::placeholders::_1));
	}

	convert::~convert()
	{}

	void convert::csv_abl(cyng::context& ctx)
	{

		//, cyng::param_factory("SML:ABL", cyng::tuple_factory(
		//	cyng::param_factory("root-dir", (cwd / "abl").string()),
		//	cyng::param_factory("prefix", "smf"),
		//	cyng::param_factory("suffix", "abl"),
		//	cyng::param_factory("version", NODE_SUFFIX),
		//	cyng::param_factory("period", rng()),	//	seconds
		//	cyng::param_factory("line-ending", "DOS")	//	DOS/UNIX
		//))

		auto const frame = ctx.get_frame();
		//out_ <<
		//	"CONVERT " << cyng::io::to_str(frame) << std::endl;

		boost::filesystem::path const p(cyng::value_cast<std::string>(frame.at(0), "D:\\installations\\EBS\\incident-sep-2019\\SK-26-0500153B018EEB-01A815155368030102.csv"));
		if (boost::filesystem::exists(p)) {

			if (boost::filesystem::is_directory(p)) {
				csv_abl_dir(p);
			}
		}
		else {
			cli_.out_
				<< "***error: file not found "
				<< p
				<< std::endl
				;
		}
	}

	void convert::csv_abl_file(boost::filesystem::path p)
	{
		//
		//	analyse file name
		//
		auto rv = cyng::split(p.filename().string(), "-.");
		if (rv.size() > 4) {

			//
			//	extract server and gateway ID
			//	02E61E276603153503
			//
			if (rv.at(3).size() != 18) {

				cli_.out_
					<< "***warning: invalid server ID "
					<< rv.at(3)
					<< std::endl
					;
			}
			else {
				auto const r = cyng::parse_hex_string(rv.at(3));
				if (r.second) {
					csv_abl_data(p, rv.at(2), r.first);
				}
				else {
					cli_.out_
						<< "***warning: invalid server ID "
						<< rv.at(3)
						<< std::endl
						;
				}

			}
		}
		else {
			cli_.out_
				<< "***warning: insufficient file name "
				<< p.filename()
				<< std::endl
				;
		}
	}

	void convert::csv_abl_dir(boost::filesystem::path dir)
	{
		std::for_each(boost::filesystem::directory_iterator(dir)
			, boost::filesystem::directory_iterator()
			, [&](boost::filesystem::path const& p) {

				if (boost::algorithm::equals(p.extension().string(), ".csv")) {
					csv_abl_file(p);
				}

			});

	}

	void convert::csv_abl_data(boost::filesystem::path p, std::string gw, cyng::buffer_t const& srv_id)
	{
		auto const vec = cyng::csv::read_file(p.string());
		if (vec.size() > 2) {

			//
			//	get first entry
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(vec.at(0), tpl);
			BOOST_ASSERT(tpl.size() > 1);
#ifdef _DEBUG
			BOOST_ASSERT(boost::algorithm::equals(cyng::value_cast<std::string>(tpl.front(), ""), "Nr."));
#endif

			//
			//	analyse CSV header
			//
			auto const header = analyse_csv_header(tpl);

			//
			//	loop over data set
			//
			for (auto pos = 2; pos < vec.size(); ++pos) {

				//
				//	get time point
				//
				tpl = cyng::value_cast(vec.at(pos), tpl);
				auto tp = get_ro_time(header, tpl);

				//
				//	create output file
				//
				auto const path = sml::get_abl_filename("push_"	//	prefix
					, "abl"	//	suffix
					, gw	//	gateway ID
					, sml::from_server_id(srv_id)
					, tp);	//	server ID

				cli_.out_
					<< "***info: write file "
					<< path
					<< std::endl
					;

				boost::filesystem::path out("D:\\installations\\EBS\\incident-sep-2019\\restored");
				std::ofstream of((out / path).string(), std::ios::app | std::ios::binary);

				//
				//	convert server id to buffer
				//
				//std::pair<buffer_t, bool > parse_hex_string(std::string const& inp);
				//auto const srv = sml::from_server_id(srv_id);


				//
				//	write header
				//
				sml::emit_abl_header(of
					, srv_id	//	srv id
					, tp
					, "\r\n");

				//
				//	write data
				//
				//0-0:96.1.0*255(03685327)	//	meter
				//1-0:1.8.0*255(8429866.6*Wh)
				//1-0:1.8.1*255(5819050.8*Wh)
				//1-0:1.8.2*255(2610815.8*Wh)
				//1-0:2.8.0*255(279.4*Wh)
				//1-0:2.8.1*255(0*Wh)
				//1-0:2.8.2*255(279.4*Wh)

				for (auto const& col : header) {
					if (col.first != sml::OBIS_CURRENT_UTC
						&& col.first != sml::OBIS_DATA_MANUFACTURER) {

						auto idx = tpl.begin();
						std::advance(idx, col.second);

						auto s = cyng::value_cast<std::string>(*idx, "");
						std::replace(s.begin(), s.end(), ' ', '*');

						of
							<< sml::to_string(col.first)
							<< '('
							<< s
							<< ')'
							<< "\r\n"
							;
					}
				}

				//
				//	write epilog
				//Raw(0)
				//!
				//
				of
					<< "Raw(0)"
					<< "\r\n"
					<< "!"
					<< "\r\n"
					;
			}

		}
		else {
			cli_.out_
				<< "***warning: file contains no data "
				<< p
				<< std::endl
				;

		}
	}
	std::map<sml::obis, std::size_t> analyse_csv_header(cyng::tuple_t tpl)
	{
		std::map<sml::obis, std::size_t> result;
		std::size_t idx{ 0 };
		for (auto const& col : tpl) {
			if (idx > 1) {
				auto const s = cyng::value_cast<std::string>(col, "");
				auto const code = sml::to_obis(s);
				result.emplace(code, idx);
			}
			++idx;
		}
		return result;
	}

	std::chrono::system_clock::time_point get_ro_time(std::map<sml::obis, std::size_t> const& header, cyng::tuple_t const& tpl)
	{
		auto pos = header.find(sml::OBIS_CURRENT_UTC);
		if (pos != header.end()) {

			//
			//	timepoint at index pos->second
			//
			if (tpl.size() >= pos->second) {
				auto idx = tpl.begin();
				std::advance(idx, pos->second);

				auto const s = cyng::value_cast<std::string>(*idx, "");

				//std::pair<std::chrono::system_clock::time_point, bool > parse_db_timestamp(std::string const&);
				auto const r = cyng::parse_db_timestamp(s);
				if (r.second) return r.first;
			}
		}
		return std::chrono::system_clock::now();
	}

}