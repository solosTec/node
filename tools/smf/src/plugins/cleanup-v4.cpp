/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "cleanup-v4.h"
#include "../cli.h"

#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>
#include <cyng/csv.h>
#include <cyng/util/split.h>
#include <cyng/parser/chrono_parser.h>
#include <cyng/parser/buffer_parser.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/set_cast.h>
#include <cyng/dom/algorithm.h>


#include <fstream>
#include <boost/algorithm/string.hpp>

namespace node
{
	cleanup::cleanup(cli* cp)
		: cli_(*cp)
		, root_dir_("D:\\installations\\sgsw\\cleanup")
		, devices_()
		, active_e350_()
		, active_ipt_()
		, inactive_devs_()
	{
		cli_.vm_.register_function("cleanup", 1, std::bind(&cleanup::cmd, this, std::placeholders::_1));
	}

	cleanup::~cleanup()
	{}

	void cleanup::cmd(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		std::cout <<
			"CLEANUP " << cyng::io::to_str(frame) << std::endl;

		auto const cmd = cyng::value_cast<std::string>(frame.at(0), "list");

		if (boost::algorithm::equals(cmd, "help")) {
			std::cout
				<< "help\t\tprint this page" << std::endl
				<< "import\timport all configured data from " << root_dir_ / "device.csv" << std::endl
				<< "list\tdump all imported devices to screen" << std::endl
				<< "e350\tcollect e350 devices" << std::endl
				<< "ipt\tcollect IP-T devices" << std::endl
				<< "inter[section]\tdetermine inactive devices" << std::endl
				<< "dump\tdump all inactive devices on screen and into a file" << std::endl
				<< "sql\tgenerate an SQL script to delete all inactive devices" << std::endl
				<< "all\tdo all steps at once" << std::endl
				;
		}
		else if (boost::algorithm::equals(cmd, "import")) {

			import_data();
		}
		else if (boost::algorithm::equals(cmd, "list")) {
			for (auto const& rec : devices_) {
				std::cout
					<< cyng::io::to_str(rec)
					<< std::endl;
			}
		}
		else if (boost::algorithm::equals(cmd, "read")) {
			read_log();
		}
		else if (boost::algorithm::equals(cmd, "inter") || boost::algorithm::equals(cmd, "intersection")) {
			intersection();
		}
		else if (boost::algorithm::equals(cmd, "dump")) {
			dump_result();
		}
		else if (boost::algorithm::equals(cmd, "sql")) {
			generate_sql();
		}
		else if (boost::algorithm::equals(cmd, "all")) {
			import_data();
			read_log();
			intersection();
			dump_result();
			generate_sql();
		}
		else {
			std::cout <<
				"CLEANUP - unknown command: "
				<< cmd
				<< std::endl;
		}

	}

	void cleanup::import_data()
	{
		auto const file_name = root_dir_ / "device.csv";
		std::cout
			<< "read device list from " << file_name << std::endl
			;

		devices_ = cyng::csv::read_file(file_name.string());

		std::cout
			<< devices_.size()
			<< " devices imported"
			<< std::endl
			;

	}

	void cleanup::read_log()
	{
		if (boost::filesystem::is_directory(root_dir_))
		{
			//
			//	reset maps
			//
			active_ipt_.clear();
			active_e350_.clear();

			std::uint64_t counter{ 0 };
			std::for_each(boost::filesystem::directory_iterator(root_dir_)
				, boost::filesystem::directory_iterator()
				, [&](boost::filesystem::path const& p) {

					if (boost::filesystem::is_regular_file(p)) {
						auto const fn = p.filename().string();
						auto const ext = p.extension().string();
						if ((boost::algorithm::starts_with(fn, "smf-e350")) && boost::algorithm::equals(ext, ".log")) {
							++counter;
							std::cout <<
								"CLEANUP - read: "
								<< p
								<< std::endl;

							read_e350(p);
						}
						else if ((boost::algorithm::starts_with(fn, "ipt-master")) && boost::algorithm::equals(ext, ".log")) {
							++counter;
							std::cout <<
								"CLEANUP - read: "
								<< p
								<< std::endl;

							read_ipt(p);
						}
						else {
							std::cout <<
								"CLEANUP - skip: "
								<< p
								<< std::endl;
						}
					}
				});

			std::cout <<
				"CLEANUP "
				<< counter
				<< " log files read"
				<< std::endl
				<< active_e350_.size()
				<< " active e350 devices"
				<< std::endl
				<< active_ipt_.size()
				<< " active ipt devices"
				<< std::endl
				;

		}
		else {
			std::cout <<
				"CLEANUP - "
				<< root_dir_
				<< " is not a directory"
				<< std::endl;

		}
	}

	void cleanup::read_e350(boost::filesystem::path p)
	{
		std::ifstream finp(p.string());
		if (finp.is_open()) {
			std::string line;
			std::size_t counter{ 0 };
			while (std::getline(finp, line, '\n')) {
				++counter;
				if (line.empty()) {
					//	skip
				}
				else if ((line.size() > 47)
					&& (line.at(0x00) == '[')
					&& (line.at(0x05) == '-')
					&& (line.at(0x08) == '-')
					&& (line.at(0x0b) == ' ')
					&& (line.at(0x0e) == ':')
					&& (line.at(0x11) == ':')
					&& (line.at(0x14) == '.')
					&& (line.at(0x1d) == ']')
					&& (line.at(0x1e) == ' ')
					&& (line.at(0x2b) == '-')
					&& (line.at(0x2c) == '-')
					) {

					const auto data = line.substr(0x2e);
					if (boost::algorithm::starts_with(data, "imega.req.login.public")) {
						//active_e350_
						const auto values = cyng::split(data.substr(0x22), ",[]");
						if (values.size() > 5) {
							auto const r = active_e350_.emplace(values.at(3), values.at(4));
							if (r.second) {
								std::cout <<
									"CLEANUP - active e350: "
									<< values.at(3)
									<< " / "
									<< values.at(4)
									<< " @"
									<< line.substr(1, 0x1c)
									<< std::endl
									;
							}
						}
					}
				}
				else if ((line.size() > 47)
					&& (line.at(0) == '[')
					&& (line.at(5) == ']')
					) {
					//	skip data
				}
				else if ((line.size() > 40)
					&& (line.at(0) == ' ')
					&& (line.at(1) == ' ')
					&& (line.at(2) == ' ')
					&& (line.at(3) == ' ')
					) {
					//	skip data
				}
				else {
					std::cout 
						<< "CLEANUP - invalid log format in "
						<< p.filename()
						<< ", #"
						<< counter
						<< ": "
						<< line.substr(0, 31)
						<< " ..."
						<< std::endl;

				}
			}
		}
		else {
			std::cout <<
				"CLEANUP - cannot open file "
				<< p
				<< std::endl;
		}
	}

	void cleanup::read_ipt(boost::filesystem::path p)
	{
		std::ifstream finp(p.string());
		if (finp.is_open()) {
			std::string line;
			std::size_t counter{ 0 };
			while (std::getline(finp, line, '\n')) {
				++counter;
				if (line.empty()) {
					//	skip
				}
				else if ((line.size() > 47)
					&& (line.at(0x00) == '[')
					&& (line.at(0x05) == '-')
					&& (line.at(0x08) == '-')
					&& (line.at(0x0b) == ' ')
					&& (line.at(0x0e) == ':')
					&& (line.at(0x11) == ':')
					&& (line.at(0x14) == '.')
					&& (line.at(0x1d) == ']')
					&& (line.at(0x1e) == ' ')
					&& (line.at(0x2b) == '-')
					&& (line.at(0x2c) == '-')
					) {

					const auto data = line.substr(0x2e);
					//ipt.req.login.scrambled ee09cce5-ea70-413f-94bb-2569def2cfd7:SGSWPilotMUC01:ip-t2014
					if (boost::algorithm::starts_with(data, "ipt.req.login.public")) {
						//active_ipt_
						const auto values = cyng::split(data.substr(22), ",[]:");
						if (values.size() > 2) {
							auto const r = active_ipt_.emplace(values.at(1), values.at(2));
							if (r.second) {
								std::cout <<
									"CLEANUP - active ipt (public): "
									<< values.at(1)
									<< " / "
									<< values.at(2)
									<< " @"
									<< line.substr(1, 0x1c)
									<< std::endl
									;
							}
						}
					}
					else if (boost::algorithm::starts_with(data, "ipt.req.login.scrambled")) {
						const auto values = cyng::split(data.substr(25), ",[]:");
						if (values.size() > 2) {
							auto const r = active_ipt_.emplace(values.at(1), values.at(2));
							if (r.second) {
								std::cout <<
									"CLEANUP - active ipt (scrambled): "
									<< values.at(1)
									<< " / "
									<< values.at(2)
									<< " @"
									<< line.substr(1, 0x1c)
									<< std::endl
									;
							}
						}
					}
				}
				else {
					std::cout 
						<< "CLEANUP - invalid log format in "
						<< p.filename()
						<< " #"
						<< counter
						<< ": "
						<< line.substr(0, 31)
						<< " ..."
						<< std::endl;

				}
			}
		}
		else {
			std::cout <<
				"CLEANUP - cannot open file "
				<< p
				<< std::endl;
		}

	}

	void cleanup::intersection()
	{
		//
		//	clear map
		//
		inactive_devs_.clear();

		//
		//	update a list of all configured devices.
		//	skip first line
		//

		bool first_line{ true };
		for (auto const& dev : devices_) {
			if (first_line) {
				first_line = false;
			}
			else {
				auto const tpl = cyng::to_tuple(dev);
				if (tpl.size() == 10) {

					auto const name = cyng::value_cast<std::string>(cyng::find(tpl, 1), "");
					auto const msisdn = cyng::value_cast<std::string>(cyng::find(tpl, 3), "");
					auto const id = cyng::value_cast<std::string>(cyng::find(tpl, 5), "");
					auto const r = inactive_devs_.emplace( name, std::pair{ msisdn, id } );
					if (!r.second) {
						std::cout <<
							"CLEANUP - found duplicate: "
							<< cyng::io::to_str(dev)
							<< std::endl;

					}
				}
			}
		}

		std::cout <<
			"CLEANUP - "
			<< inactive_devs_.size()
			<< " devices configured"
			<< std::endl;

		//
		//	remove ipt devices
		//
		for (auto const& dev : active_ipt_) {

			auto const pos = inactive_devs_.find(dev.first);
			if (pos != inactive_devs_.end()) {
				std::cout <<
					"CLEANUP - "
					<< dev.first
					<< " is an active IP-T device"
					<< std::endl;

				//
				//	remove from inactive list
				//
				inactive_devs_.erase(pos);

			}
		}

		//
		//	remove e350 devices
		//
		for (auto const& dev : active_e350_) {
			auto const pos = inactive_devs_.find(dev.first);
			if (pos != inactive_devs_.end()) {
				std::cout <<
					"CLEANUP - "
					<< dev.first
					<< " is an active e350 device"
					<< std::endl;

				//
				//	remove from inactive list
				//
				inactive_devs_.erase(pos);
			}
		}

		std::cout <<
			"CLEANUP - there are "
			<< inactive_devs_.size()
			<< " inactive device(s)"
			<< std::endl;
		
	}

	void cleanup::dump_result()
	{
		auto const dump = root_dir_ / "result.csv";
		std::ofstream fout(dump.string(), std::ios::trunc);
		if (fout.is_open()) {

			std::cout <<
				"CLEANUP - write "
				<< inactive_devs_.size()
				<< " records into "
				<< dump
				<< std::endl;
			

			fout
				<< "name,msisdn,id"
				<< std::endl;
			for (auto const& rec : inactive_devs_) {
				fout
					<< "\""
					<< rec.first
					<< "\",\""
					<< rec.second.first
					<< "\",\""
					<< rec.second.second
					<< "\""
					<< std::endl;
			}
		}
		else {
			std::cout <<
				"CLEANUP - cannot open output file "
				<< dump
				<< std::endl;

		}
	}

	void cleanup::generate_sql()
	{
		auto const file_name = root_dir_ / "result.csv";
		std::cout
			<< "read result list from " << file_name << std::endl
			;

		devices_ = cyng::csv::read_file(file_name.string());

		std::cout
			<< (devices_.size() - 1)
			<< " inactice devices imported"
			<< std::endl
			;


		auto const dump = root_dir_ / "result.sql";
		std::ofstream fout(dump.string(), std::ios::trunc);
		if (fout.is_open()) {

			std::cout <<
				"CLEANUP - write "
				<< inactive_devs_.size()
				<< " SQL commands into "
				<< dump
				<< std::endl;


			bool first_line{ true };
			for (auto const& dev : devices_) {
				if (first_line) {
					first_line = false;
				}
				else {
					auto const tpl = cyng::to_tuple(dev);
					if (tpl.size() == 3) {
						auto const name = cyng::value_cast<std::string>(cyng::find(tpl, 0), "");
						//auto const msisdn = cyng::value_cast<std::string>(cyng::find(tpl, 1), "");
						auto const id = cyng::value_cast<std::string>(cyng::find(tpl, 2), "");
						if (!name.empty()) {
							fout
								<< "DELETE FROM TDevice WHERE name = "
								<< "'"
								<< name
								<< "' AND id = '"
								<< id
								<< "';"
								<< std::endl;
						}
					}
				}
			}
		}
		else {
			std::cout <<
				"CLEANUP - cannot open output file "
				<< dump
				<< std::endl;

		}
	}

}