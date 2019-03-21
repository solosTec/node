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

	class proxy_comm
	{
	public:
		proxy_comm(ipt::session_state&);

		void register_this(cyng::controller& vm);

	private:
		//
		//	SML data
		//
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);
		void sml_public_open_response(cyng::context& ctx);
		void sml_public_close_response(cyng::context& ctx);
		void sml_get_proc_param_srv_visible(cyng::context& ctx);
		void sml_get_proc_param_srv_active(cyng::context& ctx);
		void sml_get_proc_param_firmware(cyng::context& ctx);
		void sml_get_proc_param_simple(cyng::context& ctx);
		void sml_get_proc_param_status_word(cyng::context& ctx);
		void sml_get_proc_param_memory(cyng::context& ctx);
		void sml_get_proc_param_wmbus_status(cyng::context& ctx);
		void sml_get_proc_param_wmbus_config(cyng::context& ctx);
		void sml_get_proc_param_ipt_status(cyng::context& ctx);
		void sml_get_proc_param_ipt_param(cyng::context& ctx);
		void sml_get_proc_param_iec_config(cyng::context& ctx);
		void sml_get_list_response(cyng::context& ctx);
		void sml_attention_msg(cyng::context& ctx);

	private:
		ipt::session_state& state_;
	};

}


#endif

