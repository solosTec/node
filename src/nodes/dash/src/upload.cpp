/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>
#include <upload.h>

#include <smf/config/protocols.h>
#include <smf/mbus/flag_id.h>
#include <smf/mbus/server_id.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/array_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/value_cast.hpp>
#include <cyng/parse/csv.h>
//#include <cyng/parse/csv/line_cast.hpp>
#include <cyng/parse/string.h>

#include <iostream>

#include <boost/uuid/uuid_io.hpp>

namespace smf {

    upload::upload(cyng::logger logger, bus &cluster_bus, db &cache)
        : logger_(logger)
        , cluster_bus_(cluster_bus)
        , db_(cache)
        , uidgen_() {}

    void upload::config_bridge(std::string name, upload_policy policy, std::string const &content, char sep) {

        CYNG_LOG_DEBUG(logger_, "[upload] content: " << content);

        // std::size_t counter{0};
        cyng::csv::parser_named p(
            [&, this](std::map<std::string, std::string> const &data, std::size_t line_number) {
                if (!data.empty()) {

                    //	Meter_ID,Ift_Type,GWY_IP,Port,Manufacturer,Meter_Type,Protocol,Area,Name,In_enDS,Key,AMR Address,Comments
                    //	[0] Meter_ID (MA0000000000000000000000004354752)
                    //	[1] Ift_Type (wMBus)
                    //	[2] GWY_IP (10.132.32.142)
                    //	[3] Port (6000)
                    //	[4]	Manufacturer (EMH)
                    //	[5] Meter_Type (EMH ED100L)
                    //	[6] Protocol [Wireless MBUS|IEC]
                    //	[7] Area (Moumnat)
                    //  [8] Name (Maison 44)
                    //	[9] In_enDS (Yes)
                    //	[10] Key (2BFFCB61D7E8DC439239555D3DFE1B1D)
                    //	[11] AMR Address,
                    //	[12] Comments

                    //
                    //	extract Meter_ID
                    //	have to contain at least 8 bytes
                    //
                    auto const mc = extract_value(data, "Meter_ID", "");
                    if (mc.size() < 8)
                        return;

                    //	Meter_ID,Ift_Type,GWY_IP,Manufacturer,Meter_Type,Protocol,Area,Name,In_enDS,Key,AMR Address,Comments
                    std::string meter_id = (mc.size() > 8) ? mc.substr(mc.length() - 8) : mc;

                    //	Ift_Type [RS485|wMBus]
                    auto const adapter_type = extract_value(data, "Ift_Type", "RS485");

                    //	ip address
                    auto const address = extract_value(data, "GWY_IP", "0.0.0.0");
                    auto const port = cyng::to_numeric<std::uint16_t>(extract_value(data, "Ports", "6000"));

                    //	Manufacturer
                    auto const manufacturer_code = cleanup_manufacturer_code(extract_value(data, "Manufacturer", "generic"));

                    //	get the server ID
                    auto const server_id = cleanup_server_id(
                        meter_id,
                        manufacturer_code,
                        boost::algorithm::equals(adapter_type, "RS485"), //	true == wired
                        1,                                               //	version
                        2                                                //	electricity
                    );

                    auto const meter_type = extract_value(data, "Meter_Type", "generic");
                    auto const protocol = extract_value(data, "Protocol", "IEC 62056");
                    if (!boost::algorithm::equals(protocol, "IEC 62056") && !boost::algorithm::equals(protocol, "Wireless MBUS")) {

                        cluster_bus_.sys_msg(
                            cyng::severity::LEVEL_WARNING,
                            "[upload] no or unknown protocol specified for meter[",
                            meter_id,
                            "]",
                            meter_type);
                        return;
                    }
                    auto const area = extract_value(data, "Area", "area");
                    auto const name = extract_value(data, "Name", "name");

                    auto const aes_key = extract_value(data, "Key", "00000000000000000000000000000000");
                    auto const aes = cyng::to_aes_key<cyng::crypto::aes128_size>(
                        (aes_key.size() != 32) ? "00000000000000000000000000000000" : aes_key);

                    //
                    //	Search for an existing meter with the same ID.
                    //	If meter ID already exists use this PK otherwise create a new one.
                    //  The key is from table "meter".
                    //
                    auto key = db_.lookup_meter_by_id(meter_id);
                    switch (policy) {
                    case upload_policy::APPEND:
                        //	generate new key
                        CYNG_LOG_DEBUG(logger_, "[upload] #" << line_number << ": append " << meter_id);
                        if (boost::algorithm::equals(protocol, "IEC 62056")) {
                            insert_iec(mc, server_id, meter_id, address, port, meter_type, area, name, manufacturer_code);
                        } else {
                            insert_wmbus(mc, server_id, meter_id, meter_type, aes, area, name, manufacturer_code);
                        }
                        break;
                    case upload_policy::MERGE:
                        if (key.empty()) {
                            //	insert
                            CYNG_LOG_DEBUG(logger_, "[upload] #" << line_number << ": insert " << meter_id);
                            if (boost::algorithm::equals(protocol, "IEC 62056")) {
                                insert_iec(mc, server_id, meter_id, address, port, meter_type, area, name, manufacturer_code);
                            } else {
                                insert_wmbus(mc, server_id, meter_id, meter_type, aes, area, name, manufacturer_code);
                            }
                        } else {
                            //	update
                            CYNG_LOG_DEBUG(logger_, "[upload] #" << line_number << ": update " << meter_id);
                            if (boost::algorithm::equals(protocol, "IEC 62056")) {
                                update_iec(key, mc, server_id, meter_id, address, port, meter_type, area, name, manufacturer_code);
                            } else {
                                update_wmbus(key, mc, server_id, meter_id, meter_type, aes, area, name, manufacturer_code);
                            }
                        }
                        break;
                    case upload_policy::OVERWRITE:
                        if (!key.empty()) {
                            //	remove
                            //	insert again
                            CYNG_LOG_DEBUG(logger_, "[upload] #" << line_number << ": overwrite " << meter_id);
                            cluster_bus_.req_db_remove("device", key);
                            cluster_bus_.req_db_remove("meter", key);
                            if (boost::algorithm::equals(protocol, "IEC 62056")) {
                                cluster_bus_.req_db_remove("meterIEC", key);
                                insert_iec(mc, server_id, meter_id, address, port, meter_type, area, name, manufacturer_code);
                            } else {
                                cluster_bus_.req_db_remove("meterwMBus", key);
                                insert_wmbus(mc, server_id, meter_id, meter_type, aes, area, name, manufacturer_code);
                            }
                        } else {
                            CYNG_LOG_WARNING(logger_, "[upload] overwrite " << meter_id << " not found");
                        }
                        break;
                    case upload_policy::REMOVE:
                        //  remove entries from table "meter", "device", "meterwMBus" and "meterIEC"
                        if (!key.empty()) {
                            CYNG_LOG_DEBUG(logger_, "[upload] #" << line_number << ": delete " << meter_id);
                            cluster_bus_.req_db_remove("device", key);
                            cluster_bus_.req_db_remove("meter", key);
                            if (boost::algorithm::equals(protocol, "IEC 62056")) {
                                cluster_bus_.req_db_remove("meterIEC", key);
                            } else {
                                cluster_bus_.req_db_remove("meterwMBus", key);
                            }
                        }
                        break;
                    default:
                        CYNG_LOG_ERROR(logger_, "[upload] unknown upload policy");
                        break;
                    }
                }
            },
            ',');

        p.read(content.begin(), content.end());
    }

    void upload::insert_iec(
        std::string const &mc,
        std::string const &server_id,
        std::string const &meter_id,
        std::string const &address,
        std::uint16_t port,
        std::string const &meter_type,
        std::string const &area,
        std::string const &name,
        std::string const &maker) {

        auto const tag = uidgen_();
        auto const key = cyng::key_generator(tag);

        // auto country_code = db_.cfg_.get_value("country-code", "CH");
        // auto const mc = gen_metering_code(country_code, tag);

        //
        //	convert data types
        //
        CYNG_LOG_TRACE(logger_, "[upload] insert IEC device " << mc);
        cluster_bus_.req_db_insert(
            "device",
            key,
            db_.complete(
                "device",
                cyng::param_map_factory("name", mc)("pwd", meter_id)("msisdn", meter_id)("descr", area + ", " + name)(
                    "id", meter_type)("enabled", true)),
            0);

        cluster_bus_.req_db_insert(
            "meter",
            key,
            db_.complete(
                "meter",
                cyng::param_map_factory("ident", server_id)           //  example: 01-e61e-13090016-3c-07
                ("meter", meter_id)                                   //  example: 16000913
                ("code", mc)                                          //  global unique metering code
                ("maker", maker)                                      //  3 character code
                ("protocol", config::get_name(config::protocol::IEC)) //  protocol enum
                ("item", meter_type)("factoryNr", boost::uuids::to_string(tag))),
            0);

        cluster_bus_.req_db_insert(
            "meterIEC",
            key,
            db_.complete(
                "meterIEC", cyng::param_map_factory("host", address)("port", port)("interval", std::chrono::seconds(60 * 60))),
            0);
    }
    void upload::insert_wmbus(
        std::string const &mc,
        std::string const &server_id,
        std::string const &meter_id,
        std::string const &meter_type,
        cyng::crypto::aes_128_key const &aes,
        std::string const &area,
        std::string const &name,
        std::string const &maker) {

        //
        //
        // check AES consistency
        auto const buffer = cyng::to_buffer(aes);
        if (cyng::is_ascii(buffer)) {
            std::string const str(buffer.begin(), buffer.end());
            cluster_bus_.sys_msg(
                cyng::severity::LEVEL_WARNING,
                "[upload] AES key for meter[",
                meter_id,
                "] is most likely wrong - did you mean ",
                str);
        }

        auto const tag = uidgen_();
        auto const key = cyng::key_generator(tag);

        CYNG_LOG_TRACE(logger_, "[upload] insert wM-Bus device " << mc);
        cluster_bus_.req_db_insert(
            "device",
            key,
            db_.complete(
                "device",
                cyng::param_map_factory("name", mc)("pwd", meter_id)("msisdn", meter_id)("descr", area + ", " + name)(
                    "id", meter_type)("enabled", true)),
            0);
        cluster_bus_.req_db_insert(
            "meter",
            key,
            db_.complete(
                "meter",
                cyng::param_map_factory("ident", server_id)("meter", meter_id)(
                    "code",
                    mc)("maker", maker)("protocol", config::get_name(config::protocol::WIRED_MBUS))("item", boost::uuids::to_string(tag))),
            0);
        cluster_bus_.req_db_insert(
            "meterwMBus",
            key,
            db_.complete("meterwMBus", cyng::param_map_factory("host", boost::asio::ip::make_address("0.0.0.0"))("aes", aes)),
            0);
    }

    void upload::update_iec(
        cyng::key_t key,
        std::string const &mc,
        std::string const &server_id,
        std::string const &meter_id,
        std::string const &address,
        std::uint16_t port,
        std::string const &meter_type,
        std::string const &area,
        std::string const &name,
        std::string const &maker) {

        CYNG_LOG_TRACE(logger_, "[upload] update IEC device " << mc);
        // cluster_bus_.req_db_update("device", key, cyng::param_map_factory
        //	("name", mc)
        //	("pwd", meter_id)
        //	("msisdn", meter_id)
        //	("descr", name)
        //	("id", meter_type));

        cluster_bus_.req_db_update(
            "meter",
            key,
            cyng::param_map_factory("ident", server_id)("meter", meter_id)("code", mc)("maker", maker)("protocol", "IEC:62056"));

        cluster_bus_.req_db_update("meterIEC", key, cyng::param_map_factory("host", address)("port", port));
    }

    void upload::update_wmbus(
        cyng::key_t key,
        std::string const &mc,
        std::string const &server_id,
        std::string const &meter_id,
        std::string const &meter_type,
        cyng::crypto::aes_128_key const &aes,
        std::string const &area,
        std::string const &name,
        std::string const &maker) {

        CYNG_LOG_TRACE(logger_, "[upload] update wM-Bus device " << mc);
        // cluster_bus_.req_db_update("device", key, cyng::param_map_factory
        //	("name", mc)
        //	("pwd", meter_id)
        //	("msisdn", meter_id)
        //	("descr", name)
        //	("id", meter_type));
        cluster_bus_.req_db_update(
            "meter",
            key,
            cyng::param_map_factory("ident", server_id)("meter", meter_id)(
                "code", mc)("maker", maker)("protocol", "wM-Bus:EN13757-4"));
        cluster_bus_.req_db_update("meterwMBus", key, cyng::param_map_factory("aes", aes));
    }

    std::string cleanup_manufacturer_code(std::string manufacturer) {
        if (manufacturer.size() > 2) {
            if (boost::algorithm::istarts_with(manufacturer, "Easymeter"))
                return "ESY"; //	not offical
            // ABB	0442	ABB AB, P.O. Box 1005, SE-61129 Nyk�ping, Nyk�ping,Sweden
            // ACE	0465	Actaris (Elektrizit�t)
            // ACG	0467	Actaris (Gas)
            // ACW	0477	Actaris (Wasser und W�rme)
            else if (boost::algorithm::istarts_with(manufacturer, "Actaris"))
                return "ACE";
            // AEG	04A7	AEG
            // AEL	04AC	Kohler, T�rkei
            else if (boost::algorithm::istarts_with(manufacturer, "Kohler"))
                return "AEL";
            // AEM	04AD	S.C. AEM S.A. Romania
            else if (boost::algorithm::istarts_with(manufacturer, "S.C. AEM"))
                return "AEM";
            // AMP	05B0	Ampy Automation Digilog Ltd
            else if (boost::algorithm::istarts_with(manufacturer, "Ampy"))
                return "AMP";
            // AMT	05B4	Aquametro
            // APS	0613	Apsis Kontrol Sistemleri, T�rkei
            // BEC	08A3	Berg Energiekontrollsysteme GmbH
            // BER	08B2	Bernina Electronic AG
            // BSE	0A65	Basari Elektronik A.S., T�rkei
            // BST	0A74	BESTAS Elektronik Optik, T�rkei
            // CBI	0C49	Circuit Breaker Industries, S�dafrika
            // CLO	0D8F	Clorius Raab Karcher Energie Service A/S
            // CON	0DEE	Conlog
            // CZM	0F4D	Cazzaniga S.p.A.
            // DAN	102E	Danubia
            // DFS	10D3	Danfoss A/S
            // DME	11A5	DIEHL Metering, Industriestrasse 13, 91522 Ansbach, Germany
            else if (boost::algorithm::istarts_with(manufacturer, "Diehl"))
                return "DME";
            // DZG	1347	Deutsche Z�hlergesellschaft
            else if (boost::algorithm::istarts_with(manufacturer, "Deutsche Z�hlergesellschaft"))
                return "DZG";
            // EDM	148D	EDMI Pty.Ltd.
            // EFE	14C5	Engelmann Sensor GmbH
            // EKT	1574	PA KVANT J.S., Russland
            // ELM	158D	Elektromed Elektronik Ltd, T�rkei
            // ELS	1593	ELSTER Produktion GmbH
            else if (boost::algorithm::istarts_with(manufacturer, "Elster"))
                return "ELS";
            // EMH	15A8	EMH Elektrizit�tsz�hler GmbH & CO KG
            else if (boost::algorithm::istarts_with(manufacturer, "EMH"))
                return "EMH";
            // EMU	15B5	EMU Elektronik AG
            // EMO	15AF	Enermet
            // END	15C4	ENDYS GmbH
            // ENP	15D0	Kiev Polytechnical Scientific Research
            // ENT	15D4	ENTES Elektronik, T�rkei
            // ERL	164C	Erelsan Elektrik ve Elektronik, T�rkei
            // ESM	166D	Starion Elektrik ve Elektronik, T�rkei
            // EUR	16B2	Eurometers Ltd
            // EWT	16F4	Elin Wasserwerkstechnik
            // FED	18A4	Federal Elektrik, T�rkei
            // FML	19AC	Siemens Measurements Ltd.( Formerly FML Ltd.)
            // GBJ	1C4A	Grundfoss A/S
            // GEC	1CA3	GEC Meters Ltd.
            // GSP	1E70	Ingenieurbuero Gasperowicz
            // GWF	1EE6	Gas- u. Wassermessfabrik Luzern
            // HEG	20A7	Hamburger Elektronik Gesellschaft
            // HEL	20AC	Heliowatt
            // HTC	2283	Horstmann Timers and Controls Ltd.
            // HYD	2324	Hydrometer GmbH
            else if (boost::algorithm::istarts_with(manufacturer, "Hydrometer"))
                return "HYD";
            // ICM	246D	Intracom, Griechenland
            // IDE	2485	IMIT S.p.A.
            // INV	25D6	Invensys Metering Systems AG
            // ISK	266B	Iskraemeco, Slovenia
            else if (boost::algorithm::istarts_with(manufacturer, "Iskraemeco"))
                return "ISK";
            // IST	2674	ista Deutschland (bis 2005 Viterra Energy Services)
            // ITR	2692	Itron
            else if (boost::algorithm::istarts_with(manufacturer, "Itron"))
                return "ITR";
            // IWK	26EB	IWK Regler und Kompensatoren GmbH
            // KAM	2C2D	Kamstrup Energie A/S
            else if (boost::algorithm::istarts_with(manufacturer, "Kamstrup"))
                return "KAM";
            // KHL	2D0C	Kohler, T�rkei
            else if (boost::algorithm::istarts_with(manufacturer, "Kohler"))
                return "KHL";
            // KKE	2D65	KK-Electronic A/S
            // KNX	2DD8	KONNEX-based users (Siemens Regensburg)
            // KRO	2E4F	Kromschr�der
            // KST	2E74	Kundo SystemTechnik GmbH
            // LEM	30AD	LEM HEME Ltd., UK
            // LGB	30E2	Landis & Gyr Energy Management (UK) Ltd.
            else if (boost::algorithm::istarts_with(manufacturer, "Landis & Gyr Energy"))
                return "LGB";
            // LGD	30E4	Landis & Gyr Deutschland
            else if (boost::algorithm::istarts_with(manufacturer, "Landis & Gyr Deutschland"))
                return "LGD";
            // LGZ	30FA	Landis & Gyr Zug
            else if (boost::algorithm::istarts_with(manufacturer, "Landis & Gyr Zug"))
                return "LGZ";
            else if (boost::algorithm::istarts_with(manufacturer, "L & G"))
                return "LGZ"; //	switzerland / zug

            // LHA	3101	Atlantic Meters, S�dafrika
            // LML	31AC	LUMEL, Polen
            // LSE	3265	Landis & Staefa electronic
            // LSP	3270	Landis & Staefa production
            // LUG	32A7	Landis & Staefa
            // LSZ	327A	Siemens Building Technologies
            else if (boost::algorithm::istarts_with(manufacturer, "Siemens Building"))
                return "LSZ";
            // MAD	3424	Maddalena S.r.I., Italien
            // MEI	34A9	H. Meinecke AG (jetzt Invensys Metering Systems AG)
            // MKS	3573	MAK-SAY Elektrik Elektronik, T�rkei
            // MNS	35D3	MANAS Elektronik, T�rkei
            // MPS	3613	Multiprocessor Systems Ltd, Bulgarien
            // MTC	3683	Metering Technology Corporation, USA
            // NIS	3933	Nisko Industries Israel
            // NMS	39B3	Nisko Advanced Metering Solutions Israel
            // NRM	3A4D	Norm Elektronik, T�rkei
            // ONR	3DD2	ONUR Elektroteknik, T�rkei
            // PAD	4024	PadMess GmbH
            // PMG	41A7	Spanner-Pollux GmbH (jetzt Invensys Metering Systems AG)
            // PRI	4249	Polymeters Response International Ltd.
            // RAS	4833	Hydrometer GmbH
            // REL	48AC	Relay GmbH
            else if (boost::algorithm::istarts_with(manufacturer, "Relay"))
                return "REL";
            // RKE	4965	Raab Karcher ES (jetzt ista Deutschland)
            // SAP	4C30	Sappel
            // SCH	4C68	Schnitzel GmbH
            // SEN	4CAE	Sensus GmbH
            // SMC	4DA3
            // SME	4DA5	Siame, Tunesien
            // SML	4DAC	Siemens Measurements Ltd.
            // SIE	4D25	Siemens AG
            else if (boost::algorithm::istarts_with(manufacturer, "Siemens"))
                return "SIE";
            // SLB	4D82	Schlumberger Industries Ltd.
            // SON	4DEE	Sontex SA
            // SOF	4DE6	softflow.de GmbH
            // SPL	4E0C	Sappel
            // SPX	4E18	Spanner Pollux GmbH (jetzt Invensys Metering Systems AG)
            // SVM	4ECD	AB Svensk V�rmem�tning SVM
            // TCH	5068	Techem Service AG
            else if (boost::algorithm::istarts_with(manufacturer, "Techem"))
                return "TCH";
            // TIP	5130	TIP Th�ringer Industrie Produkte GmbH
            // UAG	5427	Uher
            // UGI	54E9	United Gas Industries
            // VES	58B3	ista Deutschland (bis 2005 Viterra Energy Services)
            // VPI	5A09	Van Putten Instruments B.V.
            // WMO	5DAF	Westermo Teleindustri AB, Schweden
            // YTE	6685	Yuksek Teknoloji, T�rkei
            // ZAG	6827	Zellwerg Uster AG
            // ZAP	6830	Zaptronix
            // ZIV	6936	ZIV Aplicaciones y Tecnologia, S.A.
            else
                return manufacturer.substr(0, 3);
        }
        return manufacturer;
    }

    std::string cleanup_server_id(
        std::string const &meter_id,
        std::string const &manufacturer_code,
        bool wired,
        std::uint8_t version,
        std::uint8_t medium) {
        if (manufacturer_code.size() == 3) {

            //
            //	extract manufacrurer code as array[]
            //
            std::uint16_t mcode = mbus::encode_id(manufacturer_code);
            auto const ac = cyng::to_array<std::uint8_t>(mcode);

            auto const m = to_meter_id(meter_id);
            BOOST_ASSERT(m.size() == 4);

            //
            //	build server id
            //
            srv_id_t server_id = {
                static_cast<std::uint8_t>(wired ? 2 : 1), //	wired M-Bus
                ac.at(0),                                 //	manufacturer
                ac.at(1),
                static_cast<std::uint8_t>(m.at(3)), //	meter ID (reverse)
                static_cast<std::uint8_t>(m.at(2)),
                static_cast<std::uint8_t>(m.at(1)),
                static_cast<std::uint8_t>(m.at(0)),
                version, //	version
                medium   //	2 == electricity
            };

            return srv_id_to_str(server_id);
        }

        return meter_id;
    }

    upload_policy to_upload_policy(std::string str) {
        if (boost::algorithm::equals(str, "append"))
            return upload_policy::APPEND;
        else if (boost::algorithm::equals(str, "subst"))
            return upload_policy::OVERWRITE;
        else if (boost::algorithm::equals(str, "delete"))
            return upload_policy::REMOVE;
        /*else */ return upload_policy::MERGE;
    }

    std::string extract_value(std::map<std::string, std::string> const &data, std::string const &key, std::string def) {
        auto const pos = data.find(key);
        return (pos != data.end()) ? pos->second : def;
    }

} // namespace smf
