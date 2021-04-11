/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/serializer.h>


#include <iterator>
#include <algorithm>

#include <boost/predef.h>

namespace smf {

	namespace ipt {

		serializer::serializer(scramble_key const& def)
		: sgen_()
			, last_seq_(0)
			, scrambler_()
			, def_key_(def)
		{}

		void serializer::reset() {
			sgen_.reset();
			last_seq_ = 0;
			scrambler_.reset();
		}

		void serializer::set_sk(scramble_key const& key)
		{
			//	set new scramble key
			scrambler_ = key.key();

		}

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
			auto deq = write_header(code::CTRL_REQ_LOGIN_PUBLIC, 0, name.size() + pwd.size() + 2);
			deq.push_back(write(name));
			deq.push_back(write(pwd));

			return merge(deq);
		}

		cyng::buffer_t serializer::req_login_scrambled(std::string const& name
			, std::string const& pwd
			, scramble_key const key) {

			reset();
			
			auto deq = write_header(code::CTRL_REQ_LOGIN_SCRAMBLED, 0, name.size() + pwd.size() + 2 + key.size());
			
			//	use default scrambled key
			scrambler_ = def_key_.key();
			
			deq.push_back(write(name));
			deq.push_back(write(pwd));
			deq.push_back(write(key.key()));
			
			//
			//	use new key
			//
			scrambler_ = key.key();

			return merge(deq);

		}

//
		cyng::buffer_t serializer::res_login_public(response_t res, std::uint16_t watchdog, std::string redirect)
		{
			auto deq = write_header(code::CTRL_RES_LOGIN_PUBLIC, 0, sizeof(res) + sizeof(watchdog) + redirect.size() + 1);

			deq.push_back(write_numeric(res));
			deq.push_back(write_numeric(watchdog));
			deq.push_back(write(redirect));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_login_scrambled(response_t res, std::uint16_t watchdog, std::string redirect)
		{
			auto deq = write_header(code::CTRL_RES_LOGIN_SCRAMBLED, 0, sizeof(res) + sizeof(watchdog) + redirect.size() + 1);

			deq.push_back(write_numeric(res));
			deq.push_back(write_numeric(watchdog));
			deq.push_back(write(redirect));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_watchdog(sequence_t seq)
		{
			auto deq = write_header(code::CTRL_REQ_WATCHDOG, seq, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_watchdog(sequence_t seq)
		{
			auto deq = write_header(code::CTRL_RES_WATCHDOG, seq, 0);
			return merge(deq);
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

			auto deq = write_header(code::TP_REQ_OPEN_PUSH_CHANNEL
				, last_seq_
				, length);
			deq.push_back(write(target));
			deq.push_back(write(account));
			deq.push_back(write(number));
			deq.push_back(write(version));
			deq.push_back(write(id));
			deq.push_back(write_numeric<std::uint16_t>(timeout));
			return merge(deq);
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

			auto deq = write_header(code::TP_RES_OPEN_PUSH_CHANNEL, seq, 17);
			deq.push_back(write_numeric(res));
			deq.push_back(write_numeric(channel));
			deq.push_back(write_numeric(source));
			deq.push_back(write_numeric(packet_size));
			deq.push_back(write_numeric(window_size));
			deq.push_back(write_numeric(status));
			deq.push_back(write_numeric(count));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_close_push_channel(std::uint32_t channel)
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::TP_REQ_CLOSE_PUSH_CHANNEL, last_seq_, sizeof(channel));
			deq.push_back(write_numeric(channel));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_close_push_channel(sequence_t const seq,
			response_t const res,
			std::uint32_t const channel)
		{
			auto deq = write_header(code::TP_RES_CLOSE_PUSH_CHANNEL, seq, sizeof(res) + sizeof(channel));
			deq.push_back(write_numeric(res));
			deq.push_back(write_numeric(channel));
			return merge(deq);
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
			auto deq = write_header(code::TP_REQ_PUSHDATA_TRANSFER
				, last_seq_
				, sizeof(channel)
					+ sizeof(source)
					+ sizeof(status)
					+ sizeof(block)
					+ sizeof(size) 
					+ data.size());
			deq.push_back(write_numeric(channel));
			deq.push_back(write_numeric(source));
			deq.push_back(write_numeric(status));
			deq.push_back(write_numeric(block));
			deq.push_back(write_numeric(size));
			deq.push_back(scramble(std::move(data)));	//	no escaping
			return merge(deq);
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

			auto deq = write_header(code::TP_RES_PUSHDATA_TRANSFER, seq, 11);

			deq.push_back(write_numeric(res));
			deq.push_back(write_numeric(channel));
			deq.push_back(write_numeric(source));
			deq.push_back(write_numeric(status));
			deq.push_back(write_numeric(block));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_open_connection(std::string number)
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::TP_REQ_OPEN_CONNECTION, last_seq_, number.size() + 1);
			deq.push_back(write(number));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_open_connection(sequence_t seq, response_t res)
		{
			auto deq = write_header(code::TP_RES_OPEN_CONNECTION, seq, sizeof(res));
			deq.push_back(write_numeric(res));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_close_connection()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::TP_REQ_CLOSE_CONNECTION, last_seq_, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_close_connection(sequence_t seq,
			response_t res)
		{
			auto deq = write_header(code::TP_RES_CLOSE_CONNECTION, seq, sizeof(res));
			deq.push_back(write_numeric(res));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_protocol_version()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::APP_REQ_PROTOCOL_VERSION, last_seq_, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_protocol_version(sequence_t seq,
			response_t res)
		{
			auto deq = write_header(code::APP_RES_PROTOCOL_VERSION, seq, sizeof(res));
			deq.push_back(write_numeric(res));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_software_version()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::APP_REQ_SOFTWARE_VERSION, last_seq_, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_software_version(sequence_t seq, std::string ver)
		{
			auto deq = write_header(code::APP_RES_SOFTWARE_VERSION, seq, ver.size() + 1);
			deq.push_back(write(ver));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_device_id()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::APP_REQ_DEVICE_IDENTIFIER, last_seq_, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_device_id(sequence_t seq, std::string id)
		{
			//const cyng::vector_t frame = ctx.get_frame();
			//const sequence_t seq = cyng::value_cast<sequence_t>(frame.at(0), 0);
			//const std::string id = cyng::value_cast<std::string >(frame.at(1), "");
			auto deq = write_header(code::APP_RES_DEVICE_IDENTIFIER, seq, id.size() + 1);
			deq.push_back(write(id));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_network_status()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::APP_REQ_NETWORK_STATUS, last_seq_, 0);
			return merge(deq);
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
			auto deq = write_header(code::APP_RES_NETWORK_STATUS, seq, sizeof(dev) + (sizeof(std::uint32_t) * 5) + imsi.size() + imei.size() + 2);
			deq.push_back(write_numeric(dev));
			deq.push_back(write_numeric(stat_1));
			deq.push_back(write_numeric(stat_2));
			deq.push_back(write_numeric(stat_3));
			deq.push_back(write_numeric(stat_4));
			deq.push_back(write_numeric(stat_5));
			deq.push_back(write(imsi));
			deq.push_back(write(imei));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_ip_statistics()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::APP_REQ_IP_STATISTICS, last_seq_, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_ip_statistics(sequence_t seq,
			response_t res,
			std::uint64_t rx,
			std::uint64_t sx)
		{
			auto deq = write_header(code::APP_RES_SOFTWARE_VERSION, seq, sizeof(res) + sizeof(rx) + sizeof(sx));
			deq.push_back(write_numeric(res));
			deq.push_back(write_numeric(rx));
			deq.push_back(write_numeric(sx));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_device_auth()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::APP_REQ_DEVICE_AUTHENTIFICATION, last_seq_, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_device_auth(sequence_t seq,
			std::string account,
			std::string password,
			std::string number,
			std::string description)
		{
			auto deq = write_header(code::APP_RES_DEVICE_AUTHENTIFICATION, seq, account.size() + password.size() + number.size() + description.size() + 4);
			deq.push_back(write(account));
			deq.push_back(write(password));
			deq.push_back(write(number));
			deq.push_back(write(description));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_device_time()
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::APP_REQ_DEVICE_TIME, last_seq_, 0);
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_device_time(sequence_t seq)
		{
#if BOOST_OS_WINDOWS
			std::uint32_t now = (std::uint32_t) ::_time32(NULL);	//	C4244
#else
			std::uint32_t now = (std::uint32_t) ::time(NULL);
#endif	//	_WIN32
			auto deq = write_header(code::APP_RES_DEVICE_TIME, seq, sizeof(std::uint32_t));
			deq.push_back(write_numeric(now));
			return merge(deq);
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
		std::pair<cyng::buffer_t, sequence_t> serializer::req_register_push_target(std::string target,
			std::uint16_t p_size,
			std::uint8_t w_size)
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::CTRL_REQ_REGISTER_TARGET, last_seq_, target.size() + 1 + sizeof(p_size) + w_size);
			deq.push_back(write(target));
			deq.push_back(write_numeric(p_size));
			deq.push_back(write_numeric(w_size));
			return std::make_pair(merge(deq), last_seq_);
		}
//
		cyng::buffer_t serializer::res_register_push_target(sequence_t seq,
			response_t res,
			std::uint32_t channel)
		{
			auto deq = write_header(code::CTRL_RES_REGISTER_TARGET, seq, sizeof(res) + sizeof(channel));
			deq.push_back(write_numeric(res));
			deq.push_back(write_numeric(channel));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::req_deregister_push_target(std::string target)
		{
			last_seq_ = sgen_();
			auto deq = write_header(code::CTRL_REQ_DEREGISTER_TARGET, last_seq_, target.size() + 1);
			deq.push_back(write(target));
			return merge(deq);
		}
//
		cyng::buffer_t serializer::res_deregister_push_target(sequence_t seq,
			response_t res,
			std::string target)
		{
			auto deq = write_header(code::CTRL_RES_DEREGISTER_TARGET, seq, sizeof(res) + target.size() + 1);
			deq.push_back(write_numeric(res));
			deq.push_back(write(target));
			return merge(deq);
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
			
			auto deq = write_header(code::UNKNOWN, seq, sizeof(cmd));
			deq.push_back(write_numeric(cmd));
			return merge(deq);
		}

		std::deque<cyng::buffer_t> serializer::write_header(code cmd, sequence_t seq, std::size_t length)
		{
			std::deque<cyng::buffer_t> deq;
			switch (cmd)
			{
			case code::CTRL_RES_LOGIN_PUBLIC:	case code::CTRL_RES_LOGIN_SCRAMBLED:	//	login response
			case code::CTRL_REQ_LOGIN_PUBLIC:	case code::CTRL_REQ_LOGIN_SCRAMBLED:	//	login request
				break;
			default:
				//	commands are starting with an escape 
				deq.push_back(write_numeric<std::uint8_t>(ESCAPE_SIGN));
				break;
			}

			deq.push_back(write_numeric(static_cast<command_t>(cmd)));
			deq.push_back(write_numeric(seq));
			deq.push_back(write_numeric<std::uint8_t>(0));
			deq.push_back(write_numeric(static_cast<std::uint32_t>(length + HEADER_SIZE)));
			return deq;
		}

		cyng::buffer_t serializer::write(std::string const& str)
		{
			std::deque<cyng::buffer_t> deq;
			deq.push_back(scramble(cyng::to_buffer(str)));
			deq.push_back(write_numeric<std::uint8_t>(0));	//	'\0'
			return merge(deq);
		}

		cyng::buffer_t serializer::write(scramble_key::key_type const& key)
		{
			cyng::buffer_t buffer;

			std::transform(std::begin(key), std::end(key), std::back_inserter(buffer), [this](char c) {
				return scrambler_[c];
				});

			return buffer;
		}

		cyng::buffer_t serializer::scramble(cyng::buffer_t&& data) {

			std::transform(std::begin(data), std::end(data), std::begin(data), [this](char c) {
				return scrambler_[c];
				});
			return data;
		}

		cyng::buffer_t serializer::write(cyng::buffer_t const& data)
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

			return buffer;
		}


		cyng::buffer_t merge(std::deque<cyng::buffer_t> deq) {
			cyng::buffer_t merged;
			for (auto& buffer : deq) {
				merged.insert(merged.end(), std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()));
			}
			return merged;
		}

	}
}
