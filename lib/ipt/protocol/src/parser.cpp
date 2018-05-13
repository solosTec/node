/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/ipt/parser.h>
#include <smf/ipt/codes.h>
#include <cyng/vm/generator.h>
#include <iostream>
#include <ios>

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
			, header_()
			, f_read_string(std::bind(std::bind(&parser::read_string, this)))
			, f_read_uint8(std::bind(std::bind(&parser::read_numeric<std::uint8_t>, this)))
			, f_read_uint16(std::bind(std::bind(&parser::read_numeric<std::uint16_t>, this)))
			, f_read_uint32(std::bind(std::bind(&parser::read_numeric<std::uint32_t>, this)))
			, f_read_uint64(std::bind(std::bind(&parser::read_numeric<std::uint64_t>, this)))
			, f_read_data(std::bind(std::bind(&parser::read_data, this)))
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = command();
			header_.reset(0);
		}

		parser::~parser()
		{
            parser_callback pcb;
            cb_.swap(pcb);
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
						parser_state_ = tp_req_open_push_channel();
						break;
					case code::TP_RES_OPEN_PUSH_CHANNEL:
						parser_state_ = tp_res_open_push_channel();
						break;
					case code::TP_REQ_CLOSE_PUSH_CHANNEL:
						parser_state_ = tp_req_close_push_channel();
						break;
					case code::TP_RES_CLOSE_PUSH_CHANNEL:
						parser_state_ = tp_res_close_push_channel();
						break;
					case code::TP_REQ_PUSHDATA_TRANSFER:
						parser_state_ = tp_req_pushdata_transfer();
						break;
					case code::TP_RES_PUSHDATA_TRANSFER:
						parser_state_ = tp_res_pushdata_transfer();
						break;
					case code::TP_REQ_OPEN_CONNECTION:
						parser_state_ = tp_req_open_connection();
						break;
					case code::TP_RES_OPEN_CONNECTION:
						parser_state_ = tp_res_open_connection();
						break;
					//case code::TP_REQ_CLOSE_CONNECTION:
					//	parser_state_ = tp_req_close_connection();
					//	break;
					case code::TP_RES_CLOSE_CONNECTION:
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
						parser_state_ = app_res_protocol_version();
						break;
					case code::APP_RES_SOFTWARE_VERSION:
						parser_state_ = app_res_software_version();
						break;
					case code::APP_RES_DEVICE_IDENTIFIER:
						parser_state_ = app_res_device_identifier();
						break;
					case code::APP_RES_NETWORK_STATUS:
						parser_state_ = app_res_network_status();
						break;
					case code::APP_RES_IP_STATISTICS:
						parser_state_ = app_res_ip_statistics();
						break;
					case code::APP_RES_DEVICE_AUTHENTIFICATION:
						parser_state_ = app_res_device_authentification();
						break;
					case code::APP_RES_DEVICE_TIME:
						parser_state_ = app_res_device_time();
						break;
					case code::APP_RES_PUSH_TARGET_NAMELIST:
						parser_state_ = app_res_push_target_namelist();
						break;
					case code::APP_RES_PUSH_TARGET_ECHO:
						parser_state_ = app_res_push_target_echo();
						break;
					case code::APP_RES_TRACEROUTE:
						parser_state_ = app_res_traceroute();
						break;

					case code::CTRL_REQ_LOGIN_PUBLIC:
						parser_state_ = ctrl_req_login_public();
						break;
					case code::CTRL_REQ_LOGIN_SCRAMBLED:
						scrambler_ = def_sk_.key();
						parser_state_ = ctrl_req_login_scrambled();
						break;
					case code::CTRL_RES_LOGIN_PUBLIC:
						parser_state_ = ctrl_res_login_public();
						break;
					case code::CTRL_RES_LOGIN_SCRAMBLED:
						parser_state_ = ctrl_res_login_scrambled();
						break;

					case code::CTRL_REQ_LOGOUT:
						parser_state_ = ctrl_req_logout();
						break;
					case code::CTRL_RES_LOGOUT:
						parser_state_ = ctrl_res_logout();
						break;

					case code::CTRL_REQ_REGISTER_TARGET:
						parser_state_ = ctrl_req_register_target();
						break;
					case code::CTRL_RES_REGISTER_TARGET:
						parser_state_ = ctrl_res_register_target();
						break;
					case code::CTRL_REQ_DEREGISTER_TARGET:
						parser_state_ = ctrl_req_deregister_target();
						break;
					case code::CTRL_RES_DEREGISTER_TARGET:
						parser_state_ = ctrl_res_deregister_target();
						break;

					//	stream source register
					//CTRL_REQ_REGISTER_STREAM_SOURCE = 0xC00B,
					//CTRL_RES_REGISTER_STREAM_SOURCE = 0x400B,

					//	stream source deregister
					//CTRL_REQ_DEREGISTER_STREAM_SOURCE = 0xC00C,
					//CTRL_RES_DEREGISTER_STREAM_SOURCE = 0x400C,

					case code::UNKNOWN:
						parser_state_ = unknown_cmd();
						break;

					default:
						break;
					}
				}
				else if (stream_state_ == STATE_STREAM)
				{
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
				}
				break;
			default:
				break;
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
			cmd.overlay_[cmd.pos_++] = c_;
			if (cmd.pos_ == HEADER_SIZE)
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
					break;
				}

				return STATE_DATA;
			}
			return STATE_HEAD;
		}

		parser::state parser::state_visitor::operator()(tp_req_open_push_channel& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.req.open.push.channel"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_string	//	target name
					, parser_.f_read_string	//	account
					, parser_.f_read_string	//	number
					, parser_.f_read_string	//	version
					, parser_.f_read_string	//	device id
					, parser_.f_read_uint16) //	time out
					<< cyng::unwind_vec(10);

				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_open_push_channel& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.open.push.channel"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8	//	response
					, parser_.f_read_uint32	//	channel id
					, parser_.f_read_uint32	//	source id (session tag)
					, parser_.f_read_uint16	//	packet size
					, parser_.f_read_uint8	//	window size
					, parser_.f_read_uint8	//	status
					, parser_.f_read_uint32)	//	target count
					<< cyng::unwind_vec();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_req_close_push_channel& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.req.close.push.channel"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint32)	//	channel id
					<< cyng::unwind_vec();
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
					, parser_.f_read_uint8		//	response
					, parser_.f_read_uint32);	//	channel id
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_req_pushdata_transfer& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.transfer.pushdata"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint32	//	channel id
					, parser_.f_read_uint32	//	source id (session tag)
					, parser_.f_read_uint8	//	status
					, parser_.f_read_uint8	//	block
					, parser_.f_read_data);	//	size + data
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_pushdata_transfer& res) const
		{

			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.transfer.pushdata"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8	//	response code
					, parser_.f_read_uint32	//	channel id
					, parser_.f_read_uint32	//	source id (session tag)
					, parser_.f_read_uint8	//	status
					, parser_.f_read_uint8)	//	block
					<< cyng::unwind_vec();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_req_open_connection& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.open.connection"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_string);	//	number
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_open_connection& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.open.connection"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8)	//	response
					<< cyng::unwind_vec();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(tp_res_close_connection& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.close.connection"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8);	//	response
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(app_res_protocol_version& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.protocol.version"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8);
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_software_version& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.software.version"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_string)
					<< cyng::unwind_vec();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(app_res_device_identifier& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.dev.id"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_string);
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_network_status& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				//
				//	Note: no response code
				//
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.network.stat"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8	//	device type
					, parser_.f_read_uint32	//	status_vector	status_[ 0 ];
					, parser_.f_read_uint32	//	status_vector	status_[ 1 ];
					, parser_.f_read_uint32	//	status_vector	status_[ 2 ];
					, parser_.f_read_uint32	//	status_vector	status_[ 3 ];
					, parser_.f_read_uint32	//	status_vector	status_[ 4 ];
					, parser_.f_read_string  //	//	IMSI
					, parser_.f_read_string);  //	//	IMEI
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_ip_statistics& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.ip.statistics"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8	//	response type
					, parser_.f_read_uint64		//	rx
					, parser_.f_read_uint64);	//	tx
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_device_authentification& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.dev.auth"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_string	//	account
					, parser_.f_read_string		//	password
					, parser_.f_read_string		//	number
					, parser_.f_read_string);	//	description
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_device_time& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.dev.time"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint32);	//	seconds since 1970
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_push_target_namelist& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
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

				for (auto idx = 0; idx < size; ++idx)
				{
					parser_.code_
						<< parser_.f_read_string	//	target name
						<< parser_.f_read_string	//	device name
						<< parser_.f_read_string	//	device number
						<< parser_.f_read_string	//	software version
						<< parser_.f_read_string	//	device id
						<< parser_.f_read_uint8	//	transfer status
						;
				}

				parser_.code_ 
					<< size
					<< response
					<< cyng::invoke("ipt.res.push.target.namelist")
					<< cyng::code::REBA	//	close frame
					;
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_push_target_echo& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_
					<< cyng::generate_invoke_unwinded("ipt.res.push.target.echo"
						, cyng::code::IDENT
						, parser_.f_read_uint32 //	push channel number
						, parser_.f_read_data);	//	read push data

				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(app_res_traceroute& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_
					<< cyng::generate_invoke_unwinded("app.res.traceroute"
						, cyng::code::IDENT
						, parser_.f_read_uint16 //	traceroute index
						, parser_.f_read_uint16); //	hop counter
				return STATE_STREAM;
			}
			return STATE_DATA;
		}


		parser::state parser::state_visitor::operator()(ctrl_req_login_public& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.req.login.public"
					, cyng::code::IDENT
					, parser_.f_read_string	//	name
					, parser_.f_read_string)	//	password
					<< cyng::unwind_vec();
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_req_login_scrambled& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
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
				
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_res_login_public& res) const
		{

			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke("ipt.res.login.public"
					, cyng::code::IDENT
					, parser_.f_read_uint8	//	response
					, parser_.f_read_uint16	//	watchdog
					, parser_.f_read_string)//	redirect
					<< cyng::unwind_vec()
					;	
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_res_login_scrambled& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.login.scrambled"
					, cyng::code::IDENT
					, parser_.f_read_uint8	//	response
					, parser_.f_read_uint16	//	watchdog
					, parser_.f_read_string);	//	redirect
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_req_logout& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.logout"
					, cyng::code::IDENT
					, parser_.f_read_uint8);	//	reason
				return STATE_STREAM;
			}
			return STATE_DATA;
		}
		parser::state parser::state_visitor::operator()(ctrl_res_logout& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.logout"
					, cyng::code::IDENT
					, parser_.f_read_uint8);	//	response
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_req_register_target& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.register.push.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_string		//	target
					, parser_.f_read_uint16		//	packet size
					, parser_.f_read_uint8);	//	window size
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_res_register_target& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.register.push.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8		//	response
					, parser_.f_read_uint32);	//	channel
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_req_deregister_target& req) const
		{
			req.pos_++;
			parser_.input_.put(c_);
			if (req.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.req.deregister.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_string);	//target name
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state parser::state_visitor::operator()(ctrl_res_deregister_target& res) const
		{
			res.pos_++;
			parser_.input_.put(c_);
			if (res.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.res.deregister.target"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint8		//	response
					, parser_.f_read_string);	//target name
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

		parser::state  parser::state_visitor::operator()(unknown_cmd& cmd) const
		{
			cmd.pos_++;
			parser_.input_.put(c_);
			if (cmd.pos_ == size(parser_.header_))
			{
				parser_.code_ << cyng::generate_invoke_unwinded("ipt.unknown.cmd"
					, cyng::code::IDENT
					, parser_.header_.sequence_
					, parser_.f_read_uint16);		//	command code
				return STATE_STREAM;
			}
			return STATE_DATA;
		}

	}
}	//	node

