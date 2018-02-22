/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/sml/mbus/defs.h>
#include <limits>

namespace node
{
	namespace sml
	{

		std::uint16_t encode_id(std::string const& id)
		{
			if (id.size() == 3)
			{
				// [ASCII(1. Buchstabe)-64] * 32 * 32 + [ASCII(2. Buchstabe)-64] * 32 + [ASCII(3. Buchstabe)-64]
				return ((id.at(0) - 64) * 1024)
					+ ((id.at(1) - 64) * 32)
					+ (id.at(2) - 64);
			}
			return std::numeric_limits<std::uint16_t>::max();
		}

		std::string decode(std::uint16_t const& code)
		{
			std::string man;

			const std::uint16_t c1 = code & 0x7c00;
			man.push_back(c1 / 1024 + 64);

			const std::uint16_t c2 = code & 0x3E0;
			man.push_back(c2 / 32 + 64);

			const std::uint16_t c3 = code & 0x1F;
			man.push_back(c3 + 64);
			return man;
		}

		std::string decode(char c1, char c2)
		{
			const std::uint16_t code = static_cast<unsigned char>(c1)+(static_cast<unsigned char>(c2) << 8);
			return decode(code);
		}

		std::string get_manufacturer_name(std::uint16_t const& code)
		{
			switch (code)
			{
			case 0x0442: return "ABB Kent Meters AB";
			case 0x0465: return "Actaris (Elektrizität)";
			case 0x0467: return "Actaris (Gas)";
			case 0x0477: return "Actaris (Wasser und Wärme)";
			case 0x04A7: return "AEG";
			case 0x04AC: return "Kohler, Türkei";
			case 0x04AD: return "S.C. AEM S.A. Romania";
			case 0x05B0: return "Ampy Automation Digilog Ltd";
			case 0x05B4: return "Aquametro";
			case 0x0613: return "Apsis Kontrol Sistemleri, Türkei";
			case 0x08A3: return "Berg Energiekontrollsysteme GmbH";
			case 0x08B2: return "Bernina Electronic AG";
			case 0x0A65: return "Basari Elektronik A.S., Türkei";
			case 0x0A74: return "BESTAS Elektronik Optik, Türkei";
			case 0x0C49: return "Circuit Breaker Industries, Südafrika";
			case 0x0D8F: return "Clorius Raab Karcher Energie Service A/S";
			case 0x0DEE: return "Conlog";
			case 0x0F4D: return "Cazzaniga S.p.A.";
			case 0x102E: return "Danubia";
			case 0x10D3: return "Danfoss A/S";
			case 0x1347: return "Deutsche Zählergesellschaft";
			case 0x148D: return "EDMI Pty.Ltd.";
			case 0x14C5: return "Engelmann Sensor GmbH";
			case 0x1574: return "PA KVANT J.S., Russland";
			case 0x158D: return "Elektromed Elektronik Ltd, Türkei";
			case 0x1593: return "ELSTER Produktion GmbH";
			case 0x15A8: return "EMH Elektrizitätszähler GmbH & CO KG";
			case 0x15B5: return "EMU Elektronik AG";
			case 0x15AF: return "Enermet";
			case 0x15C4: return "ENDYS GmbH";
			case 0x15D0: return "Kiev Polytechnical Scientific Research";
			case 0x15D4: return "ENTES Elektronik, Türkei";
			case 0x164C: return "Erelsan Elektrik ve Elektronik, Türkei";
			case 0x166D: return "Starion Elektrik ve Elektronik, Türkei";
			case 0x16B2: return "Eurometers Ltd";
			case 0x16F4: return "Elin Wasserwerkstechnik";
			case 0x18A4: return "Federal Elektrik, Türkei";
			case 0x19AC: return "Siemens Measurements Ltd.( Formerly FML Ltd.)";
			case 0x1C4A: return "Grundfoss A/S";
			case 0x1CA3: return "GEC Meters Ltd.";
			case 0x1CD8: return "GEx Ltd.";	//	virtual company
			case 0x1E70: return "Ingenieurbuero Gasperowicz";
			case 0x1EE6: return "Gas- u. Wassermessfabrik Luzern";
			case 0x20A7: return "Hamburger Elektronik Gesellschaft";
			case 0x20AC: return "Heliowatt";
			case 0x2283: return "Horstmann Timers and Controls Ltd.";
			case 0x2324: return "Hydrometer GmbH";
			case 0x246D: return "Intracom, Griechenland";
			case 0x2485: return "IMIT S.p.A.";
			case 0x25D6: return "Invensys Metering Systems AG";
				//	case 0x25D6: return "Invensys Metering Systems";
				//	case 0x25D6: return "Invensys Metering Systems";
			case 0x266B: return "Iskraemeco, Slovenia";
			case 0x2674: return "ista Deutschland (bis 2005 Viterra Energy Services)";
			case 0x2692: return "Itron";
			case 0x26EB: return "IWK Regler und Kompensatoren GmbH";
			case 0x2C2D: return "Kamstrup Energie A/S";
			case 0x2D0C: return "Kohler, Türkei";
			case 0x2D65: return "KK-Electronic A/S";
			case 0x2DD8: return "KONNEX-based users (Siemens Regensburg)";
			case 0x2E4F: return "Kromschröder";
			case 0x2E74: return "Kundo SystemTechnik GmbH";
			case 0x30AD: return "LEM HEME Ltd., UK";
			case 0x30E2: return "Landis & Gyr Energy Management (UK) Ltd.";
			case 0x30E4: return "Landis & Gyr Deutschland";
			case 0x30FA: return "Landis & Gyr Zug";
			case 0x3101: return "Atlantic Meters, Südafrika";
			case 0x31AC: return "LUMEL, Polen";
			case 0x3265: return "Landis & Staefa electronic";
			case 0x3270: return "Landis & Staefa production";
			case 0x32A7: return "Landis & Staefa";
			case 0x327A: return "Siemens Building Technologies";
			case 0x3424: return "Maddalena S.r.I., Italien";
			case 0x34A9: return "H. Meinecke AG (jetzt Invensys Metering Systems AG)";
			case 0x3573: return "MAK-SAY Elektrik Elektronik, Türkei";
			case 0x35D3: return "MANAS Elektronik, Türkei";
			case 0x3613: return "Multiprocessor Systems Ltd, Bulgarien";
			case 0x3683: return "Metering Technology Corporation, USA";
			case 0x3933: return "Nisko Industries Israel";
			case 0x39B3: return "Nisko Advanced Metering Solutions Israel";
			case 0x3A4D: return "Norm Elektronik, Türkei";
			case 0x3DD2: return "ONUR Elektroteknik, Türkei";
			case 0x4024: return "PadMess GmbH";
			case 0x41A7: return "Spanner-Pollux GmbH (jetzt Invensys Metering Systems AG)";
			case 0x4249: return "Polymeters Response International Ltd.";
			case 0x4833: return "Hydrometer GmbH";
			case 0x48AC: return "Relay GmbH";
			case 0x4965: return "Raab Karcher ES (jetzt ista Deutschland)";
			case 0x4C30: return "Sappel";
			case 0x4C68: return "Schnitzel GmbH";
			case 0x4CAE: return "Sensus GmbH";
			case 0x4DA3: return "SMC";
			case 0x4DA5: return "Siame, Tunesien";
			case 0x4DAC: return "Siemens Measurements Ltd.";
			case 0x4D25: return "Siemens AG";
			case 0x4D82: return "Schlumberger Industries Ltd.";
			case 0x4DE6: return "softflow.de GmbH";
			case 0x4E0C: return "Sappel";
			case 0x4E18: return "Spanner Pollux GmbH (jetzt Invensys Metering Systems AG)";
			case 0x4ECD: return "AB Svensk Värmemätning SVM";
			case 0x5068: return "Techem Service AG";
			case 0x5427: return "Uher";
			case 0x54E9: return "United Gas Industries";
			case 0x58B3: return "ista Deutschland (bis 2005 Viterra Energy Services)";
			case 0x5A09: return "Van Putten Instruments B.V.";
			case 0x5DAF: return "Westermo Teleindustri AB, Schweden";
			case 0x6685: return "Yuksek Teknoloji, Türkei";
			case 0x6827: return "Zellwerg Uster AG";
			case 0x6830: return "Zaptronix";
			case 0x6936: return "ZIV Aplicaciones y Tecnologia, S.A.";
			default:
				break;
			}
			return decode(code);
		}

		std::string get_manufacturer_name(char c1, char c2)
		{
			const std::uint16_t code = static_cast<unsigned char>(c1)+(static_cast<unsigned char>(c2) << 8);
			return get_manufacturer_name(code);
		}
		
		std::string get_medium_name(std::uint8_t m)	
		{
			switch (m)
			{
			case 0: return "other";
			case 1: return "oil";
			case 2: return "electricity";
			case 3: return "gas";
			case 4: return "heat (return)";  //  (Wärme-Rücklauf)
			case 5: return "steam";
			case 6: return "water (hot)";
			case 7: return "water";
			case 8: return "heat cost allocator";
			case 9: return "compressed air";
			case 0xA:   return "gas (mode 2)";	//	cooling load meter
			case 0xB:   return "heat (mode 2)";	//	cooling load meter
			case 0xC:   return "water (hot - mode 2)";  // (Wärme-Vorlauf)
			case 0xD:   return "water (cold - mode 2)";
			case 0xE:   return "heat cost allocator (mode 2)";	//	bus
			case 0xF:   return "reserved";
			default:
				break;
			}
			return std::to_string(m);
		}
		

	}	//	mbus
}	//	noddy


