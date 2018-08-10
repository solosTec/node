/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_IEC_PARSER_H
#define NODE_IEC_PARSER_H

#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/intrinsics/version.h>
#include <boost/asio.hpp>
#include <boost/variant.hpp>

#include <chrono>
#include <boost/assert.hpp>

namespace node	
{
	namespace iec
	{

		/**
		 *	IEC 62056-21 parser.
		 */
		class parser
		{
		public:
			using parser_callback = std::function<void(cyng::vector_t&&)>;

		public:
			parser(parser_callback);
			virtual ~parser();

			template < typename I >
			auto read(I start, I end) -> typename std::iterator_traits<I>::difference_type
			{
				//	loop over input buffer
				std::for_each(start, end, [this](const char c)	
				{
					this->parse(c);
				});

				post_processing();
// 				input_.clear();
				return std::distance(start, end);
			}

		private:
			/**
			 *	Parse data
			 */
			void parse(char);

			/**
			 * Probe if parsing is completed and
			 * inform listener.
			 */
			void post_processing();


		private:
			/**
			* call this method if parsing is complete
			*/
			parser_callback	cb_;

			/**
			 * instruction buffer
			 */
			cyng::vector_t	code_;


		};
	}
}	

#endif	
