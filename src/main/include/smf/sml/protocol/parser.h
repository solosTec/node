/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_LIB_SML_PARSER_H
#define NODE_LIB_SML_PARSER_H

#include <smf/sml/defs.h>
#include <cyng/intrinsics/sets.h>

#include <boost/variant.hpp>
#include <functional>
#include <stack>

namespace node 
{
	namespace sml
	{
		/**
		 *	Implementation of the SML protocol
		 */
		class parser
		{
		public:
			using parser_callback = std::function<void(cyng::vector_t&&)>;

		private:
			/**
			 * This enum stores the global state
			 * of the parser. For each state there
			 * are different helper variables mostly
			 * declared in the private section of this class.
			 */
			enum state
			{
				STATE_ERROR,
				STATE_START,
				STATE_LENGTH,	//!<	check MSB
				STATE_STRING,
				STATE_OCTET,
				STATE_BOOLEAN,
				STATE_INT8,
				STATE_INT16,
				STATE_INT32,
				STATE_INT64,
				STATE_UINT8,
				STATE_UINT16,
				STATE_UINT32,
				STATE_UINT64,
				STATE_ESC,
				STATE_PROPERTY,
				STATE_START_STREAM,
				STATE_START_BLOCK,
				STATE_TIMEOUT,
				STATE_BLOCK_SIZE,
				STATE_EOM
			};

			struct sml_start {
			};
			struct sml_length {
			};
			struct sml_string 
			{
				sml_string(std::size_t size);
				bool push(char);
				std::size_t size_;
				cyng::buffer_t octet_;
			};
			struct sml_bool {
			};

			/**
			 * generic state class for numbers
			 */
			template <typename T>
			struct sml_number {
				union {
					T n_;
					char a_[sizeof(T)];
				} u_;
				std::size_t pos_;

				sml_number(std::size_t pos /*= sizeof(T)*/)
					: pos_(pos)
				{
					BOOST_ASSERT(pos_ <= sizeof(T));
				}
				bool push(char c)
				{
					BOOST_ASSERT(pos_ != 0);
					pos_--;
					u_.a_[pos_] = c;
					return pos_ == 0;
				}
			};
			using sml_uint8 = sml_number<std::uint8_t>;
			using sml_uint16 = sml_number<std::uint16_t>;
			using sml_uint32 = sml_number<std::uint32_t>;
			using sml_uint64 = sml_number<std::uint64_t>;
			using sml_int8 = sml_number<std::int8_t>;
			using sml_int16 = sml_number<std::int16_t>;
			using sml_int32 = sml_number<std::int32_t>;
			using sml_int64 = sml_number<std::int64_t>;

			struct sml_esc {
				std::size_t counter_;
				sml_esc();
			};
			struct sml_prop {};

			struct sml_start_stream{
				std::size_t counter_;
				sml_start_stream();
			};
			struct sml_start_block {
				cyng::buffer_t prop_;
				sml_start_block();
				bool push(char c);
			};
			struct sml_timeout {
				cyng::buffer_t prop_;
				sml_timeout();
				bool push(char c);
			};
			struct sml_block_size {
				cyng::buffer_t prop_;
				sml_block_size();
				bool push(char c);
			};
			struct sml_eom {
				std::size_t counter_;
				std::uint16_t crc_;
				std::uint8_t pads_;
				sml_eom();
				bool push(char c);
			};

			using parser_state_t = boost::variant<sml_start,
				sml_length,
				sml_string,	
				sml_bool,
				sml_uint8,
				sml_uint16,
				sml_uint32,
				sml_uint64,
				sml_int8,
				sml_int16,
				sml_int32,
				sml_int64,
				sml_esc,
				sml_prop,
				sml_start_stream,
				sml_start_block,
				sml_timeout,
				sml_block_size,
				sml_eom
			>;

			//
			//	signals
			//
			struct state_visitor : public boost::static_visitor<state>
			{
				state_visitor(parser&, char c);
				state operator()(sml_start&) const;
				state operator()(sml_length&) const;
				state operator()(sml_string&) const;
				state operator()(sml_bool&) const;
				state operator()(sml_uint8&) const;
				state operator()(sml_uint16&) const;
				state operator()(sml_uint32&) const;
				state operator()(sml_uint64&) const;
				state operator()(sml_int8&) const;
				state operator()(sml_int16&) const;
				state operator()(sml_int32&) const;
				state operator()(sml_int64&) const;
				state operator()(sml_esc&) const;
				state operator()(sml_prop&) const;
				state operator()(sml_start_stream&) const;
				state operator()(sml_start_block&) const;
				state operator()(sml_timeout&) const;
				state operator()(sml_block_size&) const;
				state operator()(sml_eom&) const;

				parser& parser_;
				const char c_;
			};

			/**
			 * collect elements of a list
			 */
			struct list
			{
				const std::size_t target_;
				cyng::tuple_t	values_;

				list(std::size_t);
				bool push(cyng::object);
			};

		public:
			/**
			 * @param cb this function is called, when parsing is complete
			 */
			parser(parser_callback cb, bool verbose, bool log);

			/**
			 * The destructor is required since the unique_ptr
			 * uses an incomplete type.
			 */
			virtual ~parser();

			/**
			 * parse the specified range
			 */
			template < typename I >
			void read(I start, I end)
			{
				std::for_each(start, end, [this](char c)
				{
					this->put(c);
				});

				post_processing();
			}

			/**
			 * Reset parser
			 */
			void reset();

		private:
			/**
			 * read a single byte and update
			 * parser state.
			 * Implements the state machine
			 */
			void put(char);

			/**
			 * Probe if parsing is complete and
			 * inform listener.
			 */
			void post_processing();

			/**
			 * returns next state when length info is complete
			 */
			state next_state();

			/**
			 * produce the next element
			 */
			void emit_eom();
			void emit_nil();
			void emit(cyng::buffer_t&&);

			/**
			 * debug helper
			 */
			std::string prefix() const;

			/**
			 * push element on list stack
			 */
			void push(cyng::object);

			/**
			 * push data complete
			 */
			void finalize(std::uint16_t crc, std::uint8_t gap);

		private:
			/**
			 * call this method if parsing is complete
			 */
			parser_callback	cb_;

			const bool verbose_;
			const bool log_;	//!< generate log instructions
			std::size_t pos_;	//!< position index

			/**
			 * instruction buffer
			 */
			cyng::vector_t	code_;

			/**
			 * Collect type and length data 
			 */
			sml_tl tl_;

			/**
			 * offset for long lists
			 */
			std::size_t		offset_;

			state	stream_state_;
			parser_state_t	parser_state_;

			/**
			 *	CRC designated by CCITT
			 */
			std::uint16_t	crc_;
			bool			crc_on_;	//!< stop CRC calculation

			/**
			 * message counter
			 */
			std::size_t		counter_;

			/**
			 * Collect nested data
			 */
			std::stack<list>	stack_;
		};
	}
}	//	node

//#pragma pack( pop )	//	reset packing alignment

#endif // NODE_LIB_SML_PARSER_H
