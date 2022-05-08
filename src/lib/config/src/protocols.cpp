/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/config/protocols.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    namespace config {
        std::string get_name(protocol p) {
            switch (p) {
            case protocol::ANY:
                return "any";
            case protocol::RAW:
                return "raw";
            case protocol::TCP:
                return "TCP";
            case protocol::IPT:
                return "IP-TDIN-E43863-4";
            case protocol::IEC:
                return "IEC-62056";
            case protocol::WIRED_MBUS:
                return "M-Bus";
            case protocol::WIRELESS_MBUS:
                return "wM-Bus-EN13757-4";
            case protocol::HDLC:
                return "HDLC";
            case protocol::SML:
                // https://www.bsi.bund.de/SharedDocs/Downloads/DE/BSI/Publikationen/TechnischeRichtlinien/TR03109/TR-03109-1_Anlage_Feinspezifikation_Drahtgebundene_LMN-Schnittstelle_Teilb.pdf?__blob=publicationFile
                return "SMLv1.04";
            case protocol::DLMS:
                return "DLMS";
            case protocol::COSEM:
                return "COSEM";
            default:
                break;
            }
            return "unknown protocol";
        }

    } // namespace config
} // namespace smf
