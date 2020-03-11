/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/generator.h>
#include <cyng/vm/generator.h>

namespace node
{
	namespace ipt
	{
		namespace gen
		{

			cyng::vector_t ipt_req_login_public(master_record const& config)
			{
				cyng::vector_t prg;
				return prg
					<< cyng::generate_invoke_unwinded("log.msg.info", "resolve address ", config.host_, ':', config.service_)
					<< cyng::generate_invoke_unwinded("ip.tcp.socket.resolve", config.host_, config.service_)
					<< ":SEND-LOGIN-REQUEST"			//	label
					<< cyng::code::JNE					//	jump if no error
					<< cyng::generate_invoke_unwinded("bus.reconfigure", cyng::code::LERR, config.monitor_)
					<< cyng::generate_invoke_unwinded("log.msg.error", cyng::code::LERR)	// load error register
					<< ":STOP"							//	label
					<< cyng::code::JA					//	jump always
					<< cyng::label(":SEND-LOGIN-REQUEST")
					<< cyng::generate_invoke_unwinded("log.msg.debug", "public login ", config.account_, ':', config.pwd_)
					<< cyng::generate_invoke_unwinded("ipt.start")		//	start reading ipt network
					<< cyng::generate_invoke_unwinded("req.login.public", config.account_, config.pwd_)
					<< cyng::generate_invoke_unwinded("stream.flush")
					<< cyng::label(":STOP")
					<< cyng::code::NOOP
					<< cyng::reloc()
					;
			}

			cyng::vector_t ipt_req_login_scrambled(master_record const& config)
			{
				scramble_key sk = gen_random_sk();
				//cyng::generate_invoke("log.msg.info", "log domain is running")
				cyng::vector_t prg;
				return prg
					<< cyng::generate_invoke_unwinded("log.msg.info", "resolve address", config.host_, config.service_)
					<< cyng::generate_invoke_unwinded("ip.tcp.socket.resolve", config.host_, config.service_)
					<< ":SEND-LOGIN-REQUEST"			//	label
					<< cyng::code::JNE					//	jump if no error
					<< cyng::generate_invoke_unwinded("bus.reconfigure", cyng::code::LERR)
					<< cyng::generate_invoke_unwinded("log.msg.error", cyng::code::LERR)	// load error register
					<< ":STOP"							//	label
					<< cyng::code::JA					//	jump always
					<< cyng::label(":SEND-LOGIN-REQUEST")
					<< cyng::generate_invoke_unwinded("log.msg.debug", "scrambled login", config.account_, config.pwd_)
					<< cyng::generate_invoke_unwinded("ipt.start")		//	start reading ipt network
					<< cyng::generate_invoke_unwinded("ipt.set.sk", sk)	//	set new scramble key for parser
					<< cyng::generate_invoke_unwinded("req.login.scrambled", config.account_, config.pwd_, sk)
					<< cyng::generate_invoke_unwinded("stream.flush")
					<< cyng::label(":STOP")
					<< cyng::code::NOOP
					<< cyng::reloc()
					;

			}

		}
	}
}
