/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_PARSER_H
#define SMF_SML_PARSER_H

#include <smf/sml/tokenizer.h>

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/object.h>

#include <stack>

namespace smf {

	namespace sml {


		/**
		 * This parser generates complete SML messages
		 */
		class parser
		{
			/**
			 * collect elements of an SML list
			 */
			struct list
			{
				std::size_t const size_;
				cyng::tuple_t	values_;

				list(std::size_t);
				bool push(cyng::object);

				/**
				 * current position in the SML list (zero-based)
				 */
				std::size_t pos() const;
			};

		public:
			parser();

			/**
			 * parse the specified range
			 */
			template < typename I >
			void read(I start, I end)
			{
				static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");
				std::for_each(start, end, [this](char c)
					{
						//
						//	parse
						//
						this->tokenizer_.put(c);
					});
			}

		private:
			/**
			 * callback from tokenizer
			 */
			void next(sml_type, std::size_t, cyng::buffer_t);

			void push(cyng::object&& obj);
			void push_integer(cyng::buffer_t const&);
			void push_unsigned(cyng::buffer_t const&);


		private:
			tokenizer	tokenizer_;

			/**
			 * Collect nested data
			 */
			std::stack<list>	stack_;

		};

	}
}
#endif
