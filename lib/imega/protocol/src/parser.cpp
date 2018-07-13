/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/imega/parser.h>
#include <cyng/vm/generator.h>
#include <boost/algorithm/string.hpp>

namespace node	
{
	namespace imega
	{
		/*
		 *	cu parser
		 */
		parser::parser(parser_callback cb)
			: cb_(cb)
			, code_()
			, stream_state_(STATE_INITIAL)
			, parser_state_()
			, stream_buffer_()
			, input_(&stream_buffer_)
			, wd_size_(0)
		{
			parser_state_ = login();
		}

		parser::~parser()
		{}

		void parser::parse(char c)	
		{
			//
			//	save previous state
			//
			state prev_state = stream_state_;

			switch (stream_state_)
			{

			/*
			 *	gathering login data
			 */
			case STATE_INITIAL:
				if (c == '<')
				{
					//
					//	expect something like:
					//	<PROT:1;VERS:1.0;TELNB:96110845-2;MNAME:95298073>
					//	TELNB ist the login name (96110845-2)
					//
					stream_state_ = STATE_LOGIN;
				}
				else 
				{
					stream_state_ = STATE_ERROR;
				}
				break;

			/*
			 *	login data
			 */
			case STATE_LOGIN:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;

			/*
			 *	data mode
			 */
			case STATE_STREAM_MODE:
				if (c == '<')	
				{
					stream_state_ = STATE_ALIVE_A;
				}
				else 
				{
					input_.put(c);
				}
				break;

			/*
			 *	watchdog mode <ALIVE>
			 */
			case STATE_ALIVE_A:
				if (c == 'A')	
				{
					stream_state_ = STATE_ALIVE_L;
				}
				else 
				{
					stream_state_ = STATE_STREAM_MODE;
					input_.put('<');
					input_.put(c);
				}
				break;
			case STATE_ALIVE_L:
				if (c == 'L')	
				{
					stream_state_ = STATE_ALIVE_I;
				}
				else 
				{
					stream_state_ = STATE_STREAM_MODE;
					input_.put('<');
					input_.put('A');
					input_.put(c);
				}
				break;
			case STATE_ALIVE_I:
				if (c == 'I')	
				{
					stream_state_ = STATE_ALIVE_V;
				}
				else 
				{
					stream_state_ = STATE_STREAM_MODE;
					input_.put('<');
					input_.put('A');
					input_.put('L');
					input_.put(c);
				}
				break;
			case STATE_ALIVE_V:
				if (c == 'V')	
				{
					stream_state_ = STATE_ALIVE_E;
				}
				else 
				{
					stream_state_ = STATE_STREAM_MODE;
					input_.put('<');
					input_.put('A');
					input_.put('L');
					input_.put('I');
					input_.put(c);
				}
				break;
			case STATE_ALIVE_E:
				if (c == 'E')	
				{
					stream_state_ = STATE_ALIVE;
					wd_size_ = 0;
				}
				else 
				{
					stream_state_ = STATE_STREAM_MODE;
					input_.put('<');
					input_.put('A');
					input_.put('L');
					input_.put('I');
					input_.put('V');
					input_.put(c);
				}
				break;

			/*
			 *	watchdog mode <ALIVE MNAME>
			 */
			case STATE_ALIVE:
				//
				//	This test is incomplete!
				//
				if (c == '>')	
				{
					//	watchdog complete
					stream_state_ = STATE_STREAM_MODE;
				}
				else 
				{
					wd_size_++;
					if (wd_size_ > 16)	
					{
						//	looks like an error
						//	switch to data mode
						stream_state_ = STATE_STREAM_MODE;
					}
				}

				break;

			default:
				BOOST_ASSERT_MSG(false, "illegal state");
				break;
			}

		}


		void parser::post_processing()	
		{
			if (stream_state_ == STATE_STREAM_MODE)
			{
				cyng::buffer_t buffer(std::istreambuf_iterator<char>(input_), {});
				if (!buffer.empty())
				{
					cb_(cyng::generate_invoke("imega.req.transmit.data", cyng::code::IDENT, buffer));
				}
			}
		}

		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(login& cmd) const
		{
			if (cmd.is_closing(c_))
			{
				//
				//	input is complete.
				//	expect something like:
				//	<PROT:1;VERS:1.0;TELNB:96110845-2;MNAME:95298073>
				//	* protocol
				//	* version
				//	* module name
				//	* meter number
				//
				std::string str;
				std::getline(parser_.input_, str, '\0');

				std::vector<std::string> parts;
				boost::algorithm::split(parts, str, boost::is_any_of(";"), boost::token_compress_on);

				if (parts.size() == 4)
				{
					const int protocol = cmd.read_protocol(parts.at(0));
					const cyng::version ver = cmd.read_version(parts.at(1));
					const std::string name = cmd.read_string(parts.at(2), "TELNB");
					const std::string meter = cmd.read_string(parts.at(3), "MNAME");
					parser_.cb_(cyng::generate_invoke("imega.req.login.public"
						, cyng::code::IDENT
						, protocol
						, ver
						, name
						, meter));

					return STATE_STREAM_MODE;
				}
				//BOOST_ASSERT_MSG(parts.size() == 4, "login with wrong part size");
				parser_.cb_(cyng::generate_invoke("log.msg.error", "login with wrong part size", str));
				return STATE_ERROR;
			}
			
			parser_.input_.put(c_);
			return STATE_LOGIN;
		}

		std::string parser::login::read_string(std::string const& inp, std::string const exp)
		{
			std::vector<std::string> pair;
			boost::algorithm::split(pair, inp, boost::is_any_of(":"), boost::token_compress_on);
			if (pair.size() == 2)
			{
				BOOST_ASSERT_MSG(boost::algorithm::equals(exp, pair.at(0)), "invalid key");
				return pair.at(1);
			}
			return std::string();
		}

		cyng::version parser::login::read_version(std::string const& inp)
		{
			std::vector<std::string> pair;
			boost::algorithm::split(pair, inp, boost::is_any_of(":"), boost::token_compress_on);
			if (pair.size() == 2)
			{
				BOOST_ASSERT_MSG(boost::algorithm::equals("VERS", pair.at(0)), "'VERS' expected");
				return cyng::version(::atof(pair.at(1).c_str()));
			}
			return cyng::version(0, 0);
		}

		int parser::login::read_protocol(std::string const& inp)
		{
			std::vector<std::string> pair;
			boost::algorithm::split(pair, inp, boost::is_any_of(":"), boost::token_compress_on);
			if (pair.size() == 2)
			{
				BOOST_ASSERT_MSG(boost::algorithm::equals("PROT", pair.at(0)), "'PROT' expected");
				return std::stoi(pair.at(1));
			}
			return 0;
		}

		parser::state parser::state_visitor::operator()(stream& cmd) const
		{
			return STATE_LOGIN;

		}

		parser::state parser::state_visitor::operator()(alive& cmd) const
		{
			return STATE_LOGIN;
		}

		parser::login::login()
		{}

		bool parser::login::is_closing(char c)
		{
			return (c == '>');
		}

		parser::stream::stream()
		{}

		bool parser::stream::is_opening(char c)
		{
			return (c == '<');
		}

		parser::alive::alive()
		{}

		bool parser::alive::is_closing(char c)
		{
			return (c == '>');
		}

	}
}	

