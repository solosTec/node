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
	 * Pre-process incoming SML messages.
	 * This is helper class for the geateway proxy and implements
	 * a preprocessing of incoming SML messages from an gateway (or an other device).
	 * 
	 * The registered callbacks are triggered by the SML parser from the gateway proxy.
	 * With help of the session state object the resulting messages are bounced back to 
	 * gateway proxy.
	 */
	class proxy_comm
	{
	public:
		proxy_comm(ipt::session_state&, cyng::controller& vm);

	private:
		void register_this();

		/**
		 * This effectively converts the SML message into function calls
		 * into the VM.
		 */
		void sml_msg(cyng::context& ctx);

		//
		//	SML data
		//
		void sml_eom(cyng::context& ctx);
		void sml_public_open_response(cyng::context& ctx);
		void sml_public_close_response(cyng::context& ctx);
		void sml_get_proc_param_response(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);
		void sml_get_profile_list_response(cyng::context& ctx);
		void sml_attention_msg(cyng::context& ctx);

	private:
		ipt::session_state& state_;
		cyng::controller& vm_;
	};

}


#endif

