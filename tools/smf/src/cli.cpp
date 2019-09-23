/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "cli.h"
#include <NODE_project_info.h>

#include <cyng/util/split.h>
#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	cli::cli(cyng::io_service_t& ios, boost::uuids::uuid tag, std::ostream& out, std::istream& in)
		: console(ios, tag, out, in)
		, plugin_convert_(this)
	{
	}

	cli::~cli()
	{}

	bool cli::parse(std::string const& cmd)
	{
		auto rv = cyng::split(cmd, " \t,");
		if (!rv.empty()) {

			if (boost::algorithm::equals(rv.front(), "ls")) {
				if (rv.size() > 1) {
					return cmd_list(rv.at(1));
				}
				else {
					return cmd_list(".");
				}
			}
			else if (boost::algorithm::equals(rv.front(), "echo")) {

				//
				//	print all parsed elements.
				//	usfull for script.
				//
				std::size_t counter{ 0 };
				for (auto const& s : rv) {
					switch (counter) {
					case 0:
						break;
					case 1:
						out_ << s;
						break;
					default:
						out_ << ' ' << s;
						break;
					}
					++counter;
				}
				out_ << std::endl;
			}
			else {
				auto const f = rv.front();
				rv.erase(rv.begin());
				call(f, rv);
			}
			return true;
		}
		return false;
	}

	void cli::call(std::string cmd, std::vector<std::string> const& params)
	{
		switch (params.size()) {
		case 0:
			vm_.async_run(cyng::generate_invoke(cmd));
			break;
		case 1:
			vm_.async_run(cyng::generate_invoke(cmd, params.at(0)));
			break;
		case 2:
			vm_.async_run(cyng::generate_invoke(cmd, params.at(0), params.at(1)));
			break;
		case 3:
			vm_.async_run(cyng::generate_invoke(cmd, params.at(0), params.at(1), params.at(2)));
			break;
		default:
			out_ << "***error: to much arguments " << params.size() << std::endl;
			break;
		}
		vm_.async_run(cyng::generate_invoke("nl"));
	}

	int cli::run()
	{
		//
		//	Welcome message
		//
		out_
			<< "Welcome to SMF tool v"
			<< NODE_VERSION
			<< std::endl
			<< "Enter \"q\" to leave or \"help\" for instructions"
			<< std::endl
			;

		return loop();
	}

	int cli::run(std::string const& file_name)
	{
		return (cmd_run_script(file_name))
			? EXIT_SUCCESS
			: EXIT_FAILURE
			;
	}

}