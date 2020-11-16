/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <session.h>
#include <smf/shared/protocols.h>
#include <smf/cluster/generator.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/reader.h>
#include <smf/sml/parser/obis_parser.h>

#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/controller.h>
#include <cyng/util/split.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>
#include <cyng/store/db.h>
#include <cyng/set_cast.h>
#include <cyng/buffer_cast.h>

namespace node
{
	session::session(boost::asio::ip::tcp::socket socket
		, cyng::logging::log_ptr logger
		, cyng::controller& cluster
		, cyng::controller& vm
		, cyng::store::db& db
		, bool session_login	//	session login required
		, bool session_auto_insert)
	: socket_(std::move(socket))
		, logger_(logger)
		, cluster_(cluster)
		, vm_(vm)
		, cache_(logger, db)
		, session_login_(session_login)
		, session_auto_insert_(session_auto_insert)
		, buffer_()
		, authorized_(!session_login)	//	if session login required authorization also required
		, data_()
		, rx_(0)
		, sx_(0)
		, parser_([&](wmbus::header const& h, cyng::buffer_t const& data) {


			this->decode(h, data);

		})
		, uuidgen_()
		, decoder_(logger
			, vm_
			, std::bind(&session::cb_meter_decoded, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
			, std::bind(&session::cb_data_decoded, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
			, std::bind(&session::cb_value_decoded, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6)
		)
	{
		CYNG_LOG_INFO(logger_, "new session [" 
			<< vm_.tag()
			<< "] at " 
			<< socket_.remote_endpoint() 
			<< (session_login_ ? " - authorization required" : ""));

		vm_.register_function("client.res.login", 1, [&](cyng::context& ctx) {

			auto frame = ctx.get_frame();

			//	[67dd63f3-a055-41de-bdee-475f71621a21,[26c841a3-a1c4-4ac2-92f6-8bdd2cbe80f4,1,false,name,unknown device,00000000,
			//	%(("data-layer":wM-Bus),("local-ep":127.0.0.1:63953),("remote-ep":127.0.0.1:12001),("security":public),("time":2020-08-23 18:02:59.18770200),("tp-layer":raw))]]
			CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		});

		//
		//	SML message
		//
		vm_.register_function("sml.msg", 3, std::bind(&session::sml_msg, this, std::placeholders::_1));

		//
		//	Comes from SML reader
		//	This is the only message that wmBus can generate
		//
		vm_.register_function("sml.get.list.response", 5, std::bind(&session::sml_get_list_response, this, std::placeholders::_1));

	}

	session::~session()
	{
		//
		//	remove from session list
		//
	}

	void session::start()
	{
		do_read();
	}

	void session::do_read()
	{
		auto self(shared_from_this());

		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					CYNG_LOG_TRACE(logger_, "session received "
						<< cyng::bytes_to_str(bytes_transferred));

#ifdef _DEBUG
					//cyng::io::hex_dump hd;
					//std::stringstream ss;
					//hd(ss, buffer_.begin(), buffer_.begin() + bytes_transferred);
					//CYNG_LOG_TRACE(logger_, "\n" << ss.str());
#endif
					if (authorized_) {
						//
						//	raw data
						//
						process_data(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });
					}
					else {

						//
						//	login data
						//
						process_login(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });
					}

					//
					//	continue reading
					//
					do_read();
				}
				else
				{
					//	leave
					CYNG_LOG_WARNING(logger_, "session closed: "
						<< ec.message());

					if (session_login_ && authorized_) {

						//
						//	remove client
						//
						cluster_.async_run(client_req_close(vm_.tag(), ec.value()));
						authorized_ = false;
					}
				}
			});
	}

	void session::process_data(cyng::buffer_t&& data)
	{
		//
		//	insert new record into "_wMBusUplink" table
		//	
		parser_.read(data.begin(), data.end());

		//
		//	update "sx" value of this session/device
		//
		sx_ += data.size();
		if (session_login_ && authorized_) {

			//
			//	update session throughput if session is authorized
			//
			cluster_.async_run(bus_req_db_modify("_Session"
				, cyng::table::key_generator(vm_.tag())
				, cyng::param_factory("sx", sx_)
				, 0u
				, vm_.tag()));
		}

	}

	void session::process_login(cyng::buffer_t&& data)
	{
		data_.insert(data_.end(), data.begin(), data.end());

		auto pos = std::find(data_.begin(), data_.end(), '\n');
		if (pos != data_.end()) {

			auto const vec = cyng::split(std::string(data_.begin(), pos), ":");
			if (vec.size() == 2) {

				CYNG_LOG_INFO(logger_, "send authorization request to cluster: "
					<< vec.at(0)
					<< ':'
					<< vec.at(1)
					<< '@'
					<< socket_.remote_endpoint());

				cluster_.async_run(client_req_login(vm_.tag()
					, vec.at(0)	//	name
					, vec.at(1)	//	password
					, "plain"	//	login scheme
					, cyng::param_map_factory("tp-layer", "TCP/IP")
					("data-layer", "wM-Bus:EN13757-4")
					("time", std::chrono::system_clock::now())
					("local-ep", socket_.remote_endpoint())
					("remote-ep", socket_.local_endpoint())));

			}

			//
			//	move iterator behind '\n' and
			//	remove login data from data stream
			//
			if (pos != data_.end())	++pos;
			data_.erase(data_.begin(), pos);

			//
			//	login complete
			//
			authorized_ = true;

			if (!data_.empty()) {
				process_data(std::move(data_));
			}
		}
		else {
			if (data_.size() > 256) {
				CYNG_LOG_WARNING(logger_, "give up on waiting for login after "
					<< cyng::bytes_to_str(data_.size()));
				authorized_ = true;
			}
		}
	}

	void session::decode(wmbus::header const& h, cyng::buffer_t const& data)
	{
		auto const srv_id = h.get_server_id();
		auto const ident = sml::from_server_id(srv_id);

		CYNG_LOG_INFO(logger_, "received "
			<< cyng::bytes_to_str(data.size())
			<< " from "
			<< ident);


		//	lookup meter pk
		auto const rec = cache_.lookup_meter_by_ident(ident);
		if (!rec.empty()) {

			auto const aes = cache_.lookup_aes(rec.key());
			CYNG_LOG_DEBUG(logger_, "aes key     : " << cyng::io::to_str(aes));

			cluster_.async_run(bus_insert_wMBus_uplink(std::chrono::system_clock::now()
				, ident
				, h.get_medium()
				, h.get_manufacturer()
				, h.get_frame_type()
				, cyng::io::to_str(rec.convert())
				, cluster_.tag()));

			decoder_.run(h.get_server_id()
				, h.get_frame_type()
				, data
				, aes);

		}
		else {
			CYNG_LOG_WARNING(logger_, "meter "
				<< ident
				<< " not configured");

			auto const str = cyng::io::to_hex(data);

			cluster_.async_run(bus_insert_wMBus_uplink(std::chrono::system_clock::now()
				, ident
				, h.get_medium()
				, h.get_manufacturer()
				, h.get_frame_type()
				, str
				, cluster_.tag()));

			if (session_auto_insert_) {

				//
				//	insert into TMeter and TMeterAccess 
				//
				insert_meter(h);
			}

		}
		
	}

	void session::insert_meter(wmbus::header const& h)
	{
		auto const srv_id = h.get_server_id();
		auto const ident = sml::from_server_id(srv_id);
		auto const meter = sml::get_serial(srv_id);	//	string
		auto const tag = uuidgen_();

		auto const mc = sml::gen_metering_code("YY"
			, meter
			, tag);

		//
		//	TMeter
		//
		CYNG_LOG_INFO(logger_, "insert meter "
			<< ident);

		auto const row_meter = cyng::table::data_generator(
			ident,	//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
			meter,	//	meter number (i.e. 16000913) 4 bytes 
			mc,		//	metering code 
			h.get_manufacturer(),	//	manufacturer (3 char-code)
			std::chrono::system_clock::now(),			//	time of manufacture
			h.get_manufacturer(),	//	meter type
			"",		//	parametrierversion (i.e. 16A098828.pse)
			"",		//	fabrik nummer (i.e. 06441734)
			"",		//	ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
			"C",		//	Metrological Class: A, B, C, Q3/Q1, ...
			boost::uuids::nil_uuid(),			//	optional gateway pk
			to_str(protocol_e::wMBUS));	//	[string] data protocol (IEC, M-Bus, COSEM, ...)

		auto const pk = cyng::table::key_generator(tag);
		
		cluster_.async_run(bus_req_db_insert("TMeter", pk, row_meter, 1u, cluster_.tag()));

		//
		//	TMeterAccess
		//

		cyng::crypto::aes_128_key aes;
		if (boost::algorithm::equals(ident, "01-e61e-79426800-02-0e")) {

			//
			//	gas meter
			//
			CYNG_LOG_DEBUG(logger_, "\n\n GAS METER 01-e61e-79426800-02-0e\n\n");

			aes.key_ = { 0x61, 0x40, 0xB8, 0xC0, 0x66, 0xED, 0xDE, 0x37, 0x73, 0xED, 0xF7, 0xF8, 0x00, 0x7A, 0x45, 0xAB };
		}
		else if (boost::algorithm::equals(ident, "01-e61e-79426800-02-0e") 
			|| boost::algorithm::equals(ident, "01-e61e-57140621-36-03")) {
			//	6140B8C066EDDE3773EDF7F8007A45AB
			aes.key_ = { 0x61, 0x40, 0xB8, 0xC0, 0x66, 0xED, 0xDE, 0x37, 0x73, 0xED, 0xF7, 0xF8, 0x00, 0x7A, 0x45, 0xAB };
		}
		else if (boost::algorithm::equals(ident, "01-e61e-29436587-bf-03") ||
			boost::algorithm::equals(ident, "01-e61e-13090016-3c-07")) {
			aes.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };
		}
		else if (boost::algorithm::equals(ident, "01-a815-74314504-01-02")) {
			aes.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };
		}
		auto const row_access = cyng::table::data_generator(
			0u,						//	status
			cyng::make_buffer({}),	//	pubKey
			aes, //	aes
			"", //	user
			"" //	pwd
		);

		cluster_.async_run(bus_req_db_insert("TMeterAccess", pk, row_access, 1u, cluster_.tag()));

		//
		//	TBridge
		//

		auto const ep = socket_.local_endpoint();
		auto const row_bridge = cyng::table::data_generator(
			ep.address(),	//	[ip] incoming/outgoing IP connection
			ep.port(),		//	[ip] incoming/outgoing IP connection
			false,	//	outgoing
			//	[seconds] pull cycle
			cyng::make_seconds(15 * 60));

		cluster_.async_run(bus_req_db_insert("TBridge", pk, row_bridge, 1u, cluster_.tag()));

	}

	void session::cb_meter_decoded(cyng::buffer_t const& srv_id
		, std::uint8_t status
		, std::uint8_t aes_mode
		, cyng::crypto::aes_128_key const& aes)
	{
		auto const manufacturer = sml::get_manufacturer_code(srv_id);

		CYNG_LOG_DEBUG(logger_, "server ID   : " << sml::from_server_id(srv_id));	//	OBIS_SERIAL_NR - 00 00 60 01 00 FF
		CYNG_LOG_DEBUG(logger_, "medium      : " << mbus::get_medium_name(sml::get_medium_code(srv_id)));
		CYNG_LOG_DEBUG(logger_, "manufacturer: " << sml::decode(manufacturer));	//	OBIS_DATA_MANUFACTURER - 81 81 C7 82 03 FF
		CYNG_LOG_DEBUG(logger_, "device id   : " << sml::get_serial(srv_id));
		CYNG_LOG_DEBUG(logger_, "status      : " << +status);
		CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);

	}

	void session::cb_data_decoded(cyng::buffer_t const& srv_id
		, cyng::buffer_t const& data
		, std::uint8_t status
		, boost::uuids::uuid pk)
	{
		//
		//	read data block
		//
		CYNG_LOG_DEBUG(logger_, "decoded data of "
			<< sml::from_server_id(srv_id)
			<< ": "
			<< cyng::io::to_hex(data));

	}

	void session::cb_value_decoded(cyng::buffer_t const& srv_id
		, cyng::object const& val
		, std::uint8_t scaler
		, mbus::units unit
		, sml::obis code
		, boost::uuids::uuid pk)
	{
		CYNG_LOG_TRACE(logger_, "readout (wireless mBus) "
			<< sml::from_server_id(srv_id)
			<< " value: "
			<< cyng::io::to_type(val)
			<< " "
			<< mbus::get_unit_name(unit));

	}

	void session::sml_msg(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		auto const frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	get message body
		//
		auto const msg = cyng::to_tuple(frame.at(0));
		auto const pk = cyng::value_cast(frame.at(2), boost::uuids::nil_uuid());

		//
		//	add generated instruction vector
		//	"sml.get.list.response"
		//
		ctx.queue(sml::reader::read(msg, pk));
	}

	void session::sml_get_list_response(cyng::context& ctx)
	{
		//	
		//	example:
		//	[	.,
		//		01A815743145040102,
		//		990000000003,
		//		%(("abortOnError":0),("actGatewayTime":null),("actSensorTime":null),("code":0701),("crc16":ba45),("groupNo":0),("listSignature":null),("values":
		//		%(("0100010800FF":%(("raw":14521),("scaler":-1),("status":00020240),("unit":1e),("valTime":null),("value":1452.1))),
		//		("0100010801FF":%(("raw":0),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":0))),
		//		("0100010802FF":%(("raw":14521),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":1452.1))),
		//		("0100020800FF":%(("raw":561139),("scaler":-1),("status":00020240),("unit":1e),("valTime":null),("value":56113.9))),
		//		("0100020801FF":%(("raw":0),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":0))),
		//		("0100020802FF":%(("raw":561139),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":56113.9))),
		//		("0100100700FF":%(("raw":0),("scaler":-1),("status":null),("unit":1b),("valTime":null),("value":0))))))]
		//
		//	* [string] trx
		//	* [buffer] server id
		//	* [buffer] OBIS code
		//	* [obj] values
		//	* [uuid] pk
		//	

		cyng::vector_t const frame = ctx.get_frame();
		//CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	get server/meter id
		//
		auto const srv_id = cyng::to_buffer(frame.at(1));
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << sml::from_server_id(srv_id));

		auto const params = cyng::to_param_map(frame.at(3));
		auto const pos = params.find("values");
		if (pos != params.end()) {
			auto const values = cyng::to_param_map(pos->second);
			for (auto const& val : values) {

				//CYNG_LOG_DEBUG(logger_, val.first << " = " << cyng::io::to_str(val.second));

				auto const code = sml::parse_obis(val.first);
				if (code.second) {

					auto const data = cyng::to_param_map(val.second);
					auto const raw = data.at("raw");
					auto type = static_cast<std::uint32_t>(raw.get_class().tag());
					CYNG_LOG_DEBUG(logger_, val.first
						<< " = "
						<< cyng::io::to_str(data.at("value"))
						<< ':'
						<< raw.get_class().type_name());

					//tbl->insert(cyng::table::key_generator(pk, code.first.to_buffer())
					//	, cyng::table::data_generator(cyng::io::to_str(raw), type, data.at("scaler"), data.at("unit"))
					//	, 1u	//	generation
					//	, cache_.get_tag());
				}
				else {
					//	invalid OBIS code
				}
			}
		}
		else {
			CYNG_LOG_WARNING(logger_, "sml_get_list_response(" << sml::from_server_id(srv_id) << ") has no values");
		}
	}

}
