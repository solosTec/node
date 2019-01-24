/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/modem/parser.h>
#include <cyng/vm/generator.h>
#include <iostream>
#include <ios>
#include <boost/algorithm/string.hpp>

namespace node 
{
	namespace modem
	{

		//
		//	parser
		//
		parser::parser(parser_callback cb, std::chrono::milliseconds guard_time)
			: cb_(cb)
			, guard_time_(guard_time)
			, code_()
			, stream_state_(STATE_COMMAND)
			, parser_state_()
			, stream_buffer_()
			, input_(&stream_buffer_)
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = command();
		}

		parser::~parser()
		{
			if (cb_)	cb_ = nullptr;
		}

		char parser::put(char c)
		{
			state prev_state = stream_state_;
			switch (stream_state_)
			{
			case STATE_COMMAND:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			case STATE_STREAM:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			case STATE_ESC:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			default:
				break;
			}

			if (stream_state_ != prev_state)
			{
				switch (stream_state_)
				{
				case STATE_COMMAND:
					parser_state_ = command();
					break;
				case STATE_STREAM:
					parser_state_ = stream();
					break;
				case STATE_ESC:
					parser_state_ = esc();
					break;
				default:
					break;
				}
			}
			return c;
		}

		void parser::post_processing()
		{
			if (stream_state_ == STATE_STREAM)
			{
				//
				//	use 
				//	std::istreambuf_iterator<char> eos; 
				//	if {} doesn't work
				cyng::buffer_t buffer(std::istreambuf_iterator<char>(input_), {});
				if (!buffer.empty())
				{
					cb_(cyng::generate_invoke("modem.req.transmit.data", cyng::code::IDENT, buffer));
				}
			}
		}

		void parser::set_stream_mode()
		{
			stream_state_ = STATE_STREAM;
			parser_state_ = stream();
		}

		void parser::set_cmd_mode()
		{
			post_processing();
			stream_state_ = STATE_COMMAND;
			parser_state_ = command();
		}

		void parser::clear()
		{
			code_.clear();
			stream_state_ = STATE_COMMAND;
			parser_state_ = command();
			input_.clear();

			//
			//	clear function objects
			//
			if (cb_)	cb_ = nullptr;
		}

		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(command& cmd) const
		{
			if (cmd.is_eol(c_))
			{
				//
				//	input is complete
				//
				std::string str;
				std::getline(parser_.input_, str, '\0');

				//	clear error flag
				parser_.input_.clear();	

				//
				//	parse hayes/AT command
				//
				return cmd.parse(parser_, str);

			}
			parser_.input_.put(c_);
			return STATE_COMMAND;
		}

		parser::state parser::state_visitor::operator()(stream& cmd) const
		{
			if (cmd.is_esc(c_))
			{
				return STATE_ESC;
			}
			parser_.input_.put(c_);
			return STATE_STREAM;
		}

		parser::state parser::state_visitor::operator()(esc& cmd) const
		{
			if (cmd.is_esc(c_))
			{
				cmd.counter_++;
				auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - cmd.start_);
				if (cmd.counter_ == 3)
				{
					if (delta > parser_.guard_time_)
					{
						parser_.cb_(cyng::generate_invoke("print.ok"));
						parser_.cb_(cyng::generate_invoke("stream.flush"));
						parser_.cb_(cyng::generate_invoke("log.msg.debug", "switch parser in command mode"));
						return STATE_COMMAND;
					}
					else
					{
						for (std::size_t idx = 0; idx < cmd.counter_; ++idx)
						{
							parser_.input_.put('+');
						}
						return STATE_STREAM;
					}
				}
				else
				{
					return STATE_ESC;
				}
			}

			for (std::size_t idx = 0; idx < cmd.counter_; ++idx)
			{
				parser_.input_.put('+');
			}
			return STATE_STREAM;
		}

		parser::command::command()
		{};

		bool parser::command::is_eol(char c)
		{
			return (c == '\n')
				|| (c == '\r')
				;
		}

		parser::state parser::command::parse(parser& p, std::string inp)
		{
			//
			//	optimization hint: instead of boost::algorithm::erase_head() an
			//	iterator that would be incremented could be faster.
			//

			if (inp.empty())
			{
				return STATE_COMMAND;
			}
			else if (boost::algorithm::starts_with(inp, "AT") || boost::algorithm::starts_with(inp, "at"))
			{
				boost::algorithm::erase_head(inp, 2);
				p.cb_(cyng::generate_invoke("log.msg.debug", "parsing command", inp));

				if (inp.empty())
				{
					p.cb_(cyng::generate_invoke("print.ok"));
					p.cb_(cyng::generate_invoke("stream.flush"));
				}
				else if (boost::algorithm::starts_with(inp, "+"))
				{
					//AT+name?password
					boost::algorithm::erase_head(inp, 1);
					std::vector<std::string> result;
					boost::algorithm::split(result, inp, boost::is_any_of("?"), boost::token_compress_on);
					if (result.size() == 2)
					{
						p.cb_(cyng::generate_invoke("modem.req.login.public"
							, cyng::code::IDENT
							, result.at(0)
							, result.at(1)));
					}
					else
					{
						p.cb_(cyng::generate_invoke("log.msg.error", "AT+name?password expected but got", inp));
						p.cb_(cyng::generate_invoke("print.error"));
						p.cb_(cyng::generate_invoke("stream.flush"));
					}
				}
				else if (boost::algorithm::starts_with(inp, "D") || boost::algorithm::starts_with(inp, "d"))
				{
					//	dial up
					boost::algorithm::erase_head(inp, 1);
					p.cb_(cyng::generate_invoke("modem.req.open.connection"
						, cyng::code::IDENT
						, inp));

				}
				else if (boost::algorithm::starts_with(inp, "H") || boost::algorithm::starts_with(inp, "h"))
				{
					//	ATH - hang up
					boost::algorithm::erase_head(inp, 1);
					p.cb_(cyng::generate_invoke("modem.req.close.connection"
						, cyng::code::IDENT));

				}
				else if (boost::algorithm::starts_with(inp, "I") || boost::algorithm::starts_with(inp, "i"))
				{
					//	ATI - informational output
					boost::algorithm::erase_head(inp, 1);

					try {
						const auto info = std::stoul(inp);
						p.cb_(cyng::generate_invoke("modem.req.info"
							, cyng::code::IDENT
							, static_cast<std::uint32_t>(info)));
					}
					catch (std::exception const& ex) {
						p.cb_(cyng::generate_invoke("modem.req.info"
							, cyng::code::IDENT
							, static_cast<std::uint32_t>(0u)));
					}

				}
				else if (boost::algorithm::starts_with(inp, "O") || boost::algorithm::starts_with(inp, "o"))
				{
					//	return to stream mode
					return STATE_STREAM;
				}
				else
				{
					p.cb_(cyng::generate_invoke("log.msg.error", "unknown AT command", inp));
					p.cb_(cyng::generate_invoke("print.error"));
					p.cb_(cyng::generate_invoke("stream.flush"));

				}
			}
			else
			{
				p.cb_(cyng::generate_invoke("log.msg.error", "AT expected but got", inp));
				p.cb_(cyng::generate_invoke("print.error"));
				p.cb_(cyng::generate_invoke("stream.flush"));
			}
			return STATE_COMMAND;
		}

		parser::stream::stream()
		{};

		bool parser::stream::is_esc(char c)
		{
			return (c == '+');
		}

		parser::esc::esc()
			: start_(std::chrono::system_clock::now())
			, counter_(1)
		{};

		parser::esc::esc(esc const& other)
			: start_(other.start_)
			, counter_(other.counter_)
		{}

		bool parser::esc::is_esc(char c)
		{
			return (c == '+');
		}

	}
}	//	node

