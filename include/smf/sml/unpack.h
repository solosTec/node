/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_UNPACK_H
#define SMF_SML_UNPACK_H

#include <smf/sml/parser.h>

#include <cyng/obj/intrinsics/buffer.h>

#include <cstdint>

namespace smf {

	namespace sml {

		class unpack
		{
			enum class state
			{
				PAYLOAD,
				ESC,
				PROP,
				START,
				STOP,
			} state_;

		public:
			unpack(parser::cb);

			/**
			 * parse the specified range
			 */
			template < typename I >
			std::size_t read(I start, I end)
			{
				using value_type = typename std::iterator_traits<I>::value_type;

				std::for_each(start, end, [this](value_type c) {
					//
					//	read input stream
					//
					return this->put(c);
					});
				return std::distance(start, end);
			}

			parser const& get_parser() const;

		private:

			/**
			 * read a single byte and update
			 * parser state.
			 * Implements the state machine
			 */
			void put(char);

			state state_esc(char);
			state state_prop(char);
			state state_payload(char);
			state state_start(char);
			state state_stop(char);

		private:
			parser parser_;
			std::size_t counter_;
			std::uint8_t pads_;
			std::uint16_t crc_;
		};
	}
}
#endif
