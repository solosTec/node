/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <upload.h>
#include <db.h>

#include <smf/mbus/flag_id.h>
#include <smf/mbus/server_id.h>

#include <cyng/log/record.h>
#include <cyng/io/ostream.h>
#include <cyng/parse/csv/csv_parser.h>
#include <cyng/parse/csv/line_cast.hpp>
#include <cyng/obj/value_cast.hpp>
#include <cyng/obj/array_cast.hpp>
#include <cyng/parse/string.h>

#include <iostream>

namespace smf {

	upload::upload(cyng::logger logger, db& cache)
	: logger_(logger)
		, db_(cache)
		, uidgen_()
	{}

	void upload::config_bridge(std::string name, std::string policy, std::string const& content, char sep) {

		std::size_t counter{ 0 };
		cyng::csv::parser p(sep, [&, this](cyng::csv::line_t&& vec) {
			if (counter != 0 && vec.size() > 10) {
				CYNG_LOG_DEBUG(logger_, "[upload] #" << counter << ": " << vec.size());

				//	Meter_ID,Ift_Type,GWY_IP,Manufacturer,Meter_Type,Protocol,Area,Name,In_enDS,Key,AMR Address,Comments
				//	[0] Meter_ID (MA0000000000000000000000004354752)
				//	[1] Ift_Type (wMBus)
				//	[2] GWY_IP (10.132.32.142)
				//	[3]	Manufacturer (EMH)
				//	[4] Meter_Type (EMH ED100L)
				//	[5] Protocol (Wireless MBUS)
				//	[6] Area (Moumnat,Maison 44)
				//	[7] Name (Yes)
				//	[8] In_enDS (2BFFCB61D7E8DC439239555D3DFE1B1D)
				//	[9] Key,
				//	[10] AMR Address,
				//	[11] Comments

				//
				//	extract Meter_ID
				//	have to contain at least 8 bytes
				//
				if (vec.size() < 10)	return;
				auto const mc = vec.at(0);
				if (mc.size() < 8)	return;
			
				//	Meter_ID,Ift_Type,GWY_IP,Manufacturer,Meter_Type,Protocol,Area,Name,In_enDS,Key,AMR Address,Comments
				std::string meter_id = (mc.size() > 8)
					? mc.substr(mc.length() - 8)
					: mc
					;

				//
				//	Search for an existing meter with the same ID.
				//	If meter ID already exists use this PK otherwise create a new one.
				//
				auto key = db_.lookup_meter_by_id(meter_id);
				if (key.empty()) {
					key = cyng::key_generator(uidgen_());
				}

				//	Ift_Type [RS485|wMBus]
				auto const adapter_type = vec.at(1);

				//	Manufacturer
				auto const manufacturer_code = cleanup_manufacturer_code(vec.at(4));

				auto const server_id = cleanup_server_id(meter_id
					, manufacturer_code
					, boost::algorithm::equals(adapter_type, "RS485")	//	true == wired 
					, 1		//	version
					, 2		//	electricity
				);
				auto const meter_type = vec.at(4);
				auto const protocol = vec.at(5);

				auto const aes = cyng::to_aes_key<cyng::crypto::aes128_size>(vec.at(10).size() == 32 ? vec.at(10) : "00000000000000000000000000000000");

			}
			++counter;
			});

		p.read(content.begin(), content.end());
	}

	std::string cleanup_manufacturer_code(std::string manufacturer)
	{
		if (manufacturer.size() > 2) {
			if (boost::algorithm::istarts_with(manufacturer, "Easymeter")) return "ESY";	//	not offical
			//ABB	0442	ABB AB, P.O. Box 1005, SE-61129 Nyk�ping, Nyk�ping,Sweden
			//ACE	0465	Actaris (Elektrizit�t)
			//ACG	0467	Actaris (Gas)
			//ACW	0477	Actaris (Wasser und W�rme)
			else if (boost::algorithm::istarts_with(manufacturer, "Actaris"))	return "ACE";
			//AEG	04A7	AEG
			//AEL	04AC	Kohler, T�rkei
			else if (boost::algorithm::istarts_with(manufacturer, "Kohler"))	return "AEL";
			//AEM	04AD	S.C. AEM S.A. Romania
			else if (boost::algorithm::istarts_with(manufacturer, "S.C. AEM"))	return "AEM";
			//AMP	05B0	Ampy Automation Digilog Ltd
			else if (boost::algorithm::istarts_with(manufacturer, "Ampy"))	return "AMP";
			//AMT	05B4	Aquametro
			//APS	0613	Apsis Kontrol Sistemleri, T�rkei
			//BEC	08A3	Berg Energiekontrollsysteme GmbH
			//BER	08B2	Bernina Electronic AG
			//BSE	0A65	Basari Elektronik A.S., T�rkei
			//BST	0A74	BESTAS Elektronik Optik, T�rkei
			//CBI	0C49	Circuit Breaker Industries, S�dafrika
			//CLO	0D8F	Clorius Raab Karcher Energie Service A/S
			//CON	0DEE	Conlog
			//CZM	0F4D	Cazzaniga S.p.A.
			//DAN	102E	Danubia
			//DFS	10D3	Danfoss A/S
			//DME	11A5	DIEHL Metering, Industriestrasse 13, 91522 Ansbach, Germany
			else if (boost::algorithm::istarts_with(manufacturer, "Diehl"))	return "DME";
			//DZG	1347	Deutsche Z�hlergesellschaft
			else if (boost::algorithm::istarts_with(manufacturer, "Deutsche Z�hlergesellschaft"))	return "DZG";
			//EDM	148D	EDMI Pty.Ltd.
			//EFE	14C5	Engelmann Sensor GmbH
			//EKT	1574	PA KVANT J.S., Russland
			//ELM	158D	Elektromed Elektronik Ltd, T�rkei
			//ELS	1593	ELSTER Produktion GmbH
			else if (boost::algorithm::istarts_with(manufacturer, "Elster")) return "ELS";
			//EMH	15A8	EMH Elektrizit�tsz�hler GmbH & CO KG
			else if (boost::algorithm::istarts_with(manufacturer, "EMH")) return "EMH";
			//EMU	15B5	EMU Elektronik AG
			//EMO	15AF	Enermet
			//END	15C4	ENDYS GmbH
			//ENP	15D0	Kiev Polytechnical Scientific Research
			//ENT	15D4	ENTES Elektronik, T�rkei
			//ERL	164C	Erelsan Elektrik ve Elektronik, T�rkei
			//ESM	166D	Starion Elektrik ve Elektronik, T�rkei
			//EUR	16B2	Eurometers Ltd
			//EWT	16F4	Elin Wasserwerkstechnik
			//FED	18A4	Federal Elektrik, T�rkei
			//FML	19AC	Siemens Measurements Ltd.( Formerly FML Ltd.)
			//GBJ	1C4A	Grundfoss A/S
			//GEC	1CA3	GEC Meters Ltd.
			//GSP	1E70	Ingenieurbuero Gasperowicz
			//GWF	1EE6	Gas- u. Wassermessfabrik Luzern
			//HEG	20A7	Hamburger Elektronik Gesellschaft
			//HEL	20AC	Heliowatt
			//HTC	2283	Horstmann Timers and Controls Ltd.
			//HYD	2324	Hydrometer GmbH
			else if (boost::algorithm::istarts_with(manufacturer, "Hydrometer"))	return "HYD";
			//ICM	246D	Intracom, Griechenland
			//IDE	2485	IMIT S.p.A.
			//INV	25D6	Invensys Metering Systems AG
			//ISK	266B	Iskraemeco, Slovenia
			else if (boost::algorithm::istarts_with(manufacturer, "Iskraemeco"))	return "ISK";
			//IST	2674	ista Deutschland (bis 2005 Viterra Energy Services)
			//ITR	2692	Itron
			else if (boost::algorithm::istarts_with(manufacturer, "Itron"))	return "ITR";
			//IWK	26EB	IWK Regler und Kompensatoren GmbH
			//KAM	2C2D	Kamstrup Energie A/S
			else if (boost::algorithm::istarts_with(manufacturer, "Kamstrup"))	return "KAM";
			//KHL	2D0C	Kohler, T�rkei
			else if (boost::algorithm::istarts_with(manufacturer, "Kohler"))	return "KHL";
			//KKE	2D65	KK-Electronic A/S
			//KNX	2DD8	KONNEX-based users (Siemens Regensburg)
			//KRO	2E4F	Kromschr�der
			//KST	2E74	Kundo SystemTechnik GmbH
			//LEM	30AD	LEM HEME Ltd., UK
			//LGB	30E2	Landis & Gyr Energy Management (UK) Ltd.
			else if (boost::algorithm::istarts_with(manufacturer, "Landis & Gyr Energy"))	return "LGB";
			//LGD	30E4	Landis & Gyr Deutschland
			else if (boost::algorithm::istarts_with(manufacturer, "Landis & Gyr Deutschland"))	return "LGD";
			//LGZ	30FA	Landis & Gyr Zug
			else if (boost::algorithm::istarts_with(manufacturer, "Landis & Gyr Zug"))	return "LGZ";
			else if (boost::algorithm::istarts_with(manufacturer, "L & G"))	return "LGZ";	//	switzerland / zug

			//LHA	3101	Atlantic Meters, S�dafrika
			//LML	31AC	LUMEL, Polen
			//LSE	3265	Landis & Staefa electronic
			//LSP	3270	Landis & Staefa production
			//LUG	32A7	Landis & Staefa
			//LSZ	327A	Siemens Building Technologies
			else if (boost::algorithm::istarts_with(manufacturer, "Siemens Building"))	return "LSZ";
			//MAD	3424	Maddalena S.r.I., Italien
			//MEI	34A9	H. Meinecke AG (jetzt Invensys Metering Systems AG)
			//MKS	3573	MAK-SAY Elektrik Elektronik, T�rkei
			//MNS	35D3	MANAS Elektronik, T�rkei
			//MPS	3613	Multiprocessor Systems Ltd, Bulgarien
			//MTC	3683	Metering Technology Corporation, USA
			//NIS	3933	Nisko Industries Israel
			//NMS	39B3	Nisko Advanced Metering Solutions Israel
			//NRM	3A4D	Norm Elektronik, T�rkei
			//ONR	3DD2	ONUR Elektroteknik, T�rkei
			//PAD	4024	PadMess GmbH
			//PMG	41A7	Spanner-Pollux GmbH (jetzt Invensys Metering Systems AG)
			//PRI	4249	Polymeters Response International Ltd.
			//RAS	4833	Hydrometer GmbH
			//REL	48AC	Relay GmbH
			else if (boost::algorithm::istarts_with(manufacturer, "Relay"))	return "REL";
			//RKE	4965	Raab Karcher ES (jetzt ista Deutschland)
			//SAP	4C30	Sappel
			//SCH	4C68	Schnitzel GmbH
			//SEN	4CAE	Sensus GmbH
			//SMC	4DA3	 
			//SME	4DA5	Siame, Tunesien
			//SML	4DAC	Siemens Measurements Ltd.
			//SIE	4D25	Siemens AG
			else if (boost::algorithm::istarts_with(manufacturer, "Siemens"))	return "SIE";
			//SLB	4D82	Schlumberger Industries Ltd.
			//SON	4DEE	Sontex SA
			//SOF	4DE6	softflow.de GmbH
			//SPL	4E0C	Sappel
			//SPX	4E18	Spanner Pollux GmbH (jetzt Invensys Metering Systems AG)
			//SVM	4ECD	AB Svensk V�rmem�tning SVM
			//TCH	5068	Techem Service AG
			else if (boost::algorithm::istarts_with(manufacturer, "Techem"))	return "TCH";
			//TIP	5130	TIP Th�ringer Industrie Produkte GmbH
			//UAG	5427	Uher
			//UGI	54E9	United Gas Industries
			//VES	58B3	ista Deutschland (bis 2005 Viterra Energy Services)
			//VPI	5A09	Van Putten Instruments B.V.
			//WMO	5DAF	Westermo Teleindustri AB, Schweden
			//YTE	6685	Yuksek Teknoloji, T�rkei
			//ZAG	6827	Zellwerg Uster AG
			//ZAP	6830	Zaptronix
			//ZIV	6936	ZIV Aplicaciones y Tecnologia, S.A.
			else return manufacturer.substr(0, 3);
		}
		return manufacturer;
	}

	std::string cleanup_server_id(std::string const& meter_id
		, std::string const& manufacturer_code
		, bool wired
		, std::uint8_t version
		, std::uint8_t medium)
	{
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
			std::array<char, 9>	server_id = {
				(wired ? 2 : 1),	//	wired M-Bus
				static_cast<char>(ac.at(0)),	//	manufacturer
				static_cast<char>(ac.at(1)),
				m.at(3),	//	meter ID (reverse)
				m.at(2),
				m.at(1),
				m.at(0),
				version,	//	version
				medium		//	2 == electricity
			};

			return srv_id_to_str(server_id);
		}

		return meter_id;
	}

}


