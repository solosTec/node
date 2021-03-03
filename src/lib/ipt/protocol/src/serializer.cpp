/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/serializer.h>


//#include <cyng/vm/generator.h>
//#include <cyng/value_cast.hpp>
//#include <cyng/io/serializer.h>
//#include <cyng/buffer_cast.h>
//
//#ifdef SMF_IO_DEBUG
//#include <cyng/io/hex_dump.hpp>
//#endif
//#include <cyng/tuple_cast.hpp>
#include <iterator>
#include <algorithm>

#include <boost/predef.h>

namespace smf {

	namespace ipt {

		serializer::serializer(scramble_key const& def)
		: buffer_()
			, sgen_()
			, last_seq_(0)
			, scrambler_()
			, def_key_(def)
		{
//			vm.register_function("stream.flush", 0, [this, &s](cyng::context& ctx) {
//
//#ifdef SMF_IO_DEBUG
//				//	get content of buffer
//				boost::asio::const_buffer cbuffer(*buffer_.data().begin());
//				const char* p = boost::asio::buffer_cast<const char*>(cbuffer);
//				const std::size_t size = boost::asio::buffer_size(cbuffer);
//
//				cyng::io::hex_dump hd;
//				hd(std::cerr, p, p + size);
//#endif
//				//BOOST_ASSERT(s.is_open());
//
//				boost::system::error_code ec;
//				boost::asio::write(s, buffer_, ec);
//				ctx.set_register(ec);
//			});
//
//			vm.register_function("stream.serialize", 0, [this](cyng::context& ctx) {
//
//				const cyng::vector_t frame = ctx.get_frame();
//				for (auto obj : frame)
//				{
//					cyng::io::serialize_binary(ostream_, obj);
//				}
//
//			});

			//
			//	generated from parser
			//
			//vm.register_function("ipt.set.sk.def", 1, std::bind(&serializer::set_sk, this, std::placeholders::_1));

			////
			////	after reconnect
			////
			//vm.register_function("ipt.reset.serializer", 1, std::bind(&serializer::reset, this, std::placeholders::_1));

			////
			////	push last ipt sequece number on stack
			////
			//vm.register_function("ipt.seq.push", 0, std::bind(&serializer::push_seq, this, std::placeholders::_1));

			////
			////	transfer data
			////
			//vm.register_function("ipt.transfer.data", 1, std::bind(&serializer::transfer_data, this, std::placeholders::_1));

			////
			////	generated from master node (mostly)
			////
			//vm.register_function("req.login.public", 2, std::bind(&serializer::req_login_public, this, std::placeholders::_1));
			//vm.register_function("req.login.scrambled", 3, std::bind(&serializer::req_login_scrambled, this, std::placeholders::_1));
			//vm.register_function("res.login.public", 3, std::bind(&serializer::res_login_public, this, std::placeholders::_1));
			//vm.register_function("res.login.scrambled", 3, std::bind(&serializer::res_login_scrambled, this, std::placeholders::_1));

			//vm.register_function("req.open.push.channel", 6, std::bind(&serializer::req_open_push_channel, this, std::placeholders::_1));
			//vm.register_function("res.open.push.channel", 8, std::bind(&serializer::res_open_push_channel, this, std::placeholders::_1));

			//vm.register_function("req.close.push.channel", 1, std::bind(&serializer::req_close_push_channel, this, std::placeholders::_1));
			//vm.register_function("res.close.push.channel", 3, std::bind(&serializer::res_close_push_channel, this, std::placeholders::_1));

			//vm.register_function("req.transfer.push.data", 5, std::bind(&serializer::req_transfer_push_data, this, std::placeholders::_1));
			//vm.register_function("res.transfer.push.data", 6, std::bind(&serializer::res_transfer_push_data, this, std::placeholders::_1));

			//vm.register_function("req.open.connection", 1, std::bind(&serializer::req_open_connection, this, std::placeholders::_1));
			//vm.register_function("res.open.connection", 2, std::bind(&serializer::res_open_connection, this, std::placeholders::_1));

			//vm.register_function("req.close.connection", 0, std::bind(&serializer::req_close_connection, this, std::placeholders::_1));
			//vm.register_function("res.close.connection", 2, std::bind(&serializer::res_close_connection, this, std::placeholders::_1));

			//vm.register_function("req.protocol.version", 0, std::bind(&serializer::req_protocol_version, this, std::placeholders::_1));
			//vm.register_function("res.protocol.version", 2, std::bind(&serializer::res_protocol_version, this, std::placeholders::_1));

			//vm.register_function("req.software.version", 0, std::bind(&serializer::req_software_version, this, std::placeholders::_1));
			//vm.register_function("res.software.version", 2, std::bind(&serializer::res_software_version, this, std::placeholders::_1));

			//vm.register_function("req.device.id", 0, std::bind(&serializer::req_device_id, this, std::placeholders::_1));
			//vm.register_function("res.device.id", 2, std::bind(&serializer::res_device_id, this, std::placeholders::_1));

			//vm.register_function("req.net.status", 0, std::bind(&serializer::req_network_status, this, std::placeholders::_1));
			//vm.register_function("res.net.status", 9, std::bind(&serializer::res_network_status, this, std::placeholders::_1));

			//vm.register_function("req.ip.statistics", 0, std::bind(&serializer::req_ip_statistics, this, std::placeholders::_1));
			//vm.register_function("res.ip.statistics", 3, std::bind(&serializer::res_ip_statistics, this, std::placeholders::_1));

			//vm.register_function("req.device.auth", 0, std::bind(&serializer::req_device_auth, this, std::placeholders::_1));
			//vm.register_function("res.device.auth", 5, std::bind(&serializer::res_device_auth, this, std::placeholders::_1));

			//vm.register_function("req.device.time", 0, std::bind(&serializer::req_device_time, this, std::placeholders::_1));
			//vm.register_function("res.device.time", 1, std::bind(&serializer::res_device_time, this, std::placeholders::_1));

			//vm.register_function("req.push.target.namelist", 1, std::bind(&serializer::req_push_target_namelist, this, std::placeholders::_1));
			//vm.register_function("res.push.target.namelist", 1, std::bind(&serializer::res_push_target_namelist, this, std::placeholders::_1));

			//vm.register_function("req.push.target.echo", 1, std::bind(&serializer::req_push_target_echo, this, std::placeholders::_1));
			//vm.register_function("res.push.target.echo", 1, std::bind(&serializer::res_push_target_echo, this, std::placeholders::_1));

			//vm.register_function("req.traceroute", 4, std::bind(&serializer::req_traceroute, this, std::placeholders::_1));
			//vm.register_function("res.traceroute", 1, std::bind(&serializer::res_traceroute, this, std::placeholders::_1));

			//vm.register_function("req.maintenance", 1, std::bind(&serializer::req_maintenance, this, std::placeholders::_1));
			//vm.register_function("res.maintenance", 1, std::bind(&serializer::res_maintenance, this, std::placeholders::_1));

			//vm.register_function("req.logout", 1, std::bind(&serializer::req_logout, this, std::placeholders::_1));
			//vm.register_function("res.logout", 1, std::bind(&serializer::res_logout, this, std::placeholders::_1));

			//vm.register_function("req.register.push.target", 3, std::bind(&serializer::req_register_push_target, this, std::placeholders::_1));
			//vm.register_function("res.register.push.target", 3, std::bind(&serializer::res_register_push_target, this, std::placeholders::_1));

			//vm.register_function("req.deregister.push.target", 1, std::bind(&serializer::req_deregister_push_target, this, std::placeholders::_1));
			//vm.register_function("res.deregister.push.target", 3, std::bind(&serializer::res_deregister_push_target, this, std::placeholders::_1));

			//vm.register_function("req.watchdog", 1, std::bind(&serializer::req_watchdog, this, std::placeholders::_1));
			//vm.register_function("res.watchdog", 1, std::bind(&serializer::res_watchdog, this, std::placeholders::_1));

			//vm.register_function("req.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1));
			//vm.register_function("res.multi.ctrl.public.login", 1, std::bind(&serializer::req_multi_ctrl_public_login, this, std::placeholders::_1));

			////	control - multi public login request
			//MULTI_CTRL_REQ_LOGIN_SCRAMBLED = 0xC00A,	//!<	request
			//MULTI_CTRL_RES_LOGIN_SCRAMBLED = 0x400A,	//!<	response

			////	server mode
			//SERVER_MODE_REQUEST = 0xC010,	//!<	request
			//SERVER_MODE_RESPONSE = 0x4010,	//!<	response

			////	server mode reconnect
			//SERVER_MODE_RECONNECT_REQUEST = 0xC011,	//!<	request
			//SERVER_MODE_RECONNECT_RESPONSE = 0x4011,	//!<	response

			//UNKNOWN = 0x7fff,	//!<	unknown command
			//vm.register_function("res.unknown.command", 1, std::bind(&serializer::res_unknown_command, this, std::placeholders::_1));


		}

		void serializer::reset() {
			buffer_.clear();
			sgen_.reset();
			last_seq_ = 0;
			scrambler_.reset();
		}

		cyng::buffer_t serializer::merge() {
			cyng::buffer_t merged;
			for (auto& buffer : buffer_) {
				merged.insert(merged.end(), std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()));
			}
			buffer_.clear();
			return merged;
		}

		void serializer::set_sk(scramble_key const& key)
		{
			//	set new scramble key
			scrambler_ = key.key();

		}

		//void serializer::reset(cyng::context& ctx)
		//{
		//	//	[]
		//	const cyng::vector_t frame = ctx.get_frame();
		//	//CYNG_LOG_INFO(logger_, "set_sk " << cyng::io::to_str(frame));

		//	scrambler_.reset();
		//	sgen_.reset();
		//	last_seq_ = 0;
		//	//	reset default scramble key
		//	//	error: new key is 00000
		//	def_key_ = cyng::value_cast(frame.at(0), def_key_).key();

		//	//	clear buffer
		//	buffer_.consume(buffer_.size());
		//}

		sequence_t serializer::push_seq()
		{
			return last_seq_;
		}
//
//		void serializer::transfer_data(cyng::context& ctx)
//		{
//			const cyng::vector_t frame = ctx.get_frame();
//			cyng::buffer_t const data = cyng::to_buffer(frame.at(0));
//			write(data);
//
//#ifdef _DEBUG
//			ctx.queue(cyng::generate_invoke("log.msg.trace", data.size(), cyng::invoke("log.fmt.byte"), " transferred"));
//#endif
//		}
//
		cyng::buffer_t serializer::req_login_public(std::string const& name
			, std::string const& pwd)
		{
			reset();
			write_header(code::CTRL_REQ_LOGIN_PUBLIC, 0, name.size() + pwd.size() + 2);
			write(name);
			write(pwd);

			return merge();
		}

		cyng::buffer_t serializer::req_login_scrambled(std::string const& name
			, std::string const& pwd
			, scramble_key const key) {

			reset();
			
			write_header(code::CTRL_REQ_LOGIN_SCRAMBLED, 0, name.size() + pwd.size() + 2 + key.size());
			
			//	use default scrambled key
			scrambler_ = def_key_.key();
			
			write(name);
			write(pwd);
			write(key.key());
			
			//
			//	use new key
			//
			scrambler_ = key.key();

			return merge();

		}

//
		cyng::buffer_t serializer::res_login_public(response_t res, std::uint16_t watchdog, std::string redirect)
		{
			write_header(code::CTRL_RES_LOGIN_PUBLIC, 0, sizeof(res) + sizeof(watchdog) + redirect.size() + 1);

			write_numeric(res);
			write_numeric(watchdog);
			write(redirect);
			return merge();
		}
//
		cyng::buffer_t serializer::res_login_scrambled(response_t res, std::uint16_t watchdog, std::string redirect)
		{
			write_header(code::CTRL_RES_LOGIN_SCRAMBLED, 0, sizeof(res) + sizeof(watchdog) + redirect.size() + 1);

			write_numeric(res);
			write_numeric(watchdog);
			write(redirect);
			return merge();
		}
//
		cyng::buffer_t serializer::req_watchdog(sequence_t seq)
		{
			write_header(code::CTRL_REQ_WATCHDOG, seq, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_watchdog(sequence_t seq)
		{
			write_header(code::CTRL_RES_WATCHDOG, seq, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::req_open_push_channel(std::string target,		//	[0] push target
			std::string account,
			std::string number,
			std::string version,
			std::string id,	//	device id
			std::uint16_t timeout)	
		{
			//
			//	0x9000
			//
			last_seq_ = sgen_();

			std::size_t const length = target.size()
				+ account.size()
				+ number.size()
				+ version.size()
				+ id.size()
				+ sizeof(timeout)
				+ 5;	//	one \0 for each string

			write_header(code::TP_REQ_OPEN_PUSH_CHANNEL
				, last_seq_
				, length);
			write(target);
			write(account);
			write(number);
			write(version);
			write(id);
			write_numeric<std::uint16_t>(timeout);
			return merge();
		}
//
		cyng::buffer_t serializer::res_open_push_channel(sequence_t seq,
			response_t res,
			std::uint32_t channel,
			std::uint32_t source,
			std::uint16_t packet_size,
			std::uint8_t window_size,
			std::uint8_t status,
			std::uint32_t count)
		{

#ifdef _DEBUG
			static_assert(17 == sizeof(res)
					+ sizeof(channel)
					+ sizeof(source)
					+ sizeof(packet_size)
					+ sizeof(window_size)
					+ sizeof(status)
					+ sizeof(count), "res_open_push_channel(length assumption invalid)");
#endif

			write_header(code::TP_RES_OPEN_PUSH_CHANNEL, seq, 17);
			write_numeric(res);
			write_numeric(channel);
			write_numeric(source);
			write_numeric(packet_size);
			write_numeric(window_size);
			write_numeric(status);
			write_numeric(count);
			return merge();
		}
//
		cyng::buffer_t serializer::req_close_push_channel(std::uint32_t channel)
		{
			last_seq_ = sgen_();
			write_header(code::TP_REQ_CLOSE_PUSH_CHANNEL, last_seq_, sizeof(channel));
			write_numeric(channel);
			return merge();
		}
//
		cyng::buffer_t serializer::res_close_push_channel(sequence_t const seq,
			response_t const res,
			std::uint32_t const channel)
		{
			write_header(code::TP_RES_CLOSE_PUSH_CHANNEL, seq, sizeof(res) + sizeof(channel));
			write_numeric(res);
			write_numeric(channel);
			return merge();
		}
//
		cyng::buffer_t serializer::req_transfer_push_data(std::uint32_t channel,
			std::uint32_t source,	
			std::uint8_t status,
			std::uint8_t block,
			cyng::buffer_t&& data)
		{
			auto const size = static_cast<std::uint32_t>(data.size());

			last_seq_ = sgen_();
			write_header(code::TP_REQ_PUSHDATA_TRANSFER
				, last_seq_
				, sizeof(channel)
					+ sizeof(source)
					+ sizeof(status)
					+ sizeof(block)
					+ sizeof(size) 
					+ data.size());
			write_numeric(channel);
			write_numeric(source);
			write_numeric(status);
			write_numeric(block);
			write_numeric(size);
			append(std::move(data));	//	no escaping
			return merge();
		}
//
		cyng::buffer_t serializer::res_transfer_push_data(sequence_t seq,
			response_t res,
			std::uint32_t channel,
			std::uint32_t source,
			std::uint8_t status,
			std::uint8_t block)
		{
			static_assert(11 == sizeof(res)
				+ sizeof(channel)
				+ sizeof(source)
				+ sizeof(status)
				+ sizeof(block), "res_transfer_push_data(length assumption invalid)");

			write_header(code::TP_RES_PUSHDATA_TRANSFER, seq, 11);

			write_numeric(res);
			write_numeric(channel);
			write_numeric(source);
			write_numeric(status);
			write_numeric(block);
			return merge();
		}
//
		cyng::buffer_t serializer::req_open_connection(std::string number)
		{
			last_seq_ = sgen_();
			write_header(code::TP_REQ_OPEN_CONNECTION, last_seq_, number.size() + 1);
			write(number);
			return merge();
		}
//
		cyng::buffer_t serializer::res_open_connection(sequence_t seq, response_t res)
		{
			write_header(code::TP_RES_OPEN_CONNECTION, seq, sizeof(res));
			write_numeric(res);
			return merge();
		}
//
		cyng::buffer_t serializer::req_close_connection()
		{
			last_seq_ = sgen_();
			write_header(code::TP_REQ_CLOSE_CONNECTION, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_close_connection(sequence_t seq,
			response_t res)
		{
			write_header(code::TP_RES_CLOSE_CONNECTION, seq, sizeof(res));
			write_numeric(res);
			return merge();
		}
//
		cyng::buffer_t serializer::req_protocol_version()
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_PROTOCOL_VERSION, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_protocol_version(sequence_t seq,
			response_t res)
		{
			write_header(code::APP_RES_PROTOCOL_VERSION, seq, sizeof(res));
			write_numeric(res);
			return merge();
		}
//
		cyng::buffer_t serializer::req_software_version()
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_SOFTWARE_VERSION, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_software_version(sequence_t seq, std::string ver)
		{
			write_header(code::APP_RES_SOFTWARE_VERSION, seq, ver.size() + 1);
			write(ver);
			return merge();
		}
//
		cyng::buffer_t serializer::req_device_id()
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_DEVICE_IDENTIFIER, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_device_id(sequence_t seq, std::string id)
		{
			//const cyng::vector_t frame = ctx.get_frame();
			//const sequence_t seq = cyng::value_cast<sequence_t>(frame.at(0), 0);
			//const std::string id = cyng::value_cast<std::string >(frame.at(1), "");
			write_header(code::APP_RES_DEVICE_IDENTIFIER, seq, id.size() + 1);
			write(id);
			return merge();
		}
//
		cyng::buffer_t serializer::req_network_status()
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_NETWORK_STATUS, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_network_status(sequence_t seq,
			std::uint8_t dev,
			std::uint32_t stat_1,
			std::uint32_t stat_2,
			std::uint32_t stat_3,
			std::uint32_t stat_4,
			std::uint32_t stat_5,
			std::string imsi,
			std::string imei)
		{
			write_header(code::APP_RES_NETWORK_STATUS, seq, sizeof(dev) + (sizeof(std::uint32_t) * 5) + imsi.size() + imei.size() + 2);
			write_numeric(dev);
			write_numeric(stat_1);
			write_numeric(stat_2);
			write_numeric(stat_3);
			write_numeric(stat_4);
			write_numeric(stat_5);
			write(imsi);
			write(imei);
			return merge();
		}
//
		cyng::buffer_t serializer::req_ip_statistics()
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_IP_STATISTICS, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_ip_statistics(sequence_t seq,
			response_t res,
			std::uint64_t rx,
			std::uint64_t sx)
		{
			write_header(code::APP_RES_SOFTWARE_VERSION, seq, sizeof(res) + sizeof(rx) + sizeof(sx));
			write_numeric(res);
			write_numeric(rx);
			write_numeric(sx);
			return merge();
		}
//
		cyng::buffer_t serializer::req_device_auth()
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_DEVICE_AUTHENTIFICATION, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_device_auth(sequence_t seq,
			std::string account,
			std::string password,
			std::string number,
			std::string description)
		{
			write_header(code::APP_RES_DEVICE_AUTHENTIFICATION, seq, account.size() + password.size() + number.size() + description.size() + 4);
			write(account);
			write(password);
			write(number);
			write(description);
			return merge();
		}
//
		cyng::buffer_t serializer::req_device_time()
		{
			last_seq_ = sgen_();
			write_header(code::APP_REQ_DEVICE_TIME, last_seq_, 0);
			return merge();
		}
//
		cyng::buffer_t serializer::res_device_time(sequence_t seq)
		{
#if BOOST_OS_WINDOWS
			std::uint32_t now = (std::uint32_t) ::_time32(NULL);	//	C4244
#else
			std::uint32_t now = (std::uint32_t) ::time(NULL);
#endif	//	_WIN32
			write_header(code::APP_RES_DEVICE_TIME, seq, sizeof(std::uint32_t));
			write_numeric(now);
			return merge();
		}
//
//		cyng::buffer_t serializer::req_push_target_namelist()
//		{
//			BOOST_ASSERT_MSG(false, "push target namelist is not implemented yet");
//		}
//
//		cyng::buffer_t serializer::res_push_target_namelist()
//		{
//			BOOST_ASSERT_MSG(false, "push target namelist is not implemented yet");
//		}
//
//		cyng::buffer_t serializer::req_push_target_echo()
//		{
//			BOOST_ASSERT_MSG(false, "push target echo is deprecated");
//		}
//
//		cyng::buffer_t serializer::res_push_target_echo()
//		{
//			BOOST_ASSERT_MSG(false, "push target echo is deprecated");
//		}
//
//		cyng::buffer_t serializer::req_traceroute()
//		{
//			BOOST_ASSERT_MSG(false, "traceroute is deprecated");
//
//			const cyng::vector_t frame = ctx.get_frame();
//			const std::uint8_t ipt_link = cyng::value_cast<std::uint8_t >(frame.at(0), 0);
//			const std::uint16_t traceroute_idx = cyng::value_cast<std::uint16_t >(frame.at(1), 0);
//			const std::uint16_t hop_counter = cyng::value_cast<std::uint16_t >(frame.at(2), 0);
//			const std::uint32_t channel = cyng::value_cast<std::uint32_t >(frame.at(3), 0);
//
//			last_seq_ = sgen_();
//			write_header(code::APP_REQ_TRACEROUTE, last_seq_, sizeof(ipt_link) + sizeof(traceroute_idx) + sizeof(hop_counter) + sizeof(channel));
//
//			write_numeric(ipt_link);
//			write_numeric(traceroute_idx);
//			write_numeric(hop_counter);
//			write_numeric(channel);
//
//		}
//
//		cyng::buffer_t serializer::res_traceroute()
//		{
//			BOOST_ASSERT_MSG(false, "traceroute is deprecated");
//		}
//
//		cyng::buffer_t serializer::req_maintenance(cyng::context& ctx)
//		{
//			BOOST_ASSERT_MSG(false, "maintenance is deprecated");
//		}
//
//		cyng::buffer_t serializer::res_maintenance(cyng::context& ctx)
//		{
//			BOOST_ASSERT_MSG(false, "maintenance is deprecated");
//		}
//
//		cyng::buffer_t serializer::req_logout(cyng::context& ctx)
//		{
//			BOOST_ASSERT_MSG(false, "logout is deprecated");
//		}
//
//		cyng::buffer_t serializer::res_logout(cyng::context& ctx)
//		{
//			BOOST_ASSERT_MSG(false, "logout is deprecated");
//		}
//
		cyng::buffer_t serializer::req_register_push_target(std::string target,
			std::uint16_t p_size,
			std::uint8_t w_size)
		{
			last_seq_ = sgen_();
			write_header(code::CTRL_REQ_REGISTER_TARGET, last_seq_, target.size() + 1 + sizeof(p_size) + w_size);
			write(target);
			write_numeric(p_size);
			write_numeric(w_size);
			return merge();
		}
//
		cyng::buffer_t serializer::res_register_push_target(sequence_t seq,
			response_t res,
			std::uint32_t channel)
		{
			write_header(code::CTRL_RES_REGISTER_TARGET, seq, sizeof(res) + sizeof(channel));
			write_numeric(res);
			write_numeric(channel);
			return merge();
		}
//
		cyng::buffer_t serializer::req_deregister_push_target(std::string target)
		{
			last_seq_ = sgen_();
			write_header(code::CTRL_REQ_DEREGISTER_TARGET, last_seq_, target.size() + 1);
			write(target);
			return merge();
		}
//
		cyng::buffer_t serializer::res_deregister_push_target(sequence_t seq,
			response_t res,
			std::string target)
		{
			write_header(code::CTRL_RES_DEREGISTER_TARGET, seq, sizeof(res) + target.size() + 1);
			write_numeric(res);
			write(target);
			return merge();
		}
//
//		void serializer::req_multi_ctrl_public_login(cyng::context& ctx)
//		{
//			BOOST_ASSERT_MSG(false, "req_multi_ctrl_public_login is not implemented yet");
//		}
//
//		void serializer::res_multi_ctrl_public_login(cyng::context& ctx)
//		{
//			BOOST_ASSERT_MSG(false, "req_multi_ctrl_public_login is not implemented yet");
//		}
//
		cyng::buffer_t serializer::res_unknown_command(sequence_t seq,
			command_t cmd)
		{
			write_header(code::UNKNOWN, seq, sizeof(cmd));
			write_numeric(cmd);
			return merge();
		}

		void serializer::write_header(code cmd, sequence_t seq, std::size_t length)
		{
			switch (cmd)
			{
			case code::CTRL_RES_LOGIN_PUBLIC:	case code::CTRL_RES_LOGIN_SCRAMBLED:	//	login response
			case code::CTRL_REQ_LOGIN_PUBLIC:	case code::CTRL_REQ_LOGIN_SCRAMBLED:	//	login request
				break;
			default:
				//	commands are starting with an escape 
				write_numeric<std::uint8_t>(ESCAPE_SIGN);
				break;
			}

			write_numeric(static_cast<command_t>(cmd));
			write_numeric(seq);
			write_numeric<std::uint8_t>(0);
			write_numeric(static_cast<std::uint32_t>(length + HEADER_SIZE));
		}

		void serializer::write(std::string const& str)
		{
			append(cyng::to_buffer(str));
			write_numeric<std::uint8_t>(0);	//	'\0'
		}

		void serializer::write(scramble_key::key_type const& key)
		{
			cyng::buffer_t buffer;

			std::transform(std::begin(key), std::end(key), std::back_inserter(buffer), [this](char c) {
				return scrambler_[c];
				});

			buffer_.push_back(std::move(buffer));
		}

		void serializer::append(cyng::buffer_t&& data) {
			buffer_.push_back(scramble(std::move(data)));
		}

		cyng::buffer_t serializer::scramble(cyng::buffer_t&& data) {

			std::transform(std::begin(data), std::end(data), std::begin(data), [this](char c) {
				return scrambler_[c];
				});
			return data;
		}

		void serializer::write(cyng::buffer_t const& data)
		{
			cyng::buffer_t buffer;
			buffer.reserve(data.size());

			for (auto c : data)
			{
				if (c == ESCAPE_SIGN)
				{
					//	duplicate escapes
					buffer.push_back(scrambler_[c]);
				}
				buffer.push_back(scrambler_[c]);
			}

			buffer_.push_back(std::move(buffer));
		}

	}
}
