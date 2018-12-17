/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/ipt/parser.h>
#include <smf/ipt/codes.h>

#include <iostream>
#include <ios>
#ifdef __DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node 
{
	namespace ipt
	{

		//
		//	parser
		//
		parser::parser(parser_callback cb, scramble_key const& sk)
			: cb_(cb)
			, code_()
			, stream_buffer_()
			, input_(&stream_buffer_)
			, def_sk_(scramble_key::default_scramble_key_)
			, scrambler_()
			, stream_state_(STATE_HEAD)
			, parser_state_()
#ifdef _DEBUG
			, authorized_(false)
#endif
			, read_counter_(0u)
			, header_()
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = command();
			header_.reset(0);
		}

		parser::~parser()
		{
			if (cb_)	cb_ = nullptr;
		}

		std::function<std::string()> parser::get_read_string_f()
		{
			//return std::bind(&parser::read_string, this);
			return [&]()->std::string { return this->read_string(); };
		}

		std::function<std::uint8_t()> parser::get_read_uint8_f()
		{
			return [&]()->std::uint8_t { return this->read_numeric<std::uint8_t>(); };
		}

		std::function<std::uint16_t()> parser::get_read_uint16_f()
		{
			return [&]()->std::uint16_t { return this->read_numeric<std::uint16_t>(); };
		}

		std::function<std::uint32_t()> parser::get_read_uint32_f()
		{
			return [&]()->std::uint32_t { return this->read_numeric<std::uint32_t>(); };
		}

		std::function<std::uint64_t()> parser::get_read_uint64_f()
		{
			return [&]()->std::uint64_t { return this->read_numeric<std::uint64_t>(); };
		}

		std::function<cyng::buffer_t()> parser::get_read_data_f()
		{
			return [&]()->cyng::buffer_t { return this->read_data(); };
		}

		void parser::set_sk(scramble_key const& sk)
		{
			scrambler_ = sk.key();
		}

		void parser::reset(scramble_key const& sk)
		{
			stream_state_ = STATE_HEAD;
			parser_state_ = command();
			header_.reset(0);
			def_sk_ = sk.key();
			scrambler_.reset();

#ifdef _DEBUG
			authorized_ = false;
#endif
			read_counter_ = 0u;
		}

		void parser::clear()
		{
			stream_state_ = STATE_HEAD;
			parser_state_ = command();
			header_.reset(0);
			scrambler_.reset();

			//
			//	clear function objects
			//
			if (cb_)	cb_ = nullptr;

#ifdef _DEBUG
			authorized_ = false;
#endif
			read_counter_ = 0u;
		}

		char parser::put(char c)
		{
			switch (stream_state_)
			{
			case STATE_ESC:
				if (c == ESCAPE_SIGN)
				{
					stream_state_ = STATE_STREAM;
					input_.put(c);
				}
				else
				{
					parser_state_ = command();
					stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				}
				break;
			case STATE_STREAM:
				if (c == ESCAPE_SIGN)
				{
					stream_state_ = STATE_ESC;
				}
				else
				{
					input_.put(c);
				}
				break;
			case STATE_HEAD:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				if (stream_state_ == STATE_DATA)
				{
					//
					//	prepare command
					//
					switch (header_.command_)
					{
					case code::TP_REQ_OPEN_PUSH_CHANNEL:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_REQ_OPEN_PUSH_CHANNEL - not authorized yet");
#endif
						parser_state_ = tp_req_open_push_channel();
						break;
					case code::TP_RES_OPEN_PUSH_CHANNEL:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_RES_OPEN_PUSH_CHANNEL - not authorized yet");
#endif
						parser_state_ = tp_res_open_push_channel();
						break;
					case code::TP_REQ_CLOSE_PUSH_CHANNEL:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_REQ_CLOSE_PUSH_CHANNEL - not authorized yet");
#endif
						parser_state_ = tp_req_close_push_channel();
						break;
					case code::TP_RES_CLOSE_PUSH_CHANNEL:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_RES_CLOSE_PUSH_CHANNEL - not authorized yet");
#endif
						parser_state_ = tp_res_close_push_channel();
						break;
					case code::TP_REQ_PUSHDATA_TRANSFER:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_REQ_PUSHDATA_TRANSFER - not authorized yet");
#endif
						parser_state_ = tp_req_pushdata_transfer();
						break;
					case code::TP_RES_PUSHDATA_TRANSFER:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_RES_PUSHDATA_TRANSFER - not authorized yet");
#endif
						parser_state_ = tp_res_pushdata_transfer();
						break;
					case code::TP_REQ_OPEN_CONNECTION:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_REQ_OPEN_CONNECTION - not authorized yet");
#endif
						parser_state_ = tp_req_open_connection();
						break;
					case code::TP_RES_OPEN_CONNECTION:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_RES_OPEN_CONNECTION - not authorized yet");
#endif
						parser_state_ = tp_res_open_connection();
						break;
					//case code::TP_REQ_CLOSE_CONNECTION:
					//	parser_state_ = tp_req_close_connection();
					//	break;
					case code::TP_RES_CLOSE_CONNECTION:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "TP_RES_CLOSE_CONNECTION - not authorized yet");
#endif
						parser_state_ = tp_res_close_connection();
						break;
					//	open stream channel
					//TP_REQ_OPEN_STREAM_CHANNEL = 0x9006,
					//TP_RES_OPEN_STREAM_CHANNEL = 0x1006,

					//	close stream channel
					//TP_REQ_CLOSE_STREAM_CHANNEL = 0x9007,
					//TP_RES_CLOSE_STREAM_CHANNEL = 0x1007,

					//	stream channel data transfer
					//TP_REQ_STREAMDATA_TRANSFER = 0x9008,
					//TP_RES_STREAMDATA_TRANSFER = 0x1008,

					case code::APP_RES_PROTOCOL_VERSION:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_PROTOCOL_VERSION - not authorized yet");
#endif
						parser_state_ = app_res_protocol_version();
						break;
					case code::APP_RES_SOFTWARE_VERSION:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_SOFTWARE_VERSION - not authorized yet");
#endif
						parser_state_ = app_res_software_version();
						break;
					case code::APP_RES_DEVICE_IDENTIFIER:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_DEVICE_IDENTIFIER - not authorized yet");
#endif
						parser_state_ = app_res_device_identifier();
						break;
					case code::APP_RES_NETWORK_STATUS:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_NETWORK_STATUS - not authorized yet");
#endif
						parser_state_ = app_res_network_status();
						break;
					case code::APP_RES_IP_STATISTICS:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_IP_STATISTICS - not authorized yet");
#endif
						parser_state_ = app_res_ip_statistics();
						break;
					case code::APP_RES_DEVICE_AUTHENTIFICATION:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_DEVICE_AUTHENTIFICATION - not authorized yet");
#endif
						parser_state_ = app_res_device_authentification();
						break;
					case code::APP_RES_DEVICE_TIME:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_DEVICE_TIME - not authorized yet");
#endif
						parser_state_ = app_res_device_time();
						break;
					case code::APP_RES_PUSH_TARGET_NAMELIST:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_PUSH_TARGET_NAMELIST - not authorized yet");
#endif
						parser_state_ = app_res_push_target_namelist();
						break;
					case code::APP_RES_PUSH_TARGET_ECHO:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_PUSH_TARGET_ECHO - not authorized yet");
#endif
						parser_state_ = app_res_push_target_echo();
						break;
					case code::APP_RES_TRACEROUTE:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "APP_RES_TRACEROUTE - not authorized yet");
#endif
						parser_state_ = app_res_traceroute();
						break;

					case code::CTRL_REQ_LOGIN_PUBLIC:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(!authorized_, "CTRL_RES_LOGIN_PUBLIC - already authorized");
#endif
						parser_state_ = ctrl_req_login_public();
						break;
					case code::CTRL_REQ_LOGIN_SCRAMBLED:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(!authorized_, "CTRL_RES_LOGIN_PUBLIC - already authorized");
#endif
						scrambler_ = def_sk_.key();
						parser_state_ = ctrl_req_login_scrambled();
						break;
					case code::CTRL_RES_LOGIN_PUBLIC:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(!authorized_, "CTRL_RES_LOGIN_PUBLIC - already authorized");
#endif
						parser_state_ = ctrl_res_login_public();
						break;
					case code::CTRL_RES_LOGIN_SCRAMBLED:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(!authorized_, "CTRL_RES_LOGIN_PUBLIC - already authorized");
#endif
						parser_state_ = ctrl_res_login_scrambled();
						break;

					case code::CTRL_REQ_LOGOUT:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "CTRL_REQ_LOGOUT - not authorized yet");
#endif
						parser_state_ = ctrl_req_logout();
						break;
					case code::CTRL_RES_LOGOUT:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "CTRL_RES_LOGOUT - not authorized yet");
#endif
						parser_state_ = ctrl_res_logout();
						break;

					case code::CTRL_REQ_REGISTER_TARGET:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "CTRL_REQ_REGISTER_TARGET - not authorized yet");
#endif
						parser_state_ = ctrl_req_register_target();
						break;
					case code::CTRL_RES_REGISTER_TARGET:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "CTRL_RES_REGISTER_TARGET - not authorized yet");
#endif
						parser_state_ = ctrl_res_register_target();
						break;
					case code::CTRL_REQ_DEREGISTER_TARGET:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "CTRL_REQ_DEREGISTER_TARGET - not authorized yet");
#endif
						parser_state_ = ctrl_req_deregister_target();
						break;
					case code::CTRL_RES_DEREGISTER_TARGET:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "CTRL_RES_DEREGISTER_TARGET - not authorized yet");
#endif
						parser_state_ = ctrl_res_deregister_target();
						break;

					//	stream source register
					//CTRL_REQ_REGISTER_STREAM_SOURCE = 0xC00B,
					//CTRL_RES_REGISTER_STREAM_SOURCE = 0x400B,

					//	stream source deregister
					//CTRL_REQ_DEREGISTER_STREAM_SOURCE = 0xC00C,
					//CTRL_RES_DEREGISTER_STREAM_SOURCE = 0x400C,

					case code::UNKNOWN:
#ifdef _DEBUG
						BOOST_ASSERT_MSG(authorized_, "UNKNOWN - not authorized yet");
#endif
						parser_state_ = unknown_cmd();
						break;

					default:
						break;
					}
				}
				else if (stream_state_ == STATE_STREAM)
				{
#ifdef _DEBUG
					BOOST_ASSERT_MSG(authorized_, "STATE_STREAM - not authorized yet");
#endif
					cb_(std::move(code_));
				}
				break;
			case STATE_DATA:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				if (stream_state_ == STATE_STREAM)
				{
					cb_(std::move(code_));

					//
					//	reset header data and prepare
					//	next command
					//
					header_.reset();
					input_.clear();
				}
				break;
			default:
				break;
			}

			return c;
		}

		void parser::post_processing()
		{
			//
			//	increase read counter after the read().
			//
			++read_counter_;

			//
			//	detect data transfers
			//
			if (stream_state_ == STATE_STREAM)
			{
				//
				//	use 
				//	std::istreambuf_iterator<char> eos; 
				//	if {} doesn't work
				cyng::buffer_t buffer(std::istreambuf_iterator<char>(input_), {});
				if (!buffer.empty())
				{
					cb_(cyng::generate_invoke("ipt.req.transmit.data", cyng::code::IDENT, buffer));
				}
			}
		}

		std::string parser::read_string()
		{
			std::string str;
			std::getline(input_, str, '\0');
			input_.clear();	//	clear error flag
			return str;
		}

		scramble_key parser::read_sk()
		{
			scramble_key::key_type value;
			input_.read(value.data(), value.size());
			return value;	//	implicit conversion to scramble_key
		}

		cyng::buffer_t parser::read_data()
		{
			const std::uint32_t size = read_numeric<std::uint32_t>();

			//	allocate data buffer
			cyng::buffer_t data(size);
			input_.read(data.data(), size);
			BOOST_ASSERT_MSG(input_.gcount() == size, "inconsistent push data");
			return data;
		}


		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(command& cmd) const
		{
			BOOST_ASSERT_MSG(cmd.pos_ < HEADER_SIZE, "cmd.pos_ out of range");

			cmd.overlay_[cmd.pos_++] = c_;
			if (cmd.pos_ >= HEADER_SIZE)
			{
				parser_.header_ = convert_to_header(std::begin(cmd.overlay_), std::end(cmd.overlay_));

				//
				//	test for complete command with no data
				//
				switch (parser_.header_.command_)
				{
				case code::TP_REQ_CLOSE_CONNECTION:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "TP_REQ_CLOSE_CONNECTION with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.close.connection", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_PROTOCOL_VERSION:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_PROTOCOL_VERSION with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.protocol.version", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_SOFTWARE_VERSION:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_SOFTWARE_VERSION with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.software.version", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_DEVICE_IDENTIFIER:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_DEVICE_IDENTIFIER with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.device.id", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_NETWORK_STATUS:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_NETWORK_STATUS with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.net.stat", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_IP_STATISTICS:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_IP_STATISTICS with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.ip.statistics", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_DEVICE_AUTHENTIFICATION:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_DEVICE_AUTHENTIFICATION with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.dev.auth", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_DEVICE_TIME:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_DEVICE_TIME with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.dev.time", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				//case code::APP_REQ_PUSH_TARGET_NAMELIST:
				//	BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_PUSH_TARGET_NAMELIST with invalid length");
				//	parser_.code_ << cyng::generate_invoke("ipt.req.push.target.namelist");
				//	return STATE_STREAM;
				case code::APP_REQ_PUSH_TARGET_ECHO:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_PUSH_TARGET_ECHO with invalid length");
					BOOST_ASSERT_MSG(false, "APP_REQ_PUSH_TARGET_ECHO is deprecated");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.push.target.echo", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::APP_REQ_TRACEROUTE:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "APP_REQ_TRACEROUTE with invalid length");
					BOOST_ASSERT_MSG(false, "APP_REQ_TRACEROUTE is deprecated");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.traceroute", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::CTRL_REQ_WATCHDOG:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "CTRL_REQ_WATCHDOG with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.req.watchdog", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				case code::CTRL_RES_WATCHDOG:
					BOOST_ASSERT_MSG((size(parser_.header_) == 0), "CTRL_RES_WATCHDOG with invalid length");
					parser_.code_ << cyng::unwind<cyng::vector_t>(cyng::generate_invoke("ipt.res.watchdog", cyng::code::IDENT, parser_.header_.sequence_));
					return STATE_STREAM;
				default:
					if (parser_.header_.length_ == 0) {
						parser_.code_ << cyng::generate_invoke_unwinded("log.msg.fatal", "unknown IP-T command", parser_.header_.command_);
						BOOST_ASSERT_MSG(false, "unknown IP-T command");
					}
					break;
				}

				cmd.reset();
				return STATE_DATA;
			}
			return STATE_HEAD;
		}

		parser::state parser::state_visitor::operator()(tp_req_open_push_channel& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.req.open.push.channel"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_string_f()	//	target name
					, parser_.get_read_string_f()	//	account
					, parser_.get_read_string_f()	//	number
					, parser_.get_read_string_f()	//	version
					, parser_.get_read_string_f()	//	device id
					, parser_.get_read_uint16_f()) //	time out
					<< cyng::unwind_vec(10);

				req.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_open_push_channel& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.open.push.channel"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f()	//	response
					, parser_.get_read_uint32_f()	//	channel id
					, parser_.get_read_uint32_f()	//	source id (session tag)
					, parser_.get_read_uint16_f()	//	packet size
					, parser_.get_read_uint8_f()	//	window size
					, parser_.get_read_uint8_f()	//	status
					, parser_.get_read_uint32_f())	//	target count
					<< cyng::unwind_vec();

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_req_close_push_channel& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.req.close.push.channel"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint32_f())	//	channel id
					<< cyng::unwind_vec();

				req.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_close_push_channel& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.close.push.channel"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f()		//	response
					, parser_.get_read_uint32_f());	//	channel id
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_req_pushdata_transfer& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
#ifdef __DEBUG
				//	get content of buffer
				boost::asio::const_buffer cbuffer(*parser_.stream_buffer_.data().begin());
				const char* p = boost::asio::buffer_cast<const char*>(cbuffer);
				const std::size_t size = boost::asio::buffer_size(cbuffer);

				std::stringstream ss;
				cyng::io::hex_dump hd;
				hd(ss, p, p + size);
				parser_.code_ << cyng::generate_invoke_unwinded("log.msg.trace", "dump pushdata", size, ss.str());
#endif

				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.transfer.pushdata"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint32_f()	//	channel id
					, parser_.get_read_uint32_f()	//	source id (session tag)
					, parser_.get_read_uint8_f()	//	status
					, parser_.get_read_uint8_f()	//	block
					, parser_.get_read_data_f());	//	size + data

				req.reset();
				return STATE_STREAM;
			}
#ifdef __DEBUG
			else
			{
				parser_.code_ << cyng::generate_invoke_unwinded("log.msg.trace", "dump pushdata", req.pos_, +c_);

			}
#endif
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_pushdata_transfer& res) const
		{

			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.transfer.pushdata"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f()	//	response code
					, parser_.get_read_uint32_f()	//	channel id
					, parser_.get_read_uint32_f()	//	source id (session tag)
					, parser_.get_read_uint8_f()	//	status
					, parser_.get_read_uint8_f())	//	block
					<< cyng::unwind_vec();

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_req_open_connection& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.open.connection"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_string_f());	//	number

				req.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_open_connection& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.open.connection"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f())	//	response
					<< cyng::unwind_vec();
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_close_connection& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.close.connection"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f());	//	response
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(app_res_protocol_version& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.protocol.version"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f());
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_software_version& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.software.version"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_string_f())
					<< cyng::unwind_vec();
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(app_res_device_identifier& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.dev.id"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_string_f());
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_network_status& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				//
				//	Note: no response code
				//
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.network.stat"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f()	//	device type
					, parser_.get_read_uint32_f()	//	status_vector	status_[ 0 ];
					, parser_.get_read_uint32_f()	//	status_vector	status_[ 1 ];
					, parser_.get_read_uint32_f()	//	status_vector	status_[ 2 ];
					, parser_.get_read_uint32_f()	//	status_vector	status_[ 3 ];
					, parser_.get_read_uint32_f()	//	status_vector	status_[ 4 ];
					, parser_.get_read_string_f()  //	//	IMSI
					, parser_.get_read_string_f());  //	//	IMEI
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_ip_statistics& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.ip.statistics"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f()	//	response type
					, parser_.get_read_uint64_f()		//	rx
					, parser_.get_read_uint64_f());	//	tx
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_device_authentification& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.dev.auth"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_string_f()	//	account
					, parser_.get_read_string_f()		//	password
					, parser_.get_read_string_f()		//	number
					, parser_.get_read_string_f());	//	description
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_device_time& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.dev.time"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint32_f());	//	seconds since 1970
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_push_target_namelist& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				//
				//	response
				//
				const auto response = parser_.read_numeric<std::uint8_t>();

				//
				//	array size
				//
				const auto size = parser_.read_numeric<std::uint32_t>();

				//
				//	open frame
				//
				parser_.code_ << cyng::code::ESBA;

				//
				//	AAA Style: https://herbsutter.com/2013/08/12/gotw-94-solution-aaa-style-almost-always-auto/
				//
				for (auto idx = decltype(size){0}; idx < size; ++idx)
				{
					parser_.code_
						<< parser_.get_read_string_f()	//	target name
						<< parser_.get_read_string_f()	//	device name
						<< parser_.get_read_string_f()	//	device number
						<< parser_.get_read_string_f()	//	software version
						<< parser_.get_read_string_f()	//	device id
						<< parser_.get_read_uint8_f()	//	transfer status
						;
				}

				parser_.code_ 
					<< size
					<< response
					<< cyng::invoke("ipt.res.push.target.namelist")
					<< cyng::code::REBA	//	close frame
					;

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_push_target_echo& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_
					<< cyng::generate_invoke_unwinded("ipt.res.push.target.echo"
						, cyng::code::IDENT
						, parser_.get_read_uint32_f() //	push channel number
						, parser_.get_read_data_f());	//	read push data

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_traceroute& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_
					<< cyng::generate_invoke_unwinded("app.res.traceroute"
						, cyng::code::IDENT
						, parser_.get_read_uint16_f() //	traceroute index
						, parser_.get_read_uint16_f()); //	hop counter
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}


		parser::state parser::state_visitor::operator()(ctrl_req_login_public& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.req.login.public"
					, cyng::code::IDENT
					, parser_.get_read_string_f()	//	name
					, parser_.get_read_string_f())	//	password
					<< cyng::unwind_vec();

#ifdef _DEBUG
				parser_.authorized_ = true;
#endif
				req.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_req_login_scrambled& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
				const auto name = parser_.read_string();
				const auto pwd = parser_.read_string();
				const auto sk = parser_.read_sk();
				parser_.scrambler_ = sk.key();
				parser_.code_
					<< cyng::generate_invoke("ipt.set.sk.def", sk)
					<< cyng::generate_invoke("ipt.req.login.scrambled"
						, cyng::code::IDENT
						, name	//	name
						, pwd	//	password
						, sk)	//	scramble key
					<< cyng::unwind_vec()
					;
				
#ifdef _DEBUG
				parser_.authorized_ = true;
#endif
				req.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_res_login_public& res) const
		{

			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.login.public"
					, cyng::code::IDENT
					, parser_.get_read_uint8_f()	//	response
					, parser_.get_read_uint16_f()	//	watchdog
					, parser_.get_read_string_f())//	redirect
					<< cyng::unwind_vec()
					;

#ifdef _DEBUG
				parser_.authorized_ = true;
#endif
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_res_login_scrambled& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.login.scrambled"
					, cyng::code::IDENT
					, parser_.get_read_uint8_f()	//	response
					, parser_.get_read_uint16_f()	//	watchdog
					, parser_.get_read_string_f());	//	redirect

#ifdef _DEBUG
				parser_.authorized_ = true;
#endif
				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_req_logout& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.logout"
					, cyng::code::IDENT
					, parser_.get_read_uint8_f());	//	reason

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_res_logout& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.logout"
					, cyng::code::IDENT
					, parser_.get_read_uint8_f());	//	response

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_req_register_target& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.register.push.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_string_f()	//	target
					, parser_.get_read_uint16_f()	//	packet size
					, parser_.get_read_uint8_f());	//	window size

				req.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_res_register_target& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.register.push.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f()		//	response
					, parser_.get_read_uint32_f());	//	channel

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_req_deregister_target& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.deregister.push.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_string_f());	//target name

				req.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_res_deregister_target& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.deregister.push.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint8_f()	//	response
					, parser_.get_read_string_f());	//target name

				res.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state  parser::state_visitor::operator()(unknown_cmd& cmd) const
		{
			cmd.pos_++;
			parser_.input_.put(c_);
			if (cmd.pos_ >= size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.unknown.cmd"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.get_read_uint16_f());		//	command code

				cmd.reset();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		void parser::base::reset()
		{
			pos_ = 0;
		}

		void parser::command::reset()
		{
			base::reset();
			std::fill(std::begin(overlay_), std::end(overlay_), 0);
		}

	}
}	//	node

