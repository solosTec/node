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
#include <cyng/vm/domain/log_domain.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	cli::cli(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, cluster_config_t const& cfg
		, std::ostream& out
		, std::istream& in)
	: console(mux.get_io_service(), tag, out, in)
		, plugin_convert_(this)
		, plugin_tracking_(this)
		, plugin_cleanup_(this)
		, plugin_send_(this)
		, plugin_join_(this, mux, logger, tag, cfg)
	{
		cyng::register_logger(logger, vm_);
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
				//	usefull for scripts.
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
		//
		//	build a call frame
		//
		cyng::vector_t prg;
		{
			cyng::call_frame cf(prg);
			prg
				<< cyng::unwinder(params)
				<< cyng::invoke(cmd)
				;
		}
		vm_.async_run(std::move(prg));

		//
		//	emit a NL
		//
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

	void cli::shutdown()
	{
		plugin_join_.stop();
	}

}