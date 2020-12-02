/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <tasks/client.h>
#include <smf/cluster/generator.h>
#include <smf/shared/protocols.h>
#include <smf/ipt/defs.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/protocol/message.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/crc16.h>

#include <crypto/hash/sha1.h>

#include <cyng/io/io_bytes.hpp>
#include <cyng/io/hex_dump.hpp>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/store/db.h>
#include <cyng/buffer_cast.h>
#include <cyng/tuple_cast.hpp>

namespace node
{
	client::client(cyng::async::base_task* btp
		, cyng::controller& cluster
		, boost::uuids::uuid tag
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, boost::asio::ip::tcp::endpoint ep
		, std::string const& meter
		, cyng::buffer_t const& srv_id
		, cyng::table::key_type const& key
		, bool client_login
		, bool verbose
		, std::string const& target)
	: base_(*btp)
		, cluster_(cluster)
		, vm_(cluster.get_io_service(), tag)
		, logger_(logger)
		, cache_(db)
		, ep_(ep)
		, meter_(meter)
		, srv_id_(srv_id)
		, key_(key)
		, client_login_(client_login)
		, target_(target)
		, socket_(base_.mux_.get_io_service())
		, parser_([this](cyng::vector_t&& prg) -> void {

			//CYNG_LOG_TRACE(logger_, "task #"
			//	<< base_.get_id()
			//	<< " <"
			//	<< base_.get_class_name()
			//	<< "> IEC data: "
			//	<< cyng::io::to_type(prg));

			vm_.async_run(std::move(prg));

		}, verbose)
		, buffer_read_()
		, buffer_write_()
		, state_(internal_state::INITIAL)
		, target_available_(false)
		, channel_(0)
		, source_(0)
		, period_list_()
		, trx_()
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< ep_
			<< " ["
			<< cluster_.tag()
			<< " => "
			<< vm_.tag()
			<< "]");

		//
		//	insert child VM
		//
		cluster_.embed(vm_);

		//
		//	prepare query
		//
		reset_write_buffer();

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		vm_.register_function("iec.data.start", 1, std::bind(&client::data_start, this, std::placeholders::_1));
		vm_.register_function("iec.data.line", 8, std::bind(&client::data_line, this, std::placeholders::_1));
		vm_.register_function("iec.data.bcc", 1, std::bind(&client::data_bcc, this, std::placeholders::_1));
		vm_.register_function("iec.data.eof", 2, std::bind(&client::data_eof, this, std::placeholders::_1));

		vm_.register_function("client.res.login", 7, std::bind(&client::client_res_login, this, std::placeholders::_1));
		vm_.register_function("client.res.open.push.channel", 8, std::bind(&client::client_res_open_push_channel, this, std::placeholders::_1));
		vm_.register_function("client.res.close.push.channel", 5, std::bind(&client::client_res_close_push_channel, this, std::placeholders::_1));
		vm_.register_function("client.res.transfer.pushdata", 6, std::bind(&client::client_res_transfer_push_data, this, std::placeholders::_1));
	}

	cyng::continuation client::run()
	{	
		switch (state_) {
			case internal_state::FAILURE:
				CYNG_LOG_ERROR(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< vm_.tag()
					<< " failure state: "
					<< meter_);
				break;

			case internal_state::SHUTDOWN:
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< vm_.tag()
					<< " is shutting down: "
					<< meter_);
				break;

			case internal_state::INITIAL:
				authorize_client();
				break;

			case internal_state::WAIT:
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< vm_.tag()
					<< " no login response for account: "
					<< meter_);
				break;

			case internal_state::AUTHORIZED:
				query_metering_data();
				break;

			default:
				CYNG_LOG_FATAL(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< vm_.tag()
					<< " invalid state");
				break;
		}
		//
		//	start/restart timer
		//
		auto const monitor = get_monitor();
		base_.suspend(monitor);

		return cyng::continuation::TASK_CONTINUE;
	}

	std::chrono::seconds client::get_monitor() const
	{
		return cyng::value_cast(cache_.get_value("TBridge", "interval", key_), std::chrono::seconds(12 * 60));
	}

	void client::query_metering_data()
	{
		if (!socket_.is_open()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> open connection to "
				<< ep_);

			try {

				boost::asio::ip::tcp::resolver resolver(base_.mux_.get_io_service());
				auto const endpoints = resolver.resolve(ep_);
				do_connect(endpoints);
			}
			catch (std::exception const& ex) {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< ex.what());
			}
		}
		else {
			reset_write_buffer();
			do_write();
		}
	}

	void client::authorize_client()
	{
		auto const pwd = cyng::io::to_str(cyng::sha1_hash(meter_ + NODE::SALT));

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> authorize "
			<< meter_
			<< ':'
			<< pwd);

		cluster_.async_run(client_req_login(vm_.tag()
			, meter_	//	name
			, pwd	//	pwd
			, "plain" //	login scheme
			, cyng::param_map_factory
			("tp-layer", protocol_TCP)
			("data-layer", protocol_IEC)
			("time", std::chrono::system_clock::now())
			("ep", ep_)));

	}

	cyng::continuation client::process()
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void client::stop(bool shutdown)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> remove embedded vm "
			<< vm_.tag());

		//
		//	remove from parent VM
		//
		cluster_.remove(vm_.tag());

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	void client::do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints)
	{
		boost::asio::async_connect(socket_, endpoints,
			[this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint ep)
			{
				if (!ec) {
					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> connected to "
						<< ep);

					cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
						, "connected"
						, ep
						, cluster_.tag()));

					//
					//	send query
					//
					do_write();

					//
					//	start reading
					//
					do_read();
				}
				else {
					auto const next_ts = std::chrono::system_clock::now() + get_monitor();

					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< ep_
						<< ' '
						<< ec.message());

					std::stringstream ss;
					ss
						<< "connect failed: "
						<< ec.message()
						<< " -  next try at "
						<< cyng::to_str(next_ts)
						<< " UTC"
						;
					cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
						, ss.str()
						, ep_
						, cluster_.tag()));

					socket_.close(ec);
				}
			});
	}

	void client::do_read()
	{
		socket_.async_read_some(boost::asio::buffer(buffer_read_.data(), buffer_read_.size()),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec) {

#ifdef __DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer_read_.begin(), buffer_read_.begin() + bytes_transferred);
					CYNG_LOG_TRACE(logger_, "\n" << ss.str());
#endif

					//cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
					//	, std::string(buffer_read_.begin(), buffer_read_.begin() + bytes_transferred)
					//	, socket_.remote_endpoint()
					//	, cluster_.tag()));

					//
					//	parse IEC data
					//
					parser_.read(buffer_read_.begin(), buffer_read_.begin() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> received "
						<< cyng::bytes_to_str(bytes_transferred));
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< ep_
						<< ' '
						<< ec.message());

					//
					//	socket should already be closed
					//
					socket_.close(ec);

					reset_write_buffer();

				}

			});

	}

	void client::do_write()
	{
		if (!buffer_write_.empty()) {
			boost::asio::async_write(socket_,
				boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
				[this](boost::system::error_code ec, std::size_t bytes_transferred)
				{
					if (!ec)
					{
						CYNG_LOG_TRACE(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> send "
							<< cyng::bytes_to_str(bytes_transferred));


						CYNG_LOG_TRACE(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> send "
							<< cyng::io::to_hex(buffer_write_.front()));

						

						buffer_write_.pop_front();
						if (!buffer_write_.empty())
						{
							do_write();
						}
					}
					else
					{
						//
						//	no more data will be send
						//
						socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
						socket_.close(ec);
					}
				});
		}
	}

	void client::reset_write_buffer()
	{
		buffer_write_.clear();
		std::string const hello("/?" + meter_ + "!\r\n");
		buffer_write_.emplace_back(cyng::buffer_t(hello.begin(), hello.end()));

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> query: "
			<< hello);

	}

	void client::data_start(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const [tag] = cyng::tuple_cast<
			boost::uuids::uuid		//	[0] record tag
		>(frame);

		std::stringstream ss;
		ss
			<< "start data set "
			<< tag
			<< " ("
			<< target_
			<< ")"
			;

		cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
			, ss.str()
			, ep_
			, cluster_.tag()));

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< vm_.tag()
			<< " open push channel "
			<< target_);

		//
		//	open push channel
		//
		cluster_.async_run(client_req_open_push_channel(vm_.tag()
			, target_
			, ""	//	device
			, ""	//	number
			, ""	//	version
			, ""	//	id
			, 30	//	timeout in seconds
			, cyng::param_map_factory("meter", meter_)));

	}

	void client::data_line(cyng::context& ctx)
	{
		//	[c8b5e3fb-ad83-47c8-ab21-357aec360af6,0000470700FF,0.000,A,,28]
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));

		boost::uuids::uuid tag;
		sml::obis code;			//	[1] OBIS code
		std::string value;		//	[2] value
		std::int8_t scaler;		//	[3] scaler
		std::string unit;		//	[3] unit
		std::uint8_t unit_code;	//	[4] unit
		std::string status;		//	[5] status
		std::uint64_t idx;		//	[6] index

		std::tie(tag, code, value, scaler, unit, unit_code, status, idx) = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] record tag
			cyng::buffer_t,			//	[1] OBIS code
			std::string,			//	[2] value
			std::int8_t,			//	[3] scaler
			std::string,			//	[3] unit
			std::uint8_t,			//	[4] unit-code
			std::string,			//	[5] status
			std::uint64_t			//	[6] index
		>(frame);
		
		//
		//	display only electricity values
		//
		if (code.get_medium() == sml::obis::MEDIA_ELECTRICITY) {

			auto const m = std::pow(10, scaler);
			auto dv = std::stod(value);

			std::stringstream ss;
			ss
				<< sml::get_name(code)
				<< ": "
				<< (dv*m)
				//<< cyng::io::to_str(frame.at(2))
				<< ' '
				<< unit
				;

			cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
				, ss.str()
				, ep_
				, cluster_.tag()));

			auto iv = static_cast<std::int64_t>(std::stoll(value));
			period_list_.push_back(sml::period_entry(code
				, unit_code
				, scaler
				, cyng::make_object(iv)));

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< vm_.tag()
				<< " push data ["
				<< meter_
				<< "]: "
				<< iv
				<< unit
				<< " * 10^"
				<< +scaler);

		}
		else if (code == sml::OBIS_MBUS_STATE) {
			//	"00000000"
			if (!boost::algorithm::equals(value, "00000000")) {
				//	fatal error
				cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
					, "fatal error code from metering device: " + value
					, ep_
					, cluster_.tag()));
			}
			auto ec = static_cast<std::int32_t>(std::stoul(value));
			period_list_.push_back(sml::period_entry(code
				, unit_code
				, scaler
				, cyng::make_object(ec)));

		}
		else if (code == sml::OBIS_MBUS_STATE) {
			//	"00000000"
			if (!boost::algorithm::equals(value, "00000000")) {
				//	fatal error
				cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
					, "fatal error code from metering device: " + value
					, ep_
					, cluster_.tag()));
			}
			auto ec = static_cast<std::int32_t>(std::stoul(value));
			period_list_.push_back(sml::period_entry(code
				, unit_code
				, scaler
				, cyng::make_object(ec)));

		}
		else if (code == sml::OBIS_MBUS_STATE_1
			|| code == sml::OBIS_MBUS_STATE_2
			|| code == sml::OBIS_MBUS_STATE_3) {
			//	"00000000"
			auto ec = static_cast<std::int32_t>(std::stoul(value));
			period_list_.push_back(sml::period_entry(code
				, unit_code
				, scaler
				, cyng::make_object(ec)));

		}
		else if (code == sml::OBIS_METER_ADDRESS) {
			//	03218421
			period_list_.push_back(sml::period_entry(code
				, unit_code
				, scaler
				, cyng::make_object(cyng::make_buffer(value))));
		}
		else {
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< vm_.tag()
				<< " push data ["
				<< meter_
				<< "]: "
				<< value);

		}

	}

	void client::data_bcc(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

	}

	void client::data_eof(cyng::context& ctx)
	{
		//	[57da25c7-85a8-4f05-966f-904f52d3204b,35]
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const [tag, count] = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] record tag
			std::uint64_t			//	[1] counter
		>(frame);

		std::stringstream ss;
		ss
			<< "data set "
			<< tag
			<< " with "
			<< count
			<< " entries is complete"
			;

		cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
			, ss.str()
			, ep_
			, cluster_.tag()));

		sml::res_generator gen;
		auto const tpl = gen.get_profile_list(*trx_++
			, srv_id_
			, sml::OBIS_PROFILE_15_MINUTE
			, std::chrono::system_clock::now() //	act_time
			, 0 //	reg_period
			, std::chrono::system_clock::now() //	val_time
			, 0		//	status
			, std::move(period_list_));

		//CYNG_LOG_TRACE(logger_, "DATA " << cyng::io::to_type(tpl));
		auto buffer = sml::linearize(tpl);
		sml::sml_set_crc16(buffer);
		//CYNG_LOG_TRACE(logger_, "BUFFER " << cyng::io::to_hex(buffer));

		cluster_.async_run(client_req_transfer_pushdata(vm_.tag()
			, channel_
			, source_
			, cyng::make_object(buffer)	//	data
			, cyng::param_map_factory("meter", meter_)));

		//
		//	close push channel
		//
		cluster_.async_run(client_req_close_push_channel(vm_.tag()
			, channel_	//	channel
			, cyng::param_map_factory("meter", meter_)));

		//
		//	close socket
		//
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);

	}

	void client::client_res_login(cyng::context& ctx)
	{
		//	[ccf28039-33a5-4737-8da8-7dfe3fc3a948,1,false,03218421,unknown device,00000000,%(("data-layer":IEC:62056),("ep":192.168.0.200:6006),("time":2020-11-24 17:47:31.37567800),("tp-layer":tcp))]
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] peer tag
			std::uint64_t,			//	[1] sequence number
			bool,					//	[2] success
			std::string,			//	[3] name
			std::string,			//	[4] msg
			std::uint32_t,			//	[5] query
			cyng::param_map_t		//	[6] bag
		>(frame);

		//
		//	update client state
		//
		if (std::get<2>(tpl)) {

			state_ = internal_state::AUTHORIZED;

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< vm_.tag()
				<< " login of ["
				<< meter_
				<< "]: "
				<< std::get<4>(tpl));

			//
			//	send query
			//
			query_metering_data();

		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< vm_.tag()
				<< " login of ["
				<< meter_
				<< "] failed: "
				<< std::get<4>(tpl));

			state_ = internal_state::INITIAL;	//	retry
		}
	}

	void client::client_res_open_push_channel(cyng::context& ctx)
	{
		// [ee76091f-3a55-4476-ae11-7ef2df7117fd:uuid,5u64,true,48f4efb8u32,eda854cdu32,00000001u32,%(("channel-status":0u8),("packet-size":ffffu16),("response-code":1u8),("window-size":1u8)),%(("meter":"03218421"))]

		BOOST_ASSERT(ctx.tag() == vm_.tag());
		auto const frame = ctx.get_frame();
		BOOST_ASSERT(frame.size() == ctx.frame_size());
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));

		auto [peer, seq, target_available, channel, source, count, opt, bag] = cyng::tuple_cast<
			boost::uuids::uuid,		//	[1] peer
			std::uint64_t,			//	[2] cluster sequence
			bool,					//	[3] success flag
			std::uint32_t,			//	[4] channel
			std::uint32_t,			//	[5] source
			std::uint32_t,			//	[6] count
			cyng::param_map_t,		//	[7] options
			cyng::param_map_t		//	[8] bag
		>(frame);

		if (!target_available) {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< vm_.tag()
				<< " open target ["
				<< target_
				<< "] failed");
		}
		else {
			target_available_ = target_available;		//	push target
			channel_ = channel;		//	push target channel
			source_ = source;		//	push target source
		}
	}

	void client::client_res_close_push_channel(cyng::context& ctx)
	{
		BOOST_ASSERT(ctx.tag() == vm_.tag());
		auto const frame = ctx.get_frame();
		BOOST_ASSERT(frame.size() == ctx.frame_size());
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] peer
			std::uint64_t,			//	[1] cluster sequence
			bool,					//	[2] success flag
			std::uint32_t,			//	[3] channel
			cyng::param_map_t		//	[4] bag
		>(frame);

	}

	void client::client_res_transfer_push_data(cyng::context& ctx)
	{
		BOOST_ASSERT(vm_.tag() == ctx.tag());
		auto const frame = ctx.get_frame();
		BOOST_ASSERT(frame.size() == ctx.frame_size());
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_type(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[1] peer
			std::uint64_t,			//	[2] cluster sequence
			std::uint32_t,			//	[3] source
			std::uint32_t,			//	[4] channel
			std::size_t,			//	[5] count
			cyng::param_map_t		//	[6] bag
		>(frame);
	}

}
