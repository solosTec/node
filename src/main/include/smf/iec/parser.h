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
#include <boost/uuid/random_generator.hpp>

namespace node	
{
	namespace iec
	{

		/**
		 *	IEC 62056-21 parser (readout mode).
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
			enum class state
			{
				_ERROR,
				START,		//!< initial state
				STX,			//!< after STX
				DATA_BLOCK,	//!< data block
				DATA_LINE,
				DATA_SET,
				OBIS,			//!< ID
				CHOICE_VALUE,
				CHOICE_STATUS,
				VALUE,
				STATUS,	//!< new;old status
				UNIT,
				ETX,
				BCC,	//!< XOR of all characters after STX and before ETX
				//STATE_EOF	//!> after !
			};

			struct iec_start {};
			struct iec_obis {
				std::string value_;
			};
			struct iec_value {
				std::string value_;
			};
			struct iec_choice_value {};
			struct iec_choice_status {};
			struct iec_new_value {};
			struct iec_data_line {};
			struct iec_unit {
				std::string value_;
			};
			struct iec_data_block {};
			struct iec_data_set {};
			struct iec_etx {};
			struct iec_bcc {};
			struct iec_status {
				std::string value_;
			};

			using parser_state_t = boost::variant<iec_start,
				iec_obis,
				iec_value,
				iec_choice_value,
				iec_choice_status,
				iec_data_line,
				iec_unit,
				iec_data_block,
				iec_data_set,
				iec_etx,
				iec_bcc,
				iec_status
			>;

			//
			//	signals
			//
			struct state_visitor : public boost::static_visitor<state>
			{
				state_visitor(parser&, char c);

				state operator()(iec_start&) const;
				state operator()(iec_obis&) const;
				state operator()(iec_value&) const;
				state operator()(iec_choice_value&) const;
				state operator()(iec_choice_status&) const;
				state operator()(iec_data_line&) const;
				state operator()(iec_unit&) const;
				state operator()(iec_data_block&) const;
				state operator()(iec_data_set&) const;
				state operator()(iec_etx&) const;
				state operator()(iec_bcc&) const;
				state operator()(iec_status&) const;

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

				//post_processing();
				return std::distance(start, end);
			}

		private:
			/**
			 *	Parse data
			 */
			void put(char);

			/**
			 * reset all values
			 */
			void clear();

			/**
			 * reset parser
			 */
			void reset();


			void update_bcc(char);
			void test_bcc(char);
			void set_value_group(std::size_t, std::uint8_t);
			void set_value(std::string const&);
			void set_unit(std::string const&);
			void set_id(std::string const&);
			void set_status(std::string const&);

		private:
			/**
			 * call this method if parsing is complete
			 */
			parser_callback	cb_;

			const bool verbose_;

			char bcc_;	//!< XOR of all characters after STX and before ETX
			bool bcc_flag_;

			/**
			 * global parser state
			 */
			state	state_;
			parser_state_t	parser_state_;

			/**
			 * value, unit_, status
			 */
			std::string value_, unit_, status_;

			/**
			 * current OBIS code
			 */
			node::sml::obis id_;

			/**
			 * data counter
			 */
			std::size_t counter_;

			/**
			 * generate unique key for data sets
			 */
			boost::uuids::random_generator rgn_;

			boost::uuids::uuid pk_;

		};
	}
}	

#endif	
