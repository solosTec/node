/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_FORM_DATA_H
#define NODE_HTTP_FORM_DATA_H

#include <cyng/log.h>
#include <cyng/vm/controller.h>

namespace node 
{
	class form_data
	{
	public:
		form_data(cyng::logging::log_ptr);

		void register_this(cyng::controller&);

	private:
		void http_upload_start(cyng::context& ctx);
		void http_upload_data(cyng::context& ctx);
		void http_upload_var(cyng::context& ctx);
		void http_upload_progress(cyng::context& ctx);
		void http_upload_complete(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;

		/**
		 * @brief form values.
		 *
		 * Key of this map is the web-session tag. The value contains
		 * key/value pairs that where sent by the form. 
		 */
		std::map<boost::uuids::uuid, std::map<std::string, std::string>>	data_;
	};
}

#endif
