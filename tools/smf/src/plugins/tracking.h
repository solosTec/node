/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_TRACKING_H
#define NODE_TOOL_SMF_PLUGIN_TRACKING_H

#include <smf/sml/intrinsics/obis.h>
#include <cyng/vm/context.h>
#include <cyng/compatibility/file_system.hpp>

namespace node
{
	class cli;
	class tracking
	{
	public:
		tracking(cli*);
		virtual ~tracking();

	private:
		void cmd(cyng::context& ctx);
		std::string get_status() const;
		void load(std::string);
		void init(std::string);
		void list() const;
		void start(std::string);
		void flush(std::string);
		void stop();
		void status();

		std::string get_filename(std::size_t, cyng::vector_t const&);

	private:
		cli& cli_;
		enum class status {
			INITIAL,	//	closed
			OPEN,
			RUNNING
		} status_;
		cyng::vector_t data_;
		cyng::filesystem::path af_;	//	active file
	};

	std::string get_parameter(std::size_t, cyng::vector_t const&, std::string);
	cyng::tuple_t create_record(std::string);
}

#endif	
