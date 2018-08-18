/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_IEC_PARSER_H
#define NODE_IEC_PARSER_H

#include <smf/sml/intrinsics/obis.h>
#include <cyng/intrinsics/sets.h>
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
				STATE_START,		//!< initial state
				STATE_STX,			//!< after STX
				STATE_DATA_BLOCK,	//!< data block
				STATE_DATA_LINE,
				//STATE_DATA_SET,
				STATE_ADDRESS_A,
				STATE_ADDRESS_B,
				STATE_ADDRESS_C,
				STATE_ADDRESS_D,
				STATE_ADDRESS_E,
				STATE_ADDRESS_F,
				STATE_CHOICE,
				STATE_VALUE,
				STATE_NO_VALUE,	//!< new;old value
				STATE_NEW_VALUE,	//!< new;old value
				STATE_OLD_VALUE,	//!< new;old value
				STATE_UNIT,
				STATE_ETX,
				STATE_BCC,	//!< XOR of all characters after STX and before ETX
				STATE_EOF	//!> after !
			};

			struct iec_start {};
			struct iec_address_a {};
			struct iec_address_d {
				std::string value_;
			};
			struct iec_address_e {
				std::string value_;
			};
			struct iec_address_f {
				std::string value_;
			};
			struct iec_value {
				std::string value_;
			};
			struct iec_choice {};
			struct iec_no_value {};
			struct iec_new_value {};
			struct iec_data_line {};
			struct iec_unit {
				std::string value_;
			};
			struct iec_eof {};

			using parser_state_t = boost::variant<iec_start,
				iec_address_a,
				iec_address_d,
				iec_address_e,
				iec_address_f,
				iec_choice,
				iec_value,
				iec_no_value,
				iec_data_line,
				iec_unit,
				iec_eof
			>;

			//
			//	signals
			//
			struct state_visitor : public boost::static_visitor<state>
			{
				state_visitor(parser&, char c);

				state operator()(iec_start&) const;
				state operator()(iec_address_a&) const;
				state operator()(iec_address_d&) const;
				state operator()(iec_address_e&) const;
				state operator()(iec_address_f&) const;
				state operator()(iec_choice&) const;
				state operator()(iec_value&) const;
				state operator()(iec_no_value&) const;
				state operator()(iec_data_line&) const;
				state operator()(iec_unit&) const;
				state operator()(iec_eof&) const;

				parser& parser_;
				const char c_;
			};


		public:
			parser(parser_callback, bool);
			virtual ~parser();

			template < typename I >
			auto read(I start, I end) -> typename std::iterator_traits<I>::difference_type
			{
				//	loop over input buffer
				std::for_each(start, end, [this](const char c)	
				{
					this->put(c);
				});

				post_processing();
// 				input_.clear();
				return std::distance(start, end);
			}

		private:
			/**
			 *	Parse data
			 */
			void put(char);

			/**
			 * Probe if parsing is completed and
			 * inform listener.
			 */
			void post_processing();

			void update_bcc(char);
			void set_value_group(std::size_t, std::uint8_t);
			void set_value(std::string const&);
			void set_unit(std::string const&);

		private:
			/**
			 * call this method if parsing is complete
			 */
			parser_callback	cb_;

			const bool verbose_;

			/**
			 * instruction buffer
			 */
			cyng::vector_t	code_;

			char bcc_;	//!< XOR of all characters after STX and before ETX

			/**
			 * global parser state
			 */
			state	state_;
			parser_state_t	parser_state_;

			/**
			 * address
			 */
			node::sml::obis	id_;

			/**
			 * value
			 */
			std::string value_;

		};
	}
}	

#endif	
