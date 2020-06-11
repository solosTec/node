/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_CONSOLE_H
#define NODE_TOOL_SMF_CONSOLE_H


#include <cyng/intrinsics/sets.h>
#include <cyng/vm/controller.h>
#include <cyng/compatibility/io_service.h>

#include <string>
#include <iostream>
#include <list>

#include <cyng/compatibility/file_system.hpp>

namespace node
{
	class console
	{
	public:
		console(cyng::io_service_t& ios, boost::uuids::uuid tag, std::ostream&, std::istream&);
		virtual ~console();

	protected:
		virtual bool parse(std::string const&) = 0;

	protected:
		int loop();
		virtual void set_prompt(std::iostream& os);

		/**
		 * Last chance to exit
		 */
		virtual void shutdown();
		bool exec(std::string const&);
		bool quit();
		bool history() const;

		//	commands
		bool cmd_run(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
		bool cmd_run_script(cyng::filesystem::path const&);
		//bool cmd_echo(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
		bool cmd_list(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
		bool cmd_list(cyng::filesystem::path const&);
		bool cmd_ip() const;
		//bool cmd_pwd(unsigned);

	protected:
		std::ostream& out_;
		std::istream& in_;
		bool exit_;
		std::list< std::string >	history_;
		std::size_t call_depth_;
		cyng::controller vm_;
	};

	/**
	 * Sets the specified extension if the file name doesn't contains one.
	 */
	cyng::filesystem::path verify_extension(cyng::filesystem::path p, std::string const& ext);

}

#endif	
