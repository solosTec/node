/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_PROXY_COMM_H
#define NODE_IPT_MASTER_PROXY_COMM_H

#include <cyng/vm/controller.h>

namespace node 
{
	namespace ipt
	{
		//
		//	forwward declarations
		//
		class session_state;
	}

	/**
	 * pre-process incoming SML messages
	 */
	class proxy_comm
	{
	public:
		proxy_comm(ipt::session_state&, cyng::controller& vm);

	private:
		void register_this();

		//
		//	SML data
		//
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);
		void sml_public_open_response(cyng::context& ctx);
		void sml_public_close_response(cyng::context& ctx);
		void sml_get_proc_param_response(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);
		void sml_attention_msg(cyng::context& ctx);

		//void sml_get_proc_parameter(cyng::context& ctx);

	private:
		ipt::session_state& state_;
		cyng::controller& vm_;
		std::size_t open_requests_;
	};

}


#endif

