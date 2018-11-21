/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_IMEGA_PARSER_H
#define NODE_IMEGA_PARSER_H

#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/intrinsics/version.h>
#include <boost/asio.hpp>
#include <boost/variant.hpp>

#include <chrono>
#include <boost/assert.hpp>

namespace node	
{
	namespace imega
	{

		/** @brief iMEGA parser
		 *	
		 * iMega is predecessor of the infamous IP-T protocol developed and shipped by L&G. It is mostly
		 * used to communicate over DLSM/COSEM.
		 */
		class parser
		{
		public:
			using parser_callback = std::function<void(cyng::vector_t&&)>;

		private:
			/**
			 *	Each session starts in login mode. The incoming
			 *	data are expected to be an iMEGA login command. After a successful
			 *	login, parser switch to stream mode.
			 */
			enum state : std::uint32_t
			{
				STATE_ERROR,			//!>	error
				STATE_INITIAL,			//!<	initial - expect a '<'
				STATE_LOGIN,			//!<	gathering login data
				STATE_STREAM_MODE,		//!<	parser in data mode
				STATE_ALIVE_A,			//!<	probe <ALIVE> watchdog
				STATE_ALIVE_L,			//!<	probe <ALIVE> watchdog
				STATE_ALIVE_I,			//!<	probe <ALIVE> watchdog
				STATE_ALIVE_V,			//!<	probe <ALIVE> watchdog
				STATE_ALIVE_E,			//!<	probe <ALIVE> watchdog
				STATE_ALIVE,			//!<	until '>'

				STATE_LAST,
			};

			struct login
			{
				login();

				static bool is_closing(char);
				static int read_protocol(std::string const&);
				static cyng::version read_version(std::string const& inp);
				static std::string read_string(std::string const& inp, std::string const exp);
			};
			struct stream
			{
				stream();
				static bool is_opening(char);
			};

			struct alive
			{
				alive();
				static bool is_closing(char);
			};

			using parser_state_t = boost::variant<
				login,
				stream,
				alive
			>;

			//
			//	signals
			//
			struct state_visitor : public boost::static_visitor<state>
			{
				state_visitor(parser&, char c);
				state operator()(login&) const;
				state operator()(stream&) const;
				state operator()(alive&) const;

				parser& parser_;
				const char c_;
			};


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
				input_.clear();
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

			/*
			 * internal state
			 */
			state stream_state_;
			parser_state_t	parser_state_;

			/**
			 *	input stream buffer
			 */
			boost::asio::streambuf	stream_buffer_;
			std::iostream			input_;

			/**
			 *	probe watchdog size
			 */
			std::size_t wd_size_;

		};
	}
}	

#endif	
