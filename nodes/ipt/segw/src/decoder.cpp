/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "decoder.h"
#include "cache.h"

#include <smf/mbus/defs.h>
#include <smf/mbus/header.h>
#include <smf/mbus/aes.h>
#include <smf/mbus/variable_data_block.h>
#include <smf/mbus/units.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/parser.h>

#include <cyng/crypto/aes_keys.h>
#include <cyng/io/io_buffer.h>
#include <cyng/io/serializer.h>
#include <cyng/vm/controller.h>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	decoder_wired_mbus::decoder_wired_mbus(cyng::logging::log_ptr logger
		, cache& cfg
		, cyng::controller& vm)
	: logger_(logger)
		, cache_(cfg)
		, vm_(vm)
		, uuidgen_()
	{}

	bool decoder_wired_mbus::read_short_frame(std::uint8_t,		//	C-field
		std::uint8_t,		//	A-field
		std::uint8_t,		//	checksum
		cyng::buffer_t const& payload)
	{
		return false;
	}

	bool decoder_wired_mbus::read_long_frame(std::uint8_t,		//	C-field
		std::uint8_t a,		//	A-field
		std::uint8_t ci,		//	CI-field
		std::uint8_t cs,		//	checksum
		cyng::buffer_t const& payload)
	{
		switch (ci) {
		case mbus::FIELD_CI_RESET: //	Reset(EN 13757-3)
		case mbus::FIELD_CI_DATA_SEND: //	Data send - mode 1 (EN 13757-3)
		case mbus::FIELD_CI_SLAVE_SELECT: //Slave select - mode 1 (EN 13757-3)
		case mbus::FIELD_CI_APPLICATION_RESET: //Application reset or select (EN 13757-3).
		case mbus::FIELD_CI_CMD_TO_DEVICE_SHORT: //	CMD to device with short header (OMS Vol.2 Issue 2.0.0/2009-07-20)								
		case mbus::FIELD_CI_CMD_TO_DEVICE_LONG: //	CMD to device with long header (OMS Vol.2 Issue 2.0.0/2009-07-20)
		case mbus::FIELD_CI_CMD_TLS_HANDSHAKE: //	Security Management (TLS-Handshake)
		case mbus::FIELD_CI_CMD_DLMS_LONG:
		case mbus::FIELD_CI_CMD_DLMS_SHORT:
		case mbus::FIELD_CI_CMD_SML_LONG:
		case mbus::FIELD_CI_CMD_SML_SHORT:
		case mbus::FIELD_CI_TIME_SYNC_1:	//	Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_TIME_SYNC_2: //	Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ERROR_SHORT: //	Error from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ERROR_LONG: //	Error from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ERROR:	//		Report of general application errors (EN 13757-3)
		case mbus::FIELD_CI_ALARM:	//		report of alarms (EN 13757-3)
		case mbus::FIELD_CI_HEADER_LONG:	//	12 byte header followed by variable format data (EN 13757-3)
			return read_frame_header_long(payload);
		case mbus::FIELD_CI_APL_ALARM_SHORT:	//!	<Alarm from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ALARM_LONG:	//	Alarm from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_HEADER_NO:	//	Variable data format respond without header (EN 13757-3)
		case mbus::FIELD_CI_HEADER_SHORT:	//	4 byte header followed by variable data format respond (EN 13757-3)
		case mbus::FIELD_CI_RES_LONG_DLMS:
		case mbus::FIELD_CI_RES_SHORT_DLSM:	//	short header
		case mbus::FIELD_CI_RES_LONG_SML:
		case mbus::FIELD_CI_RES_SHORT_SML:	//	short header
		case mbus::FIELD_CI_LINK_TO_DEVICE_LONG:	//	Link extension to device with long header (OMS Vol.2 Issue 2.0.02009-07-20)
		case mbus::FIELD_CI_LINK_FROM_DEVICE_SHORT:	//	Link extension from device with short header
		case mbus::FIELD_CI_LINK_FROM_DEVICE_LONG:	//	Link extension from device with long header(OMS Vol.2 Issue 2.0.0/2009-07-20)

		case mbus::FIELD_CI_EXT_DLL_I:	//	Additional Link Layer may be applied for Radio messages with or without Application Layer. 
		case mbus::FIELD_CI_EXT_DLL_II:	//	Additional Link Layer may be applied for Radio messages with or without Application Layer. 

		case mbus::FIELD_CI_EXT_DLL_III:	//	Lower Layer Service (10 Byte)

		case mbus::FIELD_CI_AUTH_LAYER:	//	Authentication and Fragmentation Layer (Lower Layer Service)

		case mbus::FIELD_CI_RES_TLS_SHORT:	//	Security Management (TLS-Handshake)
		case mbus::FIELD_CI_RES_TLS_LONG:	//	Security Management (TLS-Handshake)


		case mbus::FIELD_CI_MANU_SPEC:	//	Manufacture specific CI-Field
		case mbus::FIELD_CI_MANU_NO:	//	Manufacture specific CI-Field with no header
		case mbus::FIELD_CI_MANU_SHORT:	//	Manufacture specific CI-Field with short header
		case mbus::FIELD_CI_MANU_LONG:	//	Manufacture specific CI-Field with long header
		case mbus::FIELD_CI_MANU_SHORT_RF:	//	Manufacture specific CI-Field with short header (for RF-Test)

		case mbus::FIELD_CU_BAUDRATE_300:
		case mbus::FIELD_CU_BAUDRATE_600:
		case mbus::FIELD_CU_BAUDRATE_1200:
		case mbus::FIELD_CU_BAUDRATE_2400:
		case mbus::FIELD_CU_BAUDRATE_4800:
		case mbus::FIELD_CU_BAUDRATE_9600:
		case mbus::FIELD_CU_BAUDRATE_19200:
		case mbus::FIELD_CU_BAUDRATE_38400:

		case mbus::FIELD_CI_NULL:	//	No CI-field transmitted.
			break;

		default:
			break;
		}
		return false;
	}

	bool decoder_wired_mbus::read_ctrl_frame(std::uint8_t,		//	C-field
		std::uint8_t a,		//	A-field
		std::uint8_t ci,	//	CI-field
		std::uint8_t cs)	//	checksum
	{
		switch (ci) {
		case mbus::FIELD_CI_RESET: //	Reset(EN 13757-3)
		case mbus::FIELD_CI_DATA_SEND: //	Data send - mode 1 (EN 13757-3)
		case mbus::FIELD_CI_SLAVE_SELECT: //Slave select - mode 1 (EN 13757-3)
		case mbus::FIELD_CI_APPLICATION_RESET: //Application reset or select (EN 13757-3).
		case mbus::FIELD_CI_CMD_TO_DEVICE_SHORT: //	CMD to device with short header (OMS Vol.2 Issue 2.0.0/2009-07-20)								
		case mbus::FIELD_CI_CMD_TO_DEVICE_LONG: //	CMD to device with long header (OMS Vol.2 Issue 2.0.0/2009-07-20)
		case mbus::FIELD_CI_CMD_TLS_HANDSHAKE: //	Security Management (TLS-Handshake)
		case mbus::FIELD_CI_CMD_DLMS_LONG: 
		case mbus::FIELD_CI_CMD_DLMS_SHORT:
		case mbus::FIELD_CI_CMD_SML_LONG:
		case mbus::FIELD_CI_CMD_SML_SHORT:
		case mbus::FIELD_CI_TIME_SYNC_1:	//	Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_TIME_SYNC_2: //	Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ERROR_SHORT: //	Error from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ERROR_LONG: //	Error from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ERROR:	//		Report of general application errors (EN 13757-3)
		case mbus::FIELD_CI_ALARM:	//		report of alarms (EN 13757-3)
		case mbus::FIELD_CI_HEADER_LONG:	//	12 byte header followed by variable format data (EN 13757-3)
			break;
		case mbus::FIELD_CI_APL_ALARM_SHORT:	//!	<Alarm from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_APL_ALARM_LONG:	//	Alarm from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
		case mbus::FIELD_CI_HEADER_NO:	//	Variable data format respond without header (EN 13757-3)
		case mbus::FIELD_CI_HEADER_SHORT:	//	4 byte header followed by variable data format respond (EN 13757-3)
		case mbus::FIELD_CI_RES_LONG_DLMS:
		case mbus::FIELD_CI_RES_SHORT_DLSM:	//	short header
		case mbus::FIELD_CI_RES_LONG_SML:
		case mbus::FIELD_CI_RES_SHORT_SML:	//	short header
		case mbus::FIELD_CI_LINK_TO_DEVICE_LONG:	//	Link extension to device with long header (OMS Vol.2 Issue 2.0.02009-07-20)
		case mbus::FIELD_CI_LINK_FROM_DEVICE_SHORT:	//	Link extension from device with short header
		case mbus::FIELD_CI_LINK_FROM_DEVICE_LONG:	//	Link extension from device with long header(OMS Vol.2 Issue 2.0.0/2009-07-20)

		case mbus::FIELD_CI_EXT_DLL_I:	//	Additional Link Layer may be applied for Radio messages with or without Application Layer. 
		case mbus::FIELD_CI_EXT_DLL_II:	//	Additional Link Layer may be applied for Radio messages with or without Application Layer. 
	
		case mbus::FIELD_CI_EXT_DLL_III:	//	Lower Layer Service (10 Byte)

		case mbus::FIELD_CI_AUTH_LAYER:	//	Authentication and Fragmentation Layer (Lower Layer Service)

		case mbus::FIELD_CI_RES_TLS_SHORT:	//	Security Management (TLS-Handshake)
		case mbus::FIELD_CI_RES_TLS_LONG:	//	Security Management (TLS-Handshake)


		case mbus::FIELD_CI_MANU_SPEC:	//	Manufacture specific CI-Field
		case mbus::FIELD_CI_MANU_NO:	//	Manufacture specific CI-Field with no header
		case mbus::FIELD_CI_MANU_SHORT:	//	Manufacture specific CI-Field with short header
		case mbus::FIELD_CI_MANU_LONG:	//	Manufacture specific CI-Field with long header
		case mbus::FIELD_CI_MANU_SHORT_RF:	//	Manufacture specific CI-Field with short header (for RF-Test)

		case mbus::FIELD_CU_BAUDRATE_300:
		case mbus::FIELD_CU_BAUDRATE_600:
		case mbus::FIELD_CU_BAUDRATE_1200:
		case mbus::FIELD_CU_BAUDRATE_2400:
		case mbus::FIELD_CU_BAUDRATE_4800:
		case mbus::FIELD_CU_BAUDRATE_9600:
		case mbus::FIELD_CU_BAUDRATE_19200:
		case mbus::FIELD_CU_BAUDRATE_38400:

		case mbus::FIELD_CI_NULL:	//	No CI-field transmitted.
			break;

		default:
			break;
		}
		return false;
	}

	bool decoder_wired_mbus::read_frame_header_long(cyng::buffer_t const& payload)
	{
		//	0 = wired
		auto r1 = make_header_long_wired(payload);
		if (r1.second) {

			CYNG_LOG_DEBUG(logger_, "access nr   : " << +r1.first.get_access_no());

			auto const server_id = r1.first.get_srv_id();
			auto const manufacturer = sml::get_manufacturer_code(server_id);
			auto const mbus_status = r1.first.get_status();	//	OBIS_MBUS_STATE (00 00 61 61 00 FF)

			//
			//	signature consist of 2 bytes
			//
			auto const signature = r1.first.get_signature();

			CYNG_LOG_DEBUG(logger_, "server ID   : " << sml::from_server_id(server_id));	//	OBIS_SERIAL_NR - 00 00 60 01 00 FF
			CYNG_LOG_DEBUG(logger_, "medium      : " << mbus::get_medium_name(sml::get_medium_code(server_id)));
			CYNG_LOG_DEBUG(logger_, "manufacturer: " << sml::decode(manufacturer));	//	OBIS_DATA_MANUFACTURER - 81 81 C7 82 03 FF
			CYNG_LOG_DEBUG(logger_, "device id   : " << sml::get_serial(server_id));
			CYNG_LOG_DEBUG(logger_, "status      : " << +mbus_status);
			CYNG_LOG_DEBUG(logger_, "signature   : " << +signature);

			//
			// update device table
			//
			//if (cache_.update_device_table(server_id
			//	, sml::decode(manufacturer)
			//	, mbus_status
			//	, sml::get_version(server_id)
			//	, sml::get_medium_code(server_id)
			//	, cyng::crypto::aes_128_key{}
			//	, vm_.tag())) {

			//	CYNG_LOG_INFO(logger_, "new device: "
			//		<< sml::from_server_id(server_id));
			//}

			//
			//	populate cache with new readout
			//
			auto const pk = uuidgen_();

			//
			//	read data block
			//
			read_variable_data_block(server_id, r1.first, pk);

			//	//
			//	//	"_Readout"
			//	//
			//	cache_.write_table("_Readout", [&](cyng::store::table* tbl) {
			//		tbl->insert(cyng::table::key_generator(pk)
			//			, cyng::table::data_generator(server_id, std::chrono::system_clock::now(), mbus_status)
			//			, 1u	//	generation
			//			, cache_.get_tag());
			//		});

			return true;
		}
		return false;
	}

	void decoder_wired_mbus::read_variable_data_block(cyng::buffer_t const& srv_id
		, header_long_wired const& hl
		, boost::uuids::uuid pk)
	{
		vdb_reader reader(srv_id);
		std::size_t offset{ 0 };	//	2 x 0x2F
		auto const data = hl.data();
		while (offset < data.size()) {

			//
			//	read block
			//
			offset = reader.decode(data, offset);

			auto const val = cyng::io::to_str(reader.get_value());
			auto const type = static_cast<std::uint32_t>(reader.get_value().get_class().tag());
			auto const unit = static_cast<std::uint8_t>(reader.get_unit());

			CYNG_LOG_TRACE(logger_, "readout (wired mBus) "
				<< sml::from_server_id(srv_id)
				<< " value: "
				<< val
				<< " "
				<< mbus::get_unit_name(unit));

			//
			//	cache: "_ReadoutData"
			//
			if (reader.is_valid()) {




			}
		}
	}


	decoder_wireless_mbus::decoder_wireless_mbus(cyng::logging::log_ptr logger
		, cache& cfg
		, cyng::controller& vm)
	: logger_(logger)
		, cache_(cfg)
		, vm_(vm)
		, uuidgen_()
	{}

	/*
	 * The first 12 bytes of the user data consist of a block with a fixed size and structure.
	 *
	 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
	 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
	 */
	bool decoder_wireless_mbus::read_frame_header_long(cyng::buffer_t const& srv, cyng::buffer_t const& data)
	{
		//	1 = wireless
		auto r1 = make_header_long_wireless(data);
		if (r1.second) {

			CYNG_LOG_DEBUG(logger_, "access nr   : " << +r1.first.get_access_no());

			auto const server_id = r1.first.get_srv_id();
			auto const manufacturer = sml::get_manufacturer_code(server_id);
			auto const mbus_status = r1.first.get_status();	//	OBIS_MBUS_STATE (00 00 61 61 00 FF)

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r1.first.get_mode();

			CYNG_LOG_DEBUG(logger_, "server ID   : " << sml::from_server_id(server_id));	//	OBIS_SERIAL_NR - 00 00 60 01 00 FF
			CYNG_LOG_DEBUG(logger_, "medium      : " << mbus::get_medium_name(sml::get_medium_code(server_id)));
			CYNG_LOG_DEBUG(logger_, "manufacturer: " << sml::decode(manufacturer));	//	OBIS_DATA_MANUFACTURER - 81 81 C7 82 03 FF
			CYNG_LOG_DEBUG(logger_, "device id   : " << sml::get_serial(server_id));
			CYNG_LOG_DEBUG(logger_, "status      : " << +mbus_status);
			CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);

			//
			// update device table
			//
			if (cache_.update_device_table(server_id
				, sml::decode(manufacturer)
				, mbus_status
				, sml::get_version(server_id)
				, sml::get_medium_code(server_id)
				, cyng::crypto::aes_128_key{}
				, vm_.tag())) {

				CYNG_LOG_INFO(logger_, "new device: "
					<< sml::from_server_id(server_id));
			}

			auto const prefix = r1.first.get_prefix();
			auto r2 = this->decode_data(server_id, prefix);
			if (r2.second) {

				//
				//	populate cache with new readout
				//
				auto const pk = uuidgen_();

				//
				//	read data block
				//
				CYNG_LOG_DEBUG(logger_, "decoded data: "
					<< cyng::io::to_hex(r2.first));

				read_variable_data_block(server_id
					, r1.first.get_block_counter()
					, r2.first
					, pk);

				//
				//	"_Readout"
				//
				cache_.write_table("_Readout", [&](cyng::store::table* tbl) {
					tbl->insert(cyng::table::key_generator(pk)
						, cyng::table::data_generator(server_id, std::chrono::system_clock::now(), mbus_status)
						, 1u	//	generation
						, cache_.get_tag());
				});

			}
			else {

				CYNG_LOG_WARNING(logger_, "cannot read long header of " << sml::from_server_id(srv));
			}
			return r2.second;
		}
		return false;
	}

	/**
	 * Applied from slave with CI = 0x7A
	 */
	bool decoder_wireless_mbus::read_frame_header_short(cyng::buffer_t const& srv, cyng::buffer_t const& data)
	{
		auto const manufacturer = sml::get_manufacturer_code(srv);

		auto const prefix = make_wireless_prefix(data.begin(), data.end());
		if (prefix.second) {

			CYNG_LOG_DEBUG(logger_, "access nr: " << +prefix.first.get_access_no());
			CYNG_LOG_DEBUG(logger_, "manufacturer: " << manufacturer);
			CYNG_LOG_DEBUG(logger_, "status      : " << +prefix.first.get_status());	//	mBus status
			CYNG_LOG_DEBUG(logger_, "mode        : " << +prefix.first.get_mode());	//	AES mode

			//auto r2 = decode_data(aes_mode, srv, buffer);
			//if (r2.second) {

			//	//
			//	//	read data block
			//	//
			//	CYNG_LOG_DEBUG(logger_, "decoded data: "
			//		<< cyng::io::to_hex(r2.first.data()));

			//	//
			//	//	ToDo: read
			//	//
			//}
			//else {
			//	CYNG_LOG_WARNING(logger_, "cannot read short header (CI = 0x7A) of " << sml::from_server_id(srv));
			//}

			return prefix.second;
		}
		return false;
	}

	bool decoder_wireless_mbus::read_frame_header_short_sml(cyng::buffer_t const& srv_id, cyng::buffer_t const& data)
	{
		auto const manufacturer = sml::get_manufacturer_code(srv_id);
		auto const prefix = make_wireless_prefix(data.begin(), data.end());

		if (prefix.second) {


			CYNG_LOG_DEBUG(logger_, sml::decode(manufacturer)
				<< ": "
				<< mbus::get_medium_name(sml::get_medium_code(srv_id))
				<< " - "
				<< sml::from_server_id(srv_id)
				<< std::hex
				<< ", access #0x"
				<< +prefix.first.get_access_no()
				<< ", status: 0x"
				<< +prefix.first.get_status()
				<< ", mode: "
				<< std::dec
				<< +prefix.first.get_mode()
				<< ", counter: "
				<< +prefix.first.blocks()	//	 V * 16 Bytes
			);

			//
			//	encrypt data
			//
			auto r2 = decode_data(srv_id, prefix.first);
			if (r2.second) {

#ifdef __DEBUG
				cyng::io::hex_dump hd;
				std::stringstream ss;
				hd(ss, r2.first.data_raw().begin(), r2.first.data_raw().end());

				CYNG_LOG_TRACE(logger_, "SML data:\n"
					<< ss.str());

#endif
				//
				//	populate cache with new readout
				//
				auto const pk = uuidgen_();

				//
				//	"_Readout"
				//
				cache_.write_table("_Readout", [&](cyng::store::table* tbl) {
					tbl->insert(cyng::table::key_generator(pk)
						, cyng::table::data_generator(srv_id, std::chrono::system_clock::now(), prefix.first.get_status())
						, 1u	//	generation
						, cache_.get_tag());
				});

				//
				//	start SML parser
				//
				sml::parser sml_parser([&](cyng::vector_t&& prg) {

					//
					//	execute "sml.msg" and "sml.eom" instructions
					//
					//CYNG_LOG_TRACE(logger_, "SML instructions: "
					//	<< cyng::io::to_str(prg));

					vm_.async_run(std::move(prg));

				}, false, false, false, pk);	//	not verbose, no logging

				//
				//	parse raw data
				//
				sml_parser.read(r2.first.begin() + 2, r2.first.end());
			}
			else {
				CYNG_LOG_WARNING(logger_, "cannot read short header (CI = 0x7F) of " << sml::from_server_id(srv_id));
			}

			return r2.second;
		}
		return false;
	}

	std::pair<cyng::buffer_t, bool> decoder_wireless_mbus::decode_data(cyng::buffer_t const& server_id
		, wireless_prefix const& prefix)
	{
		switch (prefix.get_mode()) {
		case 0:	return std::make_pair(prefix.get_data(), true);	//	not decrypted
		case 5:	return decode_data_mode_5(server_id, prefix);
		case 7:
		case 13:
		default:
			break;
		}

		CYNG_LOG_ERROR(logger_, "AES mode "
			<< +prefix.get_mode()
			<< " is not supported - server ID: "
			<< sml::from_server_id(server_id));

		return std::make_pair(cyng::buffer_t{}, false);
	}

	std::pair<cyng::buffer_t, bool> decoder_wireless_mbus::decode_data_mode_5(cyng::buffer_t const& server_id
		, wireless_prefix const& prefix)
	{
		//
		//	get AES key and decrypt content (AES CBC - mode 5)
		//
		cyng::crypto::aes_128_key key = cache_.get_aes_key(server_id);

		//
		//	decode payload data
		//
		return prefix.decode_mode_5(server_id, key);
	}

	void decoder_wireless_mbus::read_variable_data_block(cyng::buffer_t const& server_id
		, std::uint8_t const blocks
		, cyng::buffer_t const& raw_data
		, boost::uuids::uuid pk)
	{
		//
		//	get number of encrypted bytes
		//
		if (0 == blocks) {

			//	not encrypted
		}
		else if (0x0F == blocks) {
			// partial encryption disabled
		}
		else {
			//
			//	multiples of 16
			//
			auto size = 16u * static_cast<std::size_t>(blocks);

			CYNG_LOG_DEBUG(logger_, "read_variable_data_block("
				<< size
				<< "/"
				<< raw_data.size()
				<< " bytes, meter "
				<< sml::from_server_id(server_id)
				<< ")");

			vdb_reader reader(server_id);
			std::size_t offset{ 2 };	//	2 x 0x2F
			while (offset < size) {

				//
				//	read block
				//
				offset = reader.decode(raw_data, offset);
				if (offset == size) {
					std::cout << std::endl;
				}

				auto const val = cyng::io::to_str(reader.get_value());
				auto const type = static_cast<std::uint32_t>(reader.get_value().get_class().tag());
				auto const unit = static_cast<std::uint8_t>(reader.get_unit());

				CYNG_LOG_TRACE(logger_, "readout (wireless mBus) "
					<< sml::from_server_id(server_id)
					<< " value: "
					<< val
					<< " "
					<< mbus::get_unit_name(unit));

				//
				//	cache: "_ReadoutData"
				//
				if (reader.is_valid()) {

					cache_.write_table("_ReadoutData", [&](cyng::store::table* tbl) {

						tbl->insert(cyng::table::key_generator(pk, reader.get_code().to_buffer())
							, cyng::table::data_generator(val, type, reader.get_scaler(), unit)
							, 1u	//	generation
							, cache_.get_tag());
					});
				}
			}
		}
	}
}

