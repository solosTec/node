/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_PROCESSOR_H
#define NODE_IP_MASTER_PROCESSOR_H

#include <smf/cluster/bus.h>
#include <smf/cluster/bus.h>

#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <pugixml.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{

	class processor
	{
	public:
		processor(cyng::logging::log_ptr, cyng::io_service_t&, boost::uuids::uuid tag, bus::shared_type, std::ostream& = std::cout, std::ostream& = std::cerr);

		cyng::controller& vm();

	private:
		void https_launch_session_plain(cyng::context& ctx);
		void https_eof_session_plain(cyng::context& ctx);
		void res_push_data(cyng::context& ctx);
		void http_upload_start(cyng::context& ctx);
		void http_upload_data(cyng::context& ctx);
		void http_upload_progress(cyng::context& ctx);
		void http_upload_complete(cyng::context& ctx);
		void http_post_xml(cyng::context& ctx);

		void process_uplink_msg(pugi::xml_document const& doc, pugi::xml_node node);
		void process_localisation_msg(pugi::xml_document const& doc, pugi::xml_node node);
		void write_db(pugi::xml_node node, cyng::buffer_t const& payload);

		void parse_xml(std::string const*);

	private:
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		bus::shared_type bus_;
		boost::uuids::random_generator_mt19937 uidgen_;
	};
	
}

#endif
