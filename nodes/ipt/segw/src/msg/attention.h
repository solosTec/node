/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SML_ATTENTION_H
#define NODE_SML_ATTENTION_H


#include <smf/sml/defs.h>
#include <smf/mbus/header.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/vm/controller.h>

namespace node
{
	class cache;
	namespace sml
	{
		/**
		 * Managing SML attentions
		 */
		class res_generator;
		class attention
		{
		public:
			attention(cyng::logging::log_ptr
				, res_generator& sml_gen				
				, cache& cfg);


			void register_this(cyng::controller& vm);

		private:
			void attention_init(cyng::context& ctx);
			void attention_send(cyng::context& ctx);
			void attention_set(cyng::context& ctx);

			void send_attention_code_ok(cyng::object trx, cyng::buffer_t);
			void send_attention_code_ok(cyng::object trx, cyng::object);

			void send_attention_code(cyng::object trx, cyng::buffer_t, obis, std::string);
			void send_attention_code(cyng::object trx, cyng::object, obis, std::string);


		private:
			cyng::logging::log_ptr logger_;

			/**
			 * buffer for current SML message
			 */
			res_generator& sml_gen_;

			/**
			 * configuration db
			 */
			cache& cache_;

		};

	}	//	sml
}

#endif
